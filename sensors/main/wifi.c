/**
 * @file
 * WiFi code.
 *
 * Okay, that's a lie. It does MQTT too....
 *
 * Derived from ESP8266_RTOS_SDK example
 * examples/wifi/getting_started/station/main/station_example_main.c, which is
 * licensed as Public Domain.
 *
 * Also derived from ESP8266_RTOS_SDK example
 * examples/protocols/mqtt/ssl/main/app_main.c, which is also licensed as
 * Public Domain.
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "queues.h"
#include "mqtt_client.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "priorities.h"
#include "wifi.h"
#include "config_storage.h"
#include "temperature.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_mqtt_status;

/** Definition bits for the above status. */
#define WIFI_STARTED       BIT0 /**< WiFi has been started. */
#define WIFI_CONNECTED     BIT1 /**< WiFi has connected to the AP. */
#define WIFI_FAIL          BIT2 /**< WiFi has failed to connect to the AP. */
#define MQTT_STARTED       BIT3 /**< MQTT has been started. */
#define MQTT_CONNECTED     BIT4 /**< MQTT has connected to the MQTT server. */
#define MQTT_SUBSCRIBED    BIT5 /**< MQTT has subscribed to the topic. */
#define MQTT_MSG_IN_FLIGHT BIT6 /**< MQTT has a message in flight. */

#define WIFI_MAXIMUM_RETRIES 3  /**< Maximum connection retries. */

static const char *TAG = "WiFi";

static int s_retry_num = 0;

esp_mqtt_client_handle_t mqtt_client = NULL;

static unsigned int mqtt_outstanding_messages = 0;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    esp_mqtt_client_handle_t client = event->client;

    int msg_id;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            xEventGroupSetBits(wifi_mqtt_status, MQTT_CONNECTED);
            msg_id = esp_mqtt_client_subscribe(client,
                                               current_config.mqtt_topic,
                                               1);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            xEventGroupClearBits(wifi_mqtt_status, MQTT_CONNECTED);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            xEventGroupSetBits(wifi_mqtt_status, MQTT_SUBSCRIBED);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            xEventGroupClearBits(wifi_mqtt_status, MQTT_SUBSCRIBED);
            break;

        case MQTT_EVENT_PUBLISHED:
            // This event, when used with QoS 1 or 2 (we are using 1), will be
            // posted when the broker accepts the message handoff. Hence, it is
            // pretty reliable (that's what QoS is for after all), and we can
            // safely go to sleep once the count reaches 0.
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            --mqtt_outstanding_messages;
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;

        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_ESP_TLS) {
                ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x",
                         event->error_handle->esp_tls_last_esp_err);
                ESP_LOGI(TAG, "Last tls stack error number: 0x%x",
                         event->error_handle->esp_tls_stack_err);
            }
            else if (event->error_handle->error_type ==
                     MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGI(TAG, "Connection refused error: 0x%x",
                         event->error_handle->connect_return_code);
            }
            else {
                ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            }
            break;

        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

/**
 * Connect to our MQTT broker.
 *
 * @note Make sure we're connected to WiFi with an IP before calling this,
 * else it will fail.
 */
static void mqtt_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = current_config.mqtt_uri,
        .client_id = current_config.station_name,
        // Not setting .cert_pem implicitly disables certificate verification,
        // which is what we actually want - since the common case for this
        // application is to use a selfsigned cert inside a closed network - or
        // even no encryption at all, which will happen if one uses an mqtt://
        // URL instead of an mqtts:// URL.
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(
                        mqtt_client,
                        ESP_EVENT_ANY_ID,
                        mqtt_event_handler,
                        mqtt_client));
    ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));
    xEventGroupSetBits(wifi_mqtt_status, MQTT_STARTED);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_ERROR_CHECK(esp_wifi_connect());
    }
    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {

        // We got disconnected, make sure connected bit is clear.
        xEventGroupClearBits(wifi_mqtt_status,
                             WIFI_CONNECTED);

        if (s_retry_num < WIFI_MAXIMUM_RETRIES) {
            ESP_ERROR_CHECK(esp_wifi_connect());
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else {
            // Give up on connecting so we don't spin here forever.
            xEventGroupSetBits(wifi_mqtt_status, WIFI_FAIL);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->ip_info.ip));
        // We've successfully connected, so reset our connection attempts
        // counter and set our connected bit.
        s_retry_num = 0;
        xEventGroupSetBits(wifi_mqtt_status, WIFI_CONNECTED);
    }
}

