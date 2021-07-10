/**
 * @file
 * Temperature related functions.
 */

#include "esp_log.h"
#include <driver/gpio.h>
#include <ds18x20.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "temperature.h"

static const char *TAG = "temperature";

#define SENSOR_GPIO GPIO_NUM_12

// From the datasheet, with R0 and R1 both 0, the 9 bit resolution takes
// 93.75ms to do the conversion.
#define MEASUREMENT_DELAY_MS 94

/**
 * Turn on our sensor.
 */
static void sensor_on(void)
{
    /* Set pullup mode on comms pin so the bus works.
     * The internal pullup violates the datasheet recommendations,
     * but works, and saves us an external resistor.
     */
    gpio_set_pull_mode(SENSOR_GPIO, GPIO_PULLUP_ONLY);
}

/**
 * Turn off our sensor.
 */
static void sensor_off(void)
{
    /* We've now gotten the temperature, turn off the bus line, and
     * pull the bus line down avoid current leakage across the pullup.
     */
    gpio_set_level(SENSOR_GPIO, 0);
    gpio_set_pull_mode(SENSOR_GPIO, GPIO_PULLDOWN_ONLY);
}

bool read_temperature(float *temperature)
{
    esp_err_t ret;
    bool success = false;
    int count;

    sensor_on();

    // Start measuring. false here means don't wait.
    ret = ds18x20_measure(SENSOR_GPIO, DS18X20_ANY, false);
    count = 0;
    // occasionally we'll fail to start the conversion, so retry until
    // it succeeds or we give up.
    while (ret != ESP_OK && count < 5) {
        ret = ds18x20_measure(SENSOR_GPIO, DS18X20_ANY, false);
        ++count;
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error starting measurement: %s", esp_err_to_name(ret));
    }
    else {
        // wait for measurement to happen
        vTaskDelay(MEASUREMENT_DELAY_MS / portTICK_PERIOD_MS);

        // Read it back.
        ret = ds18b20_read_temperature(SENSOR_GPIO, DS18X20_ANY, temperature);
        count = 0;
        // occasionally we'll get corrupt data (detectable via bad CRC)
        // so retry the read until it succeeds or we give up.
        while (ret != ESP_OK && count < 5) {
            ret = ds18b20_read_temperature(
                      SENSOR_GPIO, DS18X20_ANY, temperature);
            ++count;
        }
        if (ret == ESP_OK) {
            // We have good data, set success.
            success = true;
        }
        else {
            ESP_LOGE(TAG, "Error reading temperature: %s",
                     esp_err_to_name(ret));
        }
    }

    sensor_off();

    return success;
}

bool check_and_fix_18b20_configuration(void)
{
    // scratchpad holds 8 bytes of data
    uint8_t scratchpad[8] = {};

    bool success = false;
    esp_err_t ret;

    sensor_on();

    ret = ds18x20_read_scratchpad(SENSOR_GPIO, DS18X20_ANY, scratchpad);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error reading scratchpad: %s",
                 esp_err_to_name(ret));
    }
    else {
        /* The config register is byte index 4, and we want to turn off the
         * high 3 bits and leave the others alone.
         * Note that, strictly speaking, the DS18B20 only cares about bits 5
         * and 6 - bit 7 is always zero. But, some other sensors have that
         * high bit and can use it for other resolutions. But, clearing it
         * should result in the standard behavior and not bother the actual
         * DS18B20.
         */
        if ((scratchpad[4] & 0xE0) == 0) {
            ESP_LOGI(TAG, "Temperature sensor config is correct.");
        }
        else {
            scratchpad[4] = scratchpad[4] & 0x1F;

            ESP_LOGI(TAG,
                     "Temperature sensor config is incorrect - rewriting.");

            // We only write bytes 2-4, to the scratchpad.
            ret = ds18x20_write_scratchpad(SENSOR_GPIO,
                                           DS18X20_ANY,
                                           &scratchpad[2]);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Error writing scratchpad: %s",
                         esp_err_to_name(ret));
            }
            else {
                // And then save it once it's written.
                ret = ds18x20_copy_scratchpad(SENSOR_GPIO, DS18X20_ANY);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Error copying scratchpad: %s",
                             esp_err_to_name(ret));
                }
                else {
                    success = true;
                }
            }
        }
    }

    sensor_off();

    return success;
}