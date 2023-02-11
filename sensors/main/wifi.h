/**
 * @file
 * Header for wifi.c.
 */

#ifndef __WIFI_H_
#define __WIFI_H_

/**
 * How long to wait for WiFi to be connected
 * In my system, this takes less than 5 seconds, so a 10 second timeout seems
 * reasonable.
 */
#define WIFI_CONNECT_WAIT_TIMEOUT_S 10

/**
 * How long to wait for WiFi to come down.
 *
 * This should be nearly instantaneous, so 5 seconds is more than enough.
 */
#define WIFI_DOWN_WAIT_TIMEOUT_S 5

enum wifi_messages {
    WIFI_START,
    WIFI_STOP,
    WIFI_SEND_TEMP,
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
 * Send last temperature reading.
 */
void wifi_send_temperature(void);

/**
 * Wait for WiFi to be connected
 *
 * @return true when WiFi is connected
 * @return false on timeout
 */
bool wait_for_wifi_connected(void);

/**
 * Wait for WiFi to be turned off.
 *
 * @return true when WiFi is off
 * @return false on timeout
 */
bool wait_for_wifi_off(void);

/**
 * Wait for our message ot be sent.
 * 
 * @return true if the message was sent.
 * @return false if we timed out.
 */
bool wait_for_sending_complete(void);

#endif // __WIFI_H_