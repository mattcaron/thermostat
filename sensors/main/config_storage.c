/**
 * @file
 * Functions to read and store our config.
 */
#include <string.h>
#include <stdlib.h>
#include "config_storage.h"
#include "nvs.h"

/* "backup_mac", "phy", and "dhcp_state" are all used by the system.
 * Ensure that this namespace declaration does not conflict with them
 * and is random enough so as to not conflict with anything which might
 * be used in the future.
 */
#define NVS_CONFIG_NAMESPACE "mc-sensorconfig"

#define NVS_SSID_NAME "ssid"
#define NVS_PASS_NAME "pass"
#define NVS_STATION_NAME "sta"

config_storage_t current_config;

config_storage_t new_config;

/**
 * Helper function to read a config string from NVS.
 *
 * @param handle [in]  handle to open NVS partition.
 * @param key    [in]  key whose value to retrieve.
 * @param value  [out] buffer to receive the value.
 * @param buflen [out] length of value, in bytes.
 *
 * @return an esp_err_t result as returned from nvs_get_str.
 */
static esp_err_t read_config_string_from_nvs(nvs_handle handle, const char *key,
        char *value, size_t buflen)
{
    // API docs say length includes NUL, so we don't want to subtract 1 in all
    // of these calculations.
    size_t length = buflen;
    esp_err_t ret = nvs_get_str(handle, key, value, &length);
    // It not being there is not an error - just ignore it and leave it blank.
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ret = ESP_OK;
    }

    return ret;
}

bool read_config_from_nvs(config_storage_t *config)
{
    nvs_handle handle;
    esp_err_t ret;

    // zero our structure.
    memset(config, 0, sizeof(config_storage_t));

    ret = nvs_open(NVS_CONFIG_NAMESPACE, NVS_READONLY, &handle);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        /* The first time we open this, it can fail if the namespace
         * doesn't exist. It would create it, except we opened it read only.
         * So, open it read write, just once. But, since it is newly created,
         * we can skip the rest of the logic, since we haven't stored anything
         * yet. Since we zero the config structure above, no further work is
         * necessary, just an open and close.
         */
        ESP_ERROR_CHECK(nvs_open(NVS_CONFIG_NAMESPACE, NVS_READWRITE, &handle));
    }
    else {
        // Catch any other errors not related to it not being there.
        ESP_ERROR_CHECK(ret);

        // Read out the rest of our config items.
        ESP_ERROR_CHECK(read_config_string_from_nvs(handle, NVS_SSID_NAME,
                        config->ssid, sizeof(config->ssid)));
        ESP_ERROR_CHECK(read_config_string_from_nvs(handle, NVS_PASS_NAME,
                        config->pass, sizeof(config->pass)));
        ESP_ERROR_CHECK(read_config_string_from_nvs(handle, NVS_STATION_NAME,
                        config->station_name, sizeof(config->station_name)));
    }

    nvs_close(handle);

    return true;
}

bool write_config_to_nvs(config_storage_t *config)
{
    nvs_handle handle;
    esp_err_t ret;

    ret = nvs_open(NVS_CONFIG_NAMESPACE, NVS_READWRITE, &handle);

    ESP_ERROR_CHECK(ret);

    // Write out our config items.
    ESP_ERROR_CHECK(nvs_set_str(handle, NVS_SSID_NAME,
                                config->ssid));
    ESP_ERROR_CHECK(nvs_set_str(handle, NVS_PASS_NAME,
                                config->pass));
    ESP_ERROR_CHECK(nvs_set_str(handle, NVS_STATION_NAME,
                                config->station_name));

    nvs_commit(handle);

    nvs_close(handle);

    return true;
}