/**
 * @file
 * WiFi code.
 * 
 * Okay, that's a lie. It does COAP too....
 *
 * Derived from ESP8266_RTOS_SDK example
 * examples/wifi/getting_started/station/main/station_example_main.c, which is
 * licensed as Public Domain.
 *
 * Also derived from ESP8266_RTOS_SDK example
 * examples/protocols/coap_client/main/coap_client_example_main.c, which is
 * also licensed as Public Domain.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_sleep.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "queues.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "coap/coap.h"

#include "priorities.h"
#include "wifi.h"
#include "config_storage.h"
#include "temperature.h"
#include "ap_cache_storage.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_status;

/** Definition bits for the above status. */
#define WIFI_OFF           BIT0 /**< WiFi is off. */
#define WIFI_STOPPING      BIT1 /**< WiFi is in the process of being stopped. */
#define WIFI_STARTED       BIT2 /**< WiFi has been started. */
#define WIFI_CONNECTED     BIT3 /**< WiFi has connected to the AP. */
#define WIFI_FAIL          BIT4 /**< WiFi has failed to connect to the AP. */
#define COAP_QUEUE_EMPTY   BIT5 /**< Whether there are no COAP messages being sent. */
#define COAP_SUCCESSFUL    BIT6 /**< Whether the last CoAP message was successful or not. */

#define WIFI_MAXIMUM_RETRIES 3  /**< Maximum connection retries. */

#define COAP_TIMEOUT_S 5        /**< Maximum time to wait for a successful
                                     reply from the CoAP server. */

static const char *TAG = "WiFi";

static bool cache_is_valid = false;

/**
 * Counter for the number of wifi retries so far
 */
static int s_retry_num = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    EventBits_t status;
    esp_err_t result;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        result = tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA,
                                            current_config.station_name);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set hostname: %s", esp_err_to_name(result));
        }

        ESP_ERROR_CHECK(esp_wifi_connect());
    }
    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {

        // We got disconnected, make sure connected bit is clear.
        xEventGroupClearBits(wifi_status,
                             WIFI_CONNECTED);

        // If we are in the process of stopping, or if WiFi is off, DO NOT try
        // to connect - doing so causes an error.
        status = xEventGroupGetBits(wifi_status);
        if (status & (WIFI_OFF | WIFI_STOPPING)) {
            ESP_LOGE(TAG, "WiFI is shutting down, not reconnecting.");
        }
        else {
            if (s_retry_num < WIFI_MAXIMUM_RETRIES) {
                ESP_ERROR_CHECK(esp_wifi_connect());
                s_retry_num++;
                ESP_LOGI(TAG, "retry to connect to the AP");
            }
            else {
                // Give up on connecting so we don't spin here forever.
                xEventGroupSetBits(wifi_status, WIFI_FAIL);
            }
            ESP_LOGI(TAG,"connect to the AP fail");
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->ip_info.ip));
        // We've successfully connected, so reset our connection attempts
        // counter and set our connected bit.
        s_retry_num = 0;
        xEventGroupSetBits(wifi_status, WIFI_CONNECTED);
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

    wifi_status = xEventGroupCreate();

    // Some bits are set for off/disabled/empty - set those now, because we
    // start with wifi stopped and the queue empty.
    xEventGroupSetBits(wifi_status, WIFI_OFF);

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Set radio to not recalibrate after deep sleep and wakeup, to save power.
    // Note that this is has no return value, so there is nothing to check.
    esp_deep_sleep_set_rf_option(2);

    /* Belt and suspenders for storage to RAM.
     * Here's the story - the menuconfig item ESP8266_WIFI_NVS_ENABLED
     * determines whether it should write to NVS (flash) or not. We have this
     * disabled, so it shouldn't write to NVS. Setting the storage here should
     * effectively do the same thing, so - belt and suspenders.
     *
     * Note that the wifi config storage and caching almost certainly duplicates
     * large portions of this. However, given the nature of this application
     * (that is, frequent reboots due to how deep sleep works on the ESP8266),
     * I don't want to see writes to flash that frequently, so I implemented it
     * myself for more direct control.
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
    wifi_ap_record_t ap_info = {};

    xEventGroupClearBits(wifi_status, WIFI_OFF);

    strncpy((char *)wifi_config.sta.ssid, current_config.ssid,
            sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, current_config.pass,
            sizeof(wifi_config.sta.password));

    if (current_config.cache_ap_info) {
        ESP_LOGI(TAG, "AP info cache is enabled, reading back.");
        cache_is_valid = read_ap_cache_from_nvs();
        if (cache_is_valid) {
            ESP_LOGI(TAG, "AP info cache is valid, using.");
            wifi_config.sta.bssid_set = true;
            memcpy(wifi_config.sta.bssid, ap_cache.bssid, sizeof(wifi_config.sta.bssid));
            wifi_config.sta.channel = ap_cache.channel;
            ESP_LOGI(TAG, "Cached BSSID = %x:%x:%x:%x:%x:%x, channel = %d",
            ap_cache.bssid[0], ap_cache.bssid[1], ap_cache.bssid[2], ap_cache.bssid[3], ap_cache.bssid[4], ap_cache.bssid[5], ap_cache.channel);
        }
        else {
            ESP_LOGI(TAG, "AP info cache is invalid.");
        }
    }
    else {
        ESP_LOGI(TAG, "AP info cache is disabled.");
    }

    /* NOTE: In the example code, they ratchet up the minimum security to
     * refuse to connect to WiFi networks with poor security. I have removed
     * that code because, if you want to run a crappy network, that is your
     * choice. */

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_start());
    xEventGroupSetBits(wifi_status, WIFI_STARTED);

    ESP_LOGI(TAG, "WiFi config finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED)
     * or connection failed for the maximum number of re-tries (WIFI_FAIL).
     * The bits are set by wifi_event_handler() (see above). */
    EventBits_t status = xEventGroupWaitBits(
                             wifi_status,
                             WIFI_CONNECTED | WIFI_FAIL,
                             pdFALSE,
                             pdFALSE,
                             portMAX_DELAY);

    if (status & WIFI_CONNECTED) {
        ESP_LOGI(TAG, "connected to SSID: %s", wifi_config.sta.ssid);
        // We are now connected to WiFi.
        // If we're supposed to cache info and the cache isn't currently valid,
        // write it and set the cache to valid.
        if (current_config.cache_ap_info && !cache_is_valid) {
            ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&ap_info));
            ap_cache.channel = ap_info.primary; // primary channel
            memcpy(ap_cache.bssid, ap_info.bssid, sizeof(ap_cache.bssid));

            if (write_ap_cache_to_nvs()) {
                cache_is_valid = true;
                ESP_LOGI(TAG, "Saved AP info to cache.");
            }
            else {
                ESP_LOGE(TAG, "Failed to save AP info to cache.");
            }
        }
}
    else if (status & WIFI_FAIL) {
        ESP_LOGI(TAG, "Failed to connect to SSID: %s", wifi_config.sta.ssid);
        // We don't need to clear the connected bit, because it's cleared when
        // we get the disconnected event in the event handler.

        // But we do clear this bit because we've handled it.
        xEventGroupClearBits(wifi_status, WIFI_FAIL);

        if (current_config.cache_ap_info && cache_is_valid) {
            // If the we failed to connect, then assume our cache is now invalid
            // because we tried to connect to an AP that didn't exist, so any
            // information is obsolete.
            cache_is_valid = false;

            // and, if we were relying on the cached data, then post a message
            // to the queue to try again - which will just kick is back into
            // this function with an invalid cache, but is cleaner than adding a
            // loop.
            wifi_restart();
        }
        // otherwise, we either weren't caching data or the cache wasn't valid,
        // and should just stop trying.
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
    EventBits_t status = xEventGroupGetBits(wifi_status);
    
    // signal that we are in the process of stopping WiFi and shouldn't try to 
    // reconnect, etc.
    xEventGroupSetBits(wifi_status, WIFI_STOPPING);

    if (status & WIFI_CONNECTED) {
        ESP_ERROR_CHECK(esp_wifi_disconnect());
    }
    if (status & WIFI_STARTED) {
        ESP_ERROR_CHECK(esp_wifi_stop());
        // We don't deinit the WiFi, because it's not necessary - the config is
        // set in connect_wifi, right before esp_wifi_start.
        xEventGroupClearBits(wifi_status, WIFI_STARTED);
    }

    // WiFi is now off.
    xEventGroupSetBits(wifi_status, WIFI_OFF);
    // Since it is off, we are no longer stopping.
    xEventGroupClearBits(wifi_status, WIFI_STOPPING);
}

/**
 * Handler to handle replies from the CoAP server.
 * 
 * @param ctx context
 * @param session session
 * @param sent PDU sent
 * @param received PDU received
 * @param id message id
 */
