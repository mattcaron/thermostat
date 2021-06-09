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
    printf("  show the current WiFi status.\n");
    printf("\n");
}

/**
 * Print various WiFi items.
 */
static void emit_wifi(void)
{
    esp_err_t err;
    wifi_ap_record_t ap_info;

    printf("WiFi status:\t");

    err = esp_wifi_sta_get_ap_info(&ap_info);
    if (err == ESP_ERR_WIFI_CONN) {
        printf("not initialized.\n");
    }
    else if (err == ESP_ERR_WIFI_NOT_CONNECT) {
        printf("not connected.\n");
    }
    else if (err == ESP_OK) {
        printf("connected\n");

        printf("Station information:\n");
        printf("\tSSID:\t%s\n", ap_info.ssid);
        printf("\tChan:\t%d\n", ap_info.primary);
        // TODO - add secondary channel
        printf("\t Sig:\t%d dBm\n", ap_info.rssi);
        // TODO - add authmode and cipher stringify and print
        // TODO - add wifi mode bit stringify and print
        // TODO - add ability to show IP and subnet.
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