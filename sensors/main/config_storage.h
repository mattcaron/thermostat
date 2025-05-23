/**
 * @file
 * Header file for config_storage.c.
 */

#ifndef __CONFIG_STORAGE_H_
#define __CONFIG_STORAGE_H_

#include "esp_wifi_types.h"
#include "esp_netif.h"

/** Maximum station name length; determined by TCPIP hostname max size. */
#define MAX_STATION_NAME_LEN (TCPIP_HOSTNAME_MAX_SIZE - 1)

/** Maximum URI length. */
#define MAX_URI_LEN 512

/** Maximum IPv4 string length - 15 chars + nul - "255.255.255.255\0" */
#define MAX_IPV4_LEN 16

/** MAC address length in bytes (not the string) */
#define MAC_ADDR_LEN 6

/**
 * Configuration storage structure.
 */
typedef struct {
    char ssid[MAX_SSID_LEN+1];              /**< SSID */
    char pass[MAX_PASSPHRASE_LEN+1];        /**< Password for above. */
    bool cache_ap_info;                     /**< Store the detected WiFi
                                                 access point into to NVS
                                                 memory. */
    bool use_dhcp;                          /**< Use DHCP. */
    ip4_addr_t ipaddr;                      /**< IP address. */
    ip4_addr_t netmask;                     /**< Netmask. */
    ip4_addr_t gateway;                     /**< Gateway. */
    ip4_addr_t dns;                         /**< DNS server. */
    char station_name[MAX_STATION_NAME_LEN+1]; /**< The name of this station. */
    bool use_celsius;                       /**< Whether to use Celsius or
                                                 Farenheit */
    uint16_t poll_time_sec;                 /**< Poll time in seconds. */
    char uri[MAX_URI_LEN+1];                /**< URI to which we should
                                                 publish. */
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
 * @param [in] config Reference to configuration structure which should be
 *                    written to NVS.
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
