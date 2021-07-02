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
#define POWER_GPIO GPIO_NUM_14

// 750ms for now, until I get real DS18B20s which actually save their
// EEPROM properly.
#define MEASUREMENT_DELAY_MS 750
// #define MEASUREMENT_DELAY_MS 94

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

    /* Turn on the power line.
     * Note that I had intended to run this in parasite power mode, except
     * various 1820 compatible sensors require external capacitors in order
     * to work properly. As a result, I am just powering it from a
     * neighboring GPIO.
     */
    gpio_set_level(POWER_GPIO, 1);

}

/**
 * Turn off our sensor.
 */
static void sensor_off(void)
{
    /* We've now gotten the temperature, turn off the power line, turn off
     * the bus line, and pull the bus line down avoid current leakage across
     * the pullup.
     */
    gpio_set_level(POWER_GPIO, 0);
    gpio_set_level(SENSOR_GPIO, 0);
    gpio_set_pull_mode(SENSOR_GPIO, GPIO_PULLDOWN_ONLY);
}

bool read_temperature(float *temperature)
{
    /* TODO: this whole function could stand improvement (mainly because the
     * library could stand improvement):
     *
     * 2. If we wrote the configuration register (which we'd have to do every
     *    time because we keep powering it off), we could lower the precision
     *    to 9 bits (0.5°C resolution) and cut the conversion time from 750ms
     *    to 94. So, even with the config write time, it should save power.
     *     ^^^ - LIES!!!! There is an EEPROM. We can check it on boot and, if
     *           it's wrong, reconfigure it and save it. Done.
     *
     *           Of course, none of this functionality is actually written
     *           yet, so I'll need to add it.
     *
     * Correcting the above will likely mean making an 18B20 specific library
     * of my own which uses the esp-idf-onewire library for comms.
     *     ^^^ - More LIES!!!! I'm just hacking up the ds18x20 driver to add
     *           what I want.
     *
     */

    esp_err_t ret;
    bool success = false;

    sensor_on();

    // Start measuring. false here means don't wait.
    ret = ds18x20_measure(SENSOR_GPIO, DS18X20_ANY, false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error starting measurement: %s", esp_err_to_name(ret));
    }
    else {
        // wait for measurement to happen
        vTaskDelay(MEASUREMENT_DELAY_MS / portTICK_PERIOD_MS);

        // Read it back.
        ret = ds18b20_read_temperature(SENSOR_GPIO, DS18X20_ANY, temperature);
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
        ESP_LOGI(TAG, "Config is 0x%X", scratchpad[4]);
        /* The config register is byte index 4, and we want to turn off the
         * high 3 bits and leave the others alone.
         * Note that, strictly speaking, the DS18B20 only cares about bits 5
         * and 6 - bit 7 is always zero. But, some other sensors have that
         * high bit and can use it for other resolutions. But, clearing it
         * should result in the standard behavior and not bother the actual
         * DS18B20.
         */
        if ((scratchpad[4] & 0xE0) != 0) {
            scratchpad[4] = scratchpad[4] & 0x1F;

            ESP_LOGI(TAG, "Rewriting config.");

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