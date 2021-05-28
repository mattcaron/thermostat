/**
 * @file
 * Various configuration console commands.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "config_storage.h"

static const char *TAG = "cmd_config";

static void register_config(void);

void register_configure(void)
{
    register_config();
}

static int tasks_config(int argc, char **argv)
{
    config_storage_t config;

    if(read_config_from_nvs(&config)) {
        printf("Config:\n");
        printf("\tStation: \t%s\n", config.station_name);
        printf("\tSSID:    \t%s\n", config.ssid);
        printf("\tPassword:\t%s\n", config.pass);
    }
    else {
        printf("Error reading config\n");
        return 1;
    }

    return 0;
}

static void register_config(void)
{
    const esp_console_cmd_t cmd = {
        .command = "config",
        .help = "Configuration operations",
        .hint = NULL,
        .func = &tasks_config,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

