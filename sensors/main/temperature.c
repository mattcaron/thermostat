/**
 * @file
 * Temperature related functions.
 */

#include "esp_log.h"
#include <driver/gpio.h>
#include <ds18x20.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "priorities.h"
#include "temperature.h"
#include "config_storage.h"
#include "wifi.h"

static const char *TAG = "temperature";

#define SENSOR_GPIO GPIO_NUM_12

// From the datasheet, with R0 and R1 both 0, the 9 bit resolution takes
// 93.75ms to do the conversion.
#define MEASUREMENT_DELAY_MS 94

// The last temperature we read.
float last_temp = 0;

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

/**
 * Read the temperature.
 *
 * @param temperature [out] pointer to variable to receive the
 *                          temperature. Only valid if the function
 *                          returns true.
 *
 * @return true if the temperature read succeeded.
 * @return false if the temperature read succeeded.
 */
static bool read_temperature(float *temperature)
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

/**
 * Check and (optionally) fix the configuration.
 *
 * @return true the configuration was or is now correct.
 * @return false if there was any sort of error.
 */
static bool check_and_fix_18b20_configuration(void)
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

float c_to_f(float celsius)
{
    return celsius * 1.8 + 32;
}

/**
 * Temperatue polling task.
 *
 * @param pvParameters Parameters for the function (unused)
 */
static void temp_task(void *pvParameters)
{
    bool successful_read;
    TickType_t last_wake_time = xTaskGetTickCount();
    TickType_t interval = 0;

    float temp_temp;

    // Check and fix our sensor config.
    check_and_fix_18b20_configuration();

    while (true) {
        successful_read = false;

        while (!successful_read) {
            successful_read = read_temperature(&temp_temp);
            if (successful_read && temp_temp == 85) {
                /* 85 is the power on reset value and sometimes our call to
                 * kick off a temperature conversion silently fails, resulting
                 * in no conversion happening. Since this is for residential
                 * HVAC, if we're ever in that temperature range, no amount of * AC is ever going to save us. So, it's fair to treat this as * out of range and sample again, even though doing so might
                 * put us into an infinite loop if it were ever to get that
                 * warm. I'm willing to take that risk.
                */
                successful_read = false;
            }
        }

        // good temp, update
        last_temp = temp_temp;

        if (current_config.use_celsius) {
            ESP_LOGI(TAG, "Read temp: %.1f°C", last_temp);
        }
        else {
            ESP_LOGI(TAG, "Read temp: %.1f°F\n", c_to_f(last_temp));
        }

        // We've read our temperature, wake up our WiFi task to send it.
        wifi_send_mqtt_temperature();

        // we recalculate this every time through the loop in case the config
        // gets changed.
        interval = (current_config.poll_time_sec * 1000) /
                   portTICK_PERIOD_MS;

        // TODO - replace this with a deep sleep.
        // NOTE: we don't want to go to sleep until the MQTT message is
        // actually sent, so we'll have to check for that before we go to
        // sleep.
        vTaskDelayUntil(&last_wake_time, interval);
    }
}

float get_last_temp(void)
{
    return last_temp;
}

void start_temp_polling(void)
{
    xTaskCreate(temp_task, "temp", 2048, NULL, TEMP_TASK_PRIORITY, NULL);
}
