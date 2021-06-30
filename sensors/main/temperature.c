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

    // Measure and read
    ret = ds18b20_measure_and_read(SENSOR_GPIO, DS18X20_ANY, temperature);
    if (ret == ESP_OK) {
        // We have good data, set success.
        success = true;
    }
    else {
        ESP_LOGE(TAG, "Error reading temperature: %s",
                 esp_err_to_name(ret));
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
