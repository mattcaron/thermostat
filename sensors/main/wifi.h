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

#endif // __WIFI_H_