static void coap_message_handler(coap_context_t *ctx, coap_session_t *session,
                                 coap_pdu_t *sent, coap_pdu_t *received,
                                 const coap_tid_t id)
{
    int class = COAP_RESPONSE_CLASS(received->code);
    // This doesn't actually get the code bits, it just returns the full code.
    // int code = COAP_RESPONSE_CODE(received->code);
    int code = received->code & 0x1F;

    // success is 2.05 which (PUT).(CONTENT). Anything else is an error.
    if (class == 2 && code == 5) {
        xEventGroupSetBits(wifi_status, COAP_SUCCESSFUL);
    }
    else {
        ESP_LOGE(TAG, "Received code %d.%02d back from the CoAP server.", class, code);
    }

    xEventGroupSetBits(wifi_status, COAP_QUEUE_EMPTY);
}

/**
 * Parse the coap server URI in the config into a given coap_uri_t struct,
 * applying some validity checks along the way.
 *
 * @param uri URI structure into which all the URI details should be placed.
 * 
 * @return true if the parsing was successful and the URI should work.
 * @return false if the parsing was unsuccessful or the URI won't work.
*/
static bool parse_coap_server(coap_uri_t *uri)
{
    if (coap_split_uri((const uint8_t *)current_config.uri, strlen(current_config.uri), uri) != 0) {
        ESP_LOGE(TAG, "CoAP server uri error");
        return false;
    }

    if ((uri->scheme==COAP_URI_SCHEME_COAPS && !coap_dtls_is_supported()) ||
        (uri->scheme==COAP_URI_SCHEME_COAPS_TCP && !coap_tls_is_supported())) {
        ESP_LOGE(TAG, "CoAP server uri scheme error");
        return false;
    }

    return true;
}

/**
 * Send the temperature via CoAP.
 *
 * @return true if the packet was successfully sent
 * @return false if the packet was not successfully sent
 */
static bool coap_send_temperature(void)
{
    /* Temporary data buffer.
     *
     * This is of the format:
     *
     * <station name>: <temperature>
     *
     * Station name is MAX_STATION_NAME_LEN.
     * 
     * There's a colon and a space, which is +2.
     *
     * The temperature range is -55 to 125C (-67 to 257F), which is 3
     * characters. We also have a decimal point and another place, for a total
     * of 5 characters, and then a NUL, for 6, total. */
    char buffer[MAX_STATION_NAME_LEN + 2 + 6];

    coap_uri_t uri;
    char hostname[MAX_URI_LEN + 1];
    struct addrinfo *ainfo;
    coap_address_t dst_addr;
    coap_context_t *ctx = NULL;
    coap_session_t *session = NULL;
    coap_pdu_t *request = NULL;
    bool success = false;
    int result;

    if (!parse_coap_server(&uri)) {
        ESP_LOGE(TAG, "parse_coap_server failed");
    }
    else {
        // format our temperature
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), "%s: %.1f", current_config.station_name, get_last_temp());
        
        // This copy is necessary because the uri.host.s element is just a
        // pointer into to the URI array - not a NUL terminated string. We
        // don't want to change it, but getaddrinfo needs a NUL terminated
        // hostname / IP. So, copy it out and NUL terminate it so
        // getaddrinfo is content.
        memcpy(hostname, uri.host.s, uri.host.length);
        hostname[uri.host.length] = '\0';

        result = getaddrinfo(hostname, NULL, NULL, &ainfo);
        if (result != 0) {
            ESP_LOGE(TAG, "getaddrinfo failed: %d", result);
        }
        else {
            ctx = coap_new_context(NULL);
            if (!ctx) {
                ESP_LOGE(TAG, "coap_new_context() failed");
            } else {
                coap_address_init(&dst_addr);
                dst_addr.size = ainfo->ai_addrlen;
                memcpy(&dst_addr.addr, ainfo->ai_addr, ainfo->ai_addrlen);
                if (ainfo->ai_family == AF_INET6) {
                    dst_addr.addr.sin6.sin6_family = AF_INET6;
                    dst_addr.addr.sin6.sin6_port   = htons(uri.port);
                } else {
                    dst_addr.addr.sin.sin_family   = AF_INET;
                    dst_addr.addr.sin.sin_port     = htons(uri.port);
                }

                session = coap_new_client_session(ctx, NULL, &dst_addr,
                    uri.scheme==COAP_URI_SCHEME_COAP_TCP ? COAP_PROTO_TCP :
                    uri.scheme==COAP_URI_SCHEME_COAPS_TCP ? COAP_PROTO_TLS :
                    uri.scheme==COAP_URI_SCHEME_COAPS ? COAP_PROTO_DTLS : COAP_PROTO_UDP);
                if (!session) {
                    ESP_LOGE(TAG, "coap_new_client_session() failed");
                }
                else {
                    coap_register_response_handler(ctx, coap_message_handler);

                    request = coap_new_pdu(session);
                    if (!request) {
                        ESP_LOGE(TAG, "coap_new_pdu() failed");
                    }
                    else {
                        request->type = COAP_MESSAGE_CON;
                        request->tid = coap_new_message_id(session);
                        request->code = COAP_REQUEST_PUT;

                        coap_add_option(request, COAP_OPTION_URI_PATH,
                                        uri.path.length, uri.path.s);
                        
                        coap_add_data(request, strlen(buffer), (uint8_t *)buffer);

                        xEventGroupClearBits(wifi_status, COAP_SUCCESSFUL|COAP_QUEUE_EMPTY);

                        // coap_send deletes the request when finished
                        coap_send(session, request);

                        // this runs the coap network I/O; return value is how
                        // long it took, or -1 if error.
                        result = coap_run_once(ctx, COAP_TIMEOUT_S * 1000);
                        if (result < 0) {
                            ESP_LOGE(TAG, "coap_run_once returned %d", result);
                        }
                        else {
                            ESP_LOGI(TAG, "sending the COAP message took %d ms", result);
                        }

                        // success is only true if we got a successful return
                        // code
                        if (xEventGroupGetBits(wifi_status) & COAP_SUCCESSFUL) {
                            success = true;
                        }
                    }

                    coap_session_release(session);
                }
                coap_free_context(ctx);            
            }
        }

        coap_cleanup();
        freeaddrinfo(ainfo);
    }
    
    return success;
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
                    // TODO: Add some blinkenlights feedback here?
                    if (is_config_valid(&current_config)) {
                        ESP_LOGI(TAG, "wifi config looks valid, connecting");
                        connect_wifi();
                    }
                    else {
                        ESP_LOGE(TAG, "invalid config, not connecting to wifi");
                    }
                    break;
                case WIFI_STOP:
                    // No extraneous checks here, because the disconnect WiFi
                    // function checks extensively to not double-free something.
                    disconnect_wifi();
                    break;
                case WIFI_SEND_TEMP:
                    if (coap_send_temperature()) {
                        ESP_LOGI(TAG, "Temperature sent successfully");
                    }
                    else {
                        ESP_LOGE(TAG, "Temperature sending failed");
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
 * Send last temperature reading.
 */
void wifi_send_temperature(void)
{
    uint8_t message = WIFI_SEND_TEMP;

    xQueueSend(wifi_queue, &message, portMAX_DELAY);
}

void start_wifi(void)
{
    xTaskCreate(wifi_task, "wifi", 5 * 1024, NULL, WIFI_TASK_PRIORITY, NULL);
}

bool wait_for_wifi_connected(void)
{
    EventBits_t bits;

    bits = xEventGroupWaitBits(
            wifi_status,
            WIFI_CONNECTED,
            pdFALSE,
            pdFALSE,
            WIFI_CONNECT_WAIT_TIMEOUT_S * 1000 / portTICK_PERIOD_MS);

    if (bits & WIFI_CONNECTED) {
        // bit set
        return true;
    }
    else {
        // timeout
        return false;
    }
}

bool wait_for_wifi_off(void)
{
    EventBits_t bits;

    bits = xEventGroupWaitBits(
            wifi_status,
            WIFI_OFF,
            pdFALSE,
            pdFALSE,
            WIFI_DOWN_WAIT_TIMEOUT_S * 1000 / portTICK_PERIOD_MS);

    if (bits & WIFI_OFF) {
        // bit set
        return true;
    }
    else {
        // timeout
        return false;
    }
}

bool wait_for_sending_complete(void)
{
    EventBits_t bits;

    bits = xEventGroupWaitBits(
             wifi_status,
             COAP_QUEUE_EMPTY,
             pdFALSE,
             pdFALSE,
             COAP_TIMEOUT_S * 1000 / portTICK_PERIOD_MS);
 
    if (bits & COAP_QUEUE_EMPTY) {
        // bit set
        return true;
    }
    return false;
}