/**
 * Init the wifi as much as we can sans config - we'll have to wait for
 * a WIFI_START message to do that.
 *
 * @note This should be called before connect_wifi().
 */
static void wifi_init(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    wifi_mqtt_status = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Belt and suspenders for storage to RAM.
     * Here's the story - the menuconfig item ESP8266_WIFI_NVS_ENABLED
     * determines whether it should write to NVS (flash) or not. We have this
     * disabled, so it shouldn't write to NVS. Setting the storage here should
     * effectively do the same thing, so - belt and suspenders.
     */
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                    &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                    &wifi_event_handler, NULL));

    ESP_LOGI(TAG, "WiFi init finished.");
}

#if 0
/**
 * De-init (free) all the WiFi stuff.
 *
 * @note This is basically the reverse of the above and is only included for
 *       completeness - it is never called and has been disabled to suppress a
 *       compiler warning.
 * @note Make sure to call disconnect_wifi() first.
 */
static void wifi_deinit(void)
{
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
                    &wifi_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,
                    &wifi_event_handler));
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_event_loop_delete_default());
    ESP_ERROR_CHECK(esp_netif_deinit());
    vEventGroupDelete(s_wifi_event_group);
}
#endif

/**
 * Configure and connect to a WiFi AP.
 *
 * @note Make sure current_config is valid before this is called.
 */
