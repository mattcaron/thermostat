/**
 * @file
 * Header file for config_storage.c.
 */

#ifndef __CONFIG_STORAGE_H_
#define __CONFIG_STORAGE_H_

#include "esp_wifi_types.h"

/** Maximum station name length. */
#define MAX_STATION_NAME_LEN 64

/**
 * Configuration storage structure.
 */
typedef struct {
    char ssid[MAX_SSID_LEN+1];              /**< SSID */
    char pass[MAX_PASSPHRASE_LEN+1];        /**< Password for above. */
    char station_name[MAX_STATION_NAME_LEN+1]; /**< The name of this station. */
    bool use_celsius;                       /**< Whether to use Celsius or
                                                 Farenheit */
    // TODO: Add MQTT channel(s)
} config_storage_t;

extern config_storage_t current_config;

/**
 * Read a config from NVS.
 * @param [out] config Reference to configuration structure to
 *                     receive current config. Note that this will
 *                     be zeroed before being populated.
 *
 * @return true on success.
 * @return false on failure.
 */
bool read_config_from_nvs(config_storage_t *config);

/**
 * Write a config to NVS.
 * @param [in] config Reference to configuration structure to
 *                     receive current config. Note that this will
 *                     be zeroed before being populated.
 *
 * @return true on success.
 * @return false on failure.
 */
bool write_config_to_nvs(config_storage_t *config);

#endif // __CONFIG_STORAGE_H_