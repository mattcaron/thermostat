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
#include "driver/gpio.h"

#include "esp_system.h"
#include "console.h"
#include "cmd_system.h"
#include "config_storage.h"
#include "wifi.h"
#include "temperature.h"
#include "queues.h"

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
 * Initalize our sensor GPIOs.
 *
 * For starters, all of them are set to outputs and turned off. We'll turn
 * the bus on when we need it and power it via parasite mode.
 */
void init_sensor_gpios(void)
{
    gpio_config_t gpios = {
        .pin_bit_mask = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14,
        .mode = GPIO_MODE_DEF_OUTPUT,
    };

    ESP_ERROR_CHECK(gpio_config(&gpios));

    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_12, 0));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_13, 0));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_14, 0));
}

/**
 * Main
 */
void app_main(void)
{
    // First thing, init our sensor GPIOs and drive to ground.
    init_sensor_gpios();

    // And then we create all our queues so they're set up for our tasks to
    // use once they are started.
    if (!create_queues()) {
        ESP_LOGE(TAG, "Error creating queues. We're screwed.");
        esp_restart();
    }

    // And get our nonvolatile storage set up.
    initialize_nvs();

    // Init our wifi task. It doesn't do anything until it gets a message, so
    // it's safe.
    start_wifi();

    // read config pre-zeroes the structure passed in, so there's no explicit
    // need to zero current_config on boot.
    if (read_config_from_nvs(&current_config)) {

        // FIXME - this should be forked off into a background task so it can
        // run in the background while the console starts in the foreground.
        wifi_enable();
    }
    else {
        ESP_LOGE(TAG, "Error reading NVS config - no configuration applied.");
    }

    // Check and fix our sensor config.
    check_and_fix_18b20_configuration();

    start_console();
}
