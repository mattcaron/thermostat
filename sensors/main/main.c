/**
 * @file
 * Various system console commands.
 *
 * Derived from ESP8266_RTOS_SDK example
 * examples/system/console/main/console_example_main.c, which is
 * licensed as Public Domain.
 */

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "console.h"
#include "cmd_system.h"
#include "config_storage.h"
#include "wifi.h"

static const char *TAG = "main";

/**
 * Initialize nonvolatile storage.
 */
static void initialize_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
}

/**
 * Main
 */
void app_main(void)
{
    initialize_nvs();

    // read config pre-zeroes the structure passed in, so there's no explicit
    // need to zero current_config on boot.
    if (read_config_from_nvs(&current_config)) {

        // FIXME - this should be forked off into a background task so it can
        // run in the background while the console starts in the foreground.
        start_wifi(&current_config);
    }
    else {
        ESP_LOGE(TAG, "Error reading NVS config - no configuration applied.");
    }

    start_console();
}
