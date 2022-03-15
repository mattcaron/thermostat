/**
 * @file
 * Header for wifi.c.
 */

#ifndef __WIFI_H_
#define __WIFI_H_

/**
 * How long to wait for MQTT to be subscribed.
 * This includes the time to connect to WiFi.
 * In my system, this takes less than 10 seconds, so a 30 second timeout seems
 * reasonable.
 */
#define MQTT_CONNECT_WAIT_TIMEOUT_S 30

/**
 * How long to wait for our MQTT message to be successfully sent.
 * This takes about a second on my system, so a 5 second timeout seems
 * reasonable.
 */
#define MQTT_SEND_WAIT_TIMEOUT_S 5

/**
 * How long to wait for WiFi to come down.
 *
 * This should be nearly instantaneous, so 5 seconds is more than enough.
 */
#define WIFI_DOWN_WAIT_TIMEOUT_S 5

enum wifi_messages {
    WIFI_START,
    WIFI_STOP,
    WIFI_SEND_MQTT
};

/**
 * Start our WiFi task.
 */
void start_wifi(void);

/**
 * Enable WiFi.
 *
 * @note Only call once current_config is valid.
 */
void wifi_enable(void);

/**
 * Disable WiFi.
 */
void wifi_disable(void);

/**
 * Disable then enable WiFi.
 *
 * @note Only call once current_config is valid.
 */
void wifi_restart(void);

/**
 * Send last temperature reading over MQTT.
 */
void wifi_send_mqtt_temperature(void);

/**
 * Print various MQTT items.
 *
 * Unlike the WiFi stack, the MQTT client doesn't have a "get_info" type
 * function, so fake it with our config and internal status.
 */
void emit_mqtt_status(void);

/**
 * Wait for MQTT to be successfully subscribed.
 *
 * @return true when MQTT is subscribed
 * @return false on timeout
 */
bool wait_for_mqtt_subscribed(void);

/**
 * Wait for the MQTT queue to be empty.
 *
 * @return true when MQTT queue is empty
 * @return false on timeout
 */
bool wait_for_mqtt_queue_empty(void);

/**
 * Wait for WiFi to be turned off.
 *
 * @return true when WiFi is off
 * @return false on timeout
 */
bool wait_for_wifi_off(void);

#endif // __WIFI_H_