/**
 * @file
 * Functions to read and store our AP info between boots.
 */
#include <string.h> // for memset
#include "ap_cache_storage.h"
#include "nvs.h"

#define NVS_CACHE_NAMESPACE "mc-ap-cache"

#define NVS_CHANNEL_NAME "channel"
#define NVS_BSSID_NAME "bssid"

ap_cache_storage_t ap_cache;

bool read_ap_cache_from_nvs(void)
{
    nvs_handle handle;
    esp_err_t ret;
    bool success = false;

    // zero our structure.
    memset(&ap_cache, 0, sizeof(ap_cache_storage_t));

    ret = nvs_open(NVS_CACHE_NAMESPACE, NVS_READONLY, &handle);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        /* The first time we open this, it can fail if the namespace doesn't
         * exist. It would create it, except we opened it read only. So, open it
         * read write, just once. But, since it is newly created, we can skip
         * the rest of the logic, since we haven't stored anything yet. Since we
         * zero the config structure above, very little further work is
         * necessary, just an open and close.
         */
        ESP_ERROR_CHECK(nvs_open(NVS_CACHE_NAMESPACE, NVS_READWRITE, &handle));
    }
    else {
        // Catch any other errors not related to it not being there.
        ESP_ERROR_CHECK(ret);

        size_t length = sizeof(ap_cache.bssid);

        // Get channel
        ret = nvs_get_u8(handle, NVS_CHANNEL_NAME, &ap_cache.channel);
        if (ret == ESP_OK) {
            // Get BSSID
             ret = nvs_get_blob(handle, NVS_BSSID_NAME, ap_cache.bssid, &length);
             if (ret == ESP_OK && length == sizeof(ap_cache.bssid)) {
                success = true;
             }
        }
    }
        
    nvs_close(handle);

    return success;
}

bool write_ap_cache_to_nvs(void)
{
    nvs_handle handle;
    bool success = false;

    if(nvs_open(NVS_CACHE_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK) {
        // Write out our config items.
        if(nvs_set_u8(handle, NVS_CHANNEL_NAME, ap_cache.channel) == ESP_OK &&
           nvs_set_blob(handle, NVS_BSSID_NAME, ap_cache.bssid, sizeof(ap_cache.bssid)) == ESP_OK) {
            
            if(nvs_commit(handle) == ESP_OK) {
                success = true;
            } // else nvs_commit failed
        } // else nvs_set_* failed
    
        nvs_close(handle);
    } // else nvs_open failed

    return success;
}