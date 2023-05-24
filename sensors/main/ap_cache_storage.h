/**
 * @file
 * Header file for ap_cache_storage.c.
 */

#ifndef __AP_CACHE_STORAGE_H_
#define __AP_CACHE_STORAGE_H_

#include <stdint.h>
#include "esp_wifi.h" // for wifi_ap_record_t

/**
 * AP cache storage structure.
 */
typedef struct {
    uint8_t channel; /**< Channel */
    uint8_t bssid[sizeof(((wifi_ap_record_t *)0)->bssid)];  /**< BSSID */
} ap_cache_storage_t;

extern ap_cache_storage_t ap_cache;

/**
 * Read AP cache from NVS.
 *
 * @return true on success.
 * @return false on failure.
 */
bool read_ap_cache_from_nvs(void);

/**
 * Write AP cache to NVS.
 *
 * @return true on success.
 * @return false on failure.
 */
bool write_ap_cache_to_nvs(void);

#endif // __AP_CACHE_STORAGE_H_