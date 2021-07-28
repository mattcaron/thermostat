/**
 * @file
 * WiFi code.
 *
 * Derived from ESP8266_RTOS_SDK example
 * examples/wifi/getting_started/station/main/station_example_main.c, which is
 * licensed as Public Domain.
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

#include "lwip/err.h"
#include "lwip/sys.h"

#include "priorities.h"
#include "wifi.h"
#include "config_storage.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0 /**< We have connected to the AP. */
#define WIFI_FAIL_BIT      BIT1 /**< We have failed to connect to the AP. */

#define WIFI_MAXIMUM_RETRIES 3  /**< Maximum connection retries. */

static const char *TAG = "WiFi";

static int s_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAXIMUM_RETRIES) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
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

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                    &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                    &event_handler, NULL));

    ESP_LOGI(TAG, "WiFi init finished.");
}

/**
 * De-init (free) all the WiFi stuff.
 *
 * @note This is basically the reverse of the above and is only included for
 *       completeness - it is unlikely to ever be called.
 * @note Make sure to call disconnect_wifi() first.
 */
static void wifi_deinit(void)
{
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
                    &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,
                    &event_handler));
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_event_loop_delete_default());
    ESP_ERROR_CHECK(esp_netif_deinit());
    vEventGroupDelete(s_wifi_event_group);
}

/**
 * Configure and connect to a WiFi AP.
 *
 * @note Make sure current_config is valid before this is called.
 */
static void connect_wifi(void)
{
    wifi_config_t wifi_config = {};

    strncpy((char *)&wifi_config.sta.ssid, current_config.ssid,
            sizeof(wifi_config.sta.ssid));
    strncpy((char *)&wifi_config.sta.password, current_config.pass,
            sizeof(wifi_config.sta.password));

    /* NOTE: In the example code, they ratchet up the minimum security to
     * refuse to connect to WiFi networks with poor security. I have removed
     * that code because, if you want to run a crappy network, that is your
     * choice. */

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi config finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT)
     * or connection failed for the maximum number of re-tries (WIFI_FAIL_BIT). * The bits are set by event_handler() (see above). */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence
     * we can test which event actually happened. */
    if (bits &WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to SSID: %s", wifi_config.sta.ssid);
    }
    else if (bits &WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID: %s", wifi_config.sta.ssid);
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
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_stop());
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

    // basic wifi init (without configuration)
    wifi_init();

    while(true) {
        if (xQueueReceive(wifi_queue, &message, portMAX_DELAY) == pdTRUE) {
            switch(message) {
                case WIFI_START:
                    connect_wifi();
                    break;
                case WIFI_STOP:
                    disconnect_wifi();
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
 * Basic WiFi task creation.
 */
void start_wifi(void)
{
    xTaskCreate(wifi_task, "wifi", 2048, NULL, WIFI_TASK_PRIORITY, NULL);
}
