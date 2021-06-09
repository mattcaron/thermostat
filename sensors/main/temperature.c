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

bool read_temperature(float *temperature)
{
    /* TODO: this whole function could stand improvement (mainly because the
     * library could stand improvement):
     *
     * 1. If it had a family specific read function, we could save a call to
     *    the bus scan function.
     * 2. If we wrote the configuration register (which we'd have to do every
     *    time because we keep powering it off), we could lower the precision
     *    to 9 bits (0.5°C resolution) and cut the conversion time from 750ms
     *    to 94. So, even with the config write time, it should save power.
     *
     * Correcting the above will likely mean making an 18B20 specific library
     * of my own which uses the esp-idf-onewire library for comms.
     */


    ds18x20_addr_t address;
    size_t sensor_count = 0;
    esp_err_t ret;
    bool success = false;

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

    /* Due to an eccentricity in the library, it uses the address to
     * determine the chip family in order to determine which scratchpad
     * data to use for the temperature. As a result, you need the actual
     * address of our specific device. So, even though we only plan to ever
     * have one device in this design, we can't just use DS18X20_ANY.
     */
    ret = ds18x20_scan_devices(SENSOR_GPIO, &address, 1, &sensor_count);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error scanning bus: %s", esp_err_to_name(ret));
    }
    else {
        // Measure and read
        ret = ds18x20_measure_and_read(SENSOR_GPIO, address, temperature);
        if (ret == ESP_OK) {
            // We have good data, set success.
            success = true;
        }
        else {
            ESP_LOGE(TAG, "Error reading temperature: %s",
                     esp_err_to_name(ret));
        }
    }

    /* We've now gotten the temperature, turn off the power line, turn off
     * the bus line, and pull the bus line down avoid current leakage across
     * the pullup.
     */
    gpio_set_level(POWER_GPIO, 0);
    gpio_set_level(SENSOR_GPIO, 0);
    gpio_set_pull_mode(SENSOR_GPIO, GPIO_PULLDOWN_ONLY);

    return success;
}