static void connect_wifi(void)
{
    wifi_config_t wifi_config = {};

    strncpy((char *)wifi_config.sta.ssid, current_config.ssid,
            sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, current_config.pass,
            sizeof(wifi_config.sta.password));

    /* NOTE: In the example code, they ratchet up the minimum security to
     * refuse to connect to WiFi networks with poor security. I have removed
     * that code because, if you want to run a crappy network, that is your
     * choice. */

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_start());
    xEventGroupSetBits(wifi_mqtt_status, WIFI_STARTED);

    ESP_LOGI(TAG, "WiFi config finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT)
     * or connection failed for the maximum number of re-tries (WIFI_FAIL_BIT). * The bits are set by wifi_event_handler() (see above). */
    EventBits_t status = xEventGroupWaitBits(
                             wifi_mqtt_status,
                             WIFI_CONNECTED | WIFI_FAIL,
                             pdFALSE,
                             pdFALSE,
                             portMAX_DELAY);

    if (status & WIFI_CONNECTED) {
        ESP_LOGI(TAG, "connected to SSID: %s", wifi_config.sta.ssid);

        // We are now connected to WiFi, so do our MQTT connection stuff
        mqtt_start();
    }
    else if (status & WIFI_FAIL) {
        ESP_LOGI(TAG, "Failed to connect to SSID: %s", wifi_config.sta.ssid);
        // We don't need to clear the connected bit, because it's cleared when
        // we get the disconnected event in the event handler.
    }
    else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

/**
 * Disconnect the connected WiFi.
 */
static void disconnect_wifi(void)
{
    EventBits_t status = xEventGroupGetBits(wifi_mqtt_status);

    if (status & MQTT_SUBSCRIBED) {
        ESP_ERROR_CHECK(esp_mqtt_client_unsubscribe(mqtt_client,
                        current_config.mqtt_topic));
    }
    if (status & MQTT_CONNECTED) {
        ESP_ERROR_CHECK(esp_mqtt_client_disconnect(mqtt_client));
    }
    if (status & MQTT_STARTED) {
        ESP_ERROR_CHECK(esp_mqtt_client_stop(mqtt_client));
        ESP_ERROR_CHECK(esp_mqtt_client_destroy(mqtt_client));
    }

    if (status & WIFI_CONNECTED) {
        ESP_ERROR_CHECK(esp_wifi_disconnect());
    }
    if (status & WIFI_STARTED) {
        ESP_ERROR_CHECK(esp_wifi_stop());
        // We don't deinit the WiFi, because it's not necessary - the config is
        // set in connect_wifi, right before esp_wifi_start.
    }
}

/**
 * WiFi task.
 *
 * This basically just sleeps until it gets a message telling it to do
 * something.
 *
 * @param pvParameters Parameters for the function (unused)
 */
static void wifi_task(void *pvParameters)
{
    uint8_t message;
    /* temporary temperature buffer. The range is -55 to 125C (-67 to 257F),
     * which is 3 characters. We also have a decimal point and another place, for a total of 5 characters, and then a NUL, for 6, total. */
    char buffer[6];
    int mqtt_msg_id;

    EventBits_t status;

    // basic wifi init (without configuration)
    wifi_init();

    while(true) {
        if (xQueueReceive(wifi_queue, &message, portMAX_DELAY) == pdTRUE) {
            switch(message) {
                case WIFI_START:
                    // TODO: Add some blinkenlights feedback here?
                    status = xEventGroupGetBits(wifi_mqtt_status);
                    if (status & WIFI_CONNECTED) {
                        ESP_LOGI(TAG,
                                 "WiFi already connected, discarding "
                                 "spurious WIFI_START message");
                    }
                    else {
                        if (is_config_valid(&current_config)) {
                            ESP_LOGI(TAG,
                                     "wifi config looks valid, connecting");
                            connect_wifi();
                        }
                        else {
                            ESP_LOGI(TAG,
                                     "invalid config, not connecting to wifi");
                        }
                    }
                    break;
                case WIFI_STOP:
                    // No extraneous checks here, because the disconnect WiFi
                    // function checks extensively to not double-free something.
                    disconnect_wifi();
                    break;
                case WIFI_SEND_MQTT:
                    status = xEventGroupGetBits(wifi_mqtt_status);
                    if (status & MQTT_SUBSCRIBED) {
                        memset(buffer, 0, sizeof(buffer));
                        sprintf(buffer, "%.1f", get_last_temp());
                        mqtt_msg_id = esp_mqtt_client_publish(
                                          mqtt_client,
                                          current_config.mqtt_topic,
                                          buffer,
                                          strlen(buffer),
                                          1,
                                          false);
                        if (mqtt_msg_id >= 0) {
                            // We submitted a message to MQTT and it is now in
                            // flight. The temp task checks this count via //
                            // get_mqtt_messages_oustanding before going to
                            // sleep.
                            ++mqtt_outstanding_messages;
                            ESP_LOGI(TAG, "mqtt publish successful, msg_id=%d",
                                     mqtt_msg_id);
                        }
                        else {
                            ESP_LOGE(TAG, "mqtt publish unsuccessful");
                        }
                    }
                    else {
                        ESP_LOGI(TAG,
                                 "MQTT not subscribed, "
                                 "not publishing message.");
                    }
                    break;
                default:
                    ESP_LOGE(TAG, "Unknown command: %d", message);
                    break;
            }
        }
        // else we didn't get an item from the queue, just go back and wait.
    }
}

void wifi_enable(void)
{
    uint8_t message = WIFI_START;

    xQueueSend(wifi_queue, &message, portMAX_DELAY);
}

void wifi_disable(void)
{
    uint8_t message = WIFI_STOP;

    xQueueSend(wifi_queue, &message, portMAX_DELAY);
}

void wifi_restart(void)
{
    // This just submits both items to the message queue, so they'll be
    // processed in order.

    uint8_t message = WIFI_STOP;

    xQueueSend(wifi_queue, &message, portMAX_DELAY);

    message = WIFI_START;

    xQueueSend(wifi_queue, &message, portMAX_DELAY);
}

/**
 * Send last temperature reading over MQTT.
 */
void wifi_send_mqtt_temperature(void)
{
    uint8_t message = WIFI_SEND_MQTT;

    xQueueSend(wifi_queue, &message, portMAX_DELAY);
}

void start_wifi(void)
{
    xTaskCreate(wifi_task, "wifi", 3072, NULL, WIFI_TASK_PRIORITY, NULL);
}

void emit_mqtt_status(void)
{
    EventBits_t status = xEventGroupGetBits(wifi_mqtt_status);

    printf("MQTT information:\n");
    if (!(status & MQTT_STARTED)) {
        printf("\tStarted: No\n");
    }
    else {
        printf("\tStarted: No\n");
        printf("\tURI:\t%s\n", current_config.mqtt_uri);
        printf("\tConnected:\t");
        if (status & MQTT_CONNECTED) {
            printf("Yes\n");
        }
        else {
            printf("No\n");
        }

        printf("\tTopic:\t%s\n", current_config.mqtt_topic);
        printf("\tSubscribed:\t");
        if (status & MQTT_SUBSCRIBED) {
            printf("Yes\n");
        }
        else {
            printf("No\n");
        }
    }
}

unsigned int get_mqtt_outstanding_messages_(void)
{
    return mqtt_outstanding_messages;
}
