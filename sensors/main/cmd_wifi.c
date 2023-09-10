/**
 * @file
 * Various configuration wifi commands.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_console.h"
#include "esp_wifi.h"
#include "argtable3/argtable3.h"
#include "wifi.h"
#include "config_storage.h"

static const char *TAG = "cmd_wifi";

#define WIFI_HELP_COMMAND "help"
#define WIFI_SHOW_COMMAND "show"

static void register_wifi_cmd(void);

void register_wifi(void)
{
    register_wifi_cmd();
}

/**
 * Print the help for the wifi subcommands.
 */
static void emit_wifi_help(void)
{
    printf("wifi commands:\n\n");
    printf(WIFI_HELP_COMMAND "\n");
    printf("  show this help.\n");
    printf("\n");
    printf(WIFI_SHOW_COMMAND "\n");
    printf("  show the current WiFi and COAP status.\n");
    printf("\n");
}

/**
 * Print various WiFi items.
 */
static void emit_wifi(void)
{
    esp_err_t err;
    wifi_ap_record_t ap_info;
    uint8_t mac[MAC_ADDR_LEN];
    tcpip_adapter_ip_info_t ip_info;
    tcpip_adapter_dns_info_t dns_info;
    char temp_buf[MAX_IPV4_LEN];
    char * ret_buf;
    
    printf("WiFi status:\t");

    err = esp_wifi_sta_get_ap_info(&ap_info);
    if (err == ESP_ERR_WIFI_CONN) {
        printf("not initialized.\n");
    }
    else if (err == ESP_ERR_WIFI_NOT_CONNECT) {
        printf("not connected.\n\n");
        printf("Station information:\n");
        // this should work because it should be initialized now (checked above)
        ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, mac));
        printf("\tMAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    else if (err == ESP_OK) {
        printf("connected\n\n");
        printf("Station information:\n");
        // this should work because it should be initialized now (checked above)
        ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, mac));
        printf("\tMAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        printf("\tSSID:\t%s\n", ap_info.ssid);
        printf("\tChan:\t%d\n", ap_info.primary);
        // TODO - add secondary channel
        printf("\t Sig:\t%d dBm\n", ap_info.rssi);
        // TODO - add authmode and cipher stringify and print
        // TODO - add wifi mode bit stringify and print
        
        ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
        ret_buf = ip4addr_ntoa_r(&ip_info.ip, temp_buf, sizeof(temp_buf));
        if (ret_buf == NULL) {
            ESP_LOGE(TAG, "Buffer too small when formatting IP address for printing");
        }
        else {
            printf("\tIP Address:\t%s\n", ret_buf);
        }

        ret_buf = ip4addr_ntoa_r(&ip_info.netmask, temp_buf, sizeof(temp_buf));
        if (ret_buf == NULL) {
            ESP_LOGE(TAG, "Buffer too small when formatting netmask for printing");
        }
        else {
            printf("\tNetmask:\t%s\n", ret_buf);
        }

        ret_buf = ip4addr_ntoa_r(&ip_info.gw, temp_buf, sizeof(temp_buf));
        if (ret_buf == NULL) {
            ESP_LOGE(TAG, "Buffer too small when formatting gateway for printing");
        }
        else {
            printf("\tGateway:\t%s\n", ret_buf);
        }

        ESP_ERROR_CHECK(tcpip_adapter_get_dns_info(TCPIP_ADAPTER_IF_STA, TCPIP_ADAPTER_DNS_MAIN, &dns_info));
        ret_buf = ip4addr_ntoa_r(&dns_info.ip.u_addr.ip4, temp_buf, sizeof(temp_buf));
        if (ret_buf == NULL) {
            ESP_LOGE(TAG, "Buffer too small when formatting DNS for printing");
        }
        else {
            printf("\tDNS Server:\t%s\n", ret_buf);
        } 
    }
    else {
        // We should never get here.
        // All possible return codes are handled above.
        ESP_ERROR_CHECK(err);
    }
}

/**
 * WiFi tasks - parses commands and executes them appropriately.
 *
 * @param argc [in]  Number of arguments, including the command itself.
 * @param argv [in]  Arguments, including the command itself.
 *
 * @return 0 if success
 * @return 1 if error
 */
static int tasks_wifi(int argc, char **argv)
{
    // assume failure.
    int retval = 1;

    if (argc == 1) {
        // no commands, print help
        emit_wifi_help();
        retval = 0;
    }
    else {
        if (strcmp(argv[1], WIFI_HELP_COMMAND) == 0) {
            emit_wifi_help();
            retval = 0;
        }
        else if (strcmp(argv[1], WIFI_SHOW_COMMAND) == 0) {
            emit_wifi();
            retval = 0;
        }
        else {
            printf("Error: Unknown command: %s\n", argv[1]);
        }
    }

    return retval;
}

/**
 * Register the config command.
 */
static void register_wifi_cmd(void)
{
    const esp_console_cmd_t cmd = {
        .command = "wifi",
        .help = "WiFi operations",
        .hint = NULL,
        .func = &tasks_wifi,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}