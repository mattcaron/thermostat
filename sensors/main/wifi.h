/**
 * @file
 * Header for wifi.c.
 */

#ifndef __WIFI_H_
#define __WIFI_H_

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
 */
void wait_for_mqtt_subscribed(void);

/**
 * Wait for the MQTT queue to be empty.
 */
void wait_for_mqtt_queue_empty(void);

/**
 * Wait for WiFi to be turned off.
 */
void wait_for_wifi_off(void);

#endif // __WIFI_H_