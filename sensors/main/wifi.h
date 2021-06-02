/**
 * @file
 * Header for wifi.c.
 */

#ifndef __CMD_WIFI_H_
#define __CMD_WIFI_H_

/**
 * Start wifi.
 *
 * @param current_config config to apply.
 */
void start_wifi(config_storage_t *current_config);

/**
 * Stop wifi.
 */
void stop_wifi(void);

#endif // __CMD_WIFI_H_
