/**
 * @file
 * Header file for config_storage.c.
 */

#ifndef __CONFIG_STORAGE_H_
#define __CONFIG_STORAGE_H_

#include "esp_wifi_types.h"

/** Maximum station name length. */
#define MAX_STATION_NAME_LEN 64

/** Maximum MQTT URI length. */
#define MAX_MQTT_URI_LEN 256

/** Maximum MQTT topic length. */
#define MAX_MQTT_TOPIC_LEN 64

/**
 * Configuration storage structure.
 */
typedef struct {
    char ssid[MAX_SSID_LEN+1];              /**< SSID */
    char pass[MAX_PASSPHRASE_LEN+1];        /**< Password for above. */
    char station_name[MAX_STATION_NAME_LEN+1]; /**< The name of this station. */
    bool use_celsius;                       /**< Whether to use Celsius or
                                                 Farenheit */
    uint16_t poll_time_sec;                 /**< Poll time in seconds. */
    char mqtt_uri[MAX_MQTT_URI_LEN+1];      /**< MQTT uri to which we should
                                                 subscribe. */
    char mqtt_topic[MAX_MQTT_TOPIC_LEN+1];  /**< MQTT Topic to which we should
                                                 publish our temp data. */
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

/**
 * Basic configuration check.
 *
 * This is a basic cursory check - it will not detect, for example, a malformed
 * URL, but it will detect an empty string.
 *
 * @param config [in] configuration to check.
 *
 * @return true if config is valid.
 * @return false if config is invalid.
 */
bool is_config_valid(config_storage_t *config);

#endif // __CONFIG_STORAGE_H_
