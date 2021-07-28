/**
 * @file
 * Header for wifi.c.
 */

#ifndef __CMD_WIFI_H_
#define __CMD_WIFI_H_

enum wifi_messages {
    WIFI_START,
    WIFI_STOP
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

#endif // __CMD_WIFI_H_
