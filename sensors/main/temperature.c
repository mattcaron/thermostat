/**
 * @file
 * Temperature related functions.
 */

#include "esp_log.h"
#include "esp_sleep.h"
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
#define POWER_GPIO GPIO_NUM_13

/* For our config, we want to turn off the high 3 bits and leave the others
 * alone. Note that, strictly speaking, the DS18B20 only cares about bits 5
 * and 6 - bit 7 is always zero. But, some other sensors have that high bit
 * and can use it for other resolutions. But, clearing it should result in the
 * standard behavior and not bother the actual DS18B20.
 */

/* Values for the config register for various conversion precisions and
 * conversion times in ms.
 *
 * Note that various third party sensors may use these bits for other purposes;
 * if so, add them as necessary. This is only tested with the actual Dallas /
 * Maxim DS18B20.
 */
#define DS18B20_12BIT 0x7F
#define DS18B20_12BIT_TIME 750
#define DS18B20_11BIT 0x5F
#define DS18B20_11BIT_TIME 375
#define DS18B20_10BIT 0x3F
#define DS18B20_10BIT_TIME 188
#define DS18B20_9BIT  0x1F
#define DS18B20_9BIT_TIME 94

/*
 * Precision and delay selection. Note that we add one to the delay to ensure
 * ample time.
 */
#define SENSOR_CONFIG_REG_VALUE DS18B20_12BIT
#define MEASUREMENT_DELAY_MS DS18B20_12BIT_TIME + 1

/*
 * If we are running the DS18B20 in its default mode (12 bit resolution), we
 * don't need to update the config because new parts will be set to the
 * defaults. However, if not, this needs to be set to 1 so we update the config
 * accordingly.
 */
#define CHECK_DS18B20_CONFIG 1

// The last temperature we read.
float last_temp = 0;

// Use deep sleep. Defaults to true, but can be disabled via a console command.
// Note that there is no way to re-enable it; we just assume the user will 
// reboot the whole unit (and instruct them to do so).
bool use_deep_sleep = true;

/**
 * Turn on our sensor.
 */
static void sensor_on(void)
{
    /* Turn on the power, and set pullup mode on comms pin so the bus works.
     * The internal pullup violates the datasheet recommendations, but works,
     * and saves us an external resistor.
     */
    ESP_ERROR_CHECK(gpio_set_level(POWER_GPIO, 1));
    ESP_ERROR_CHECK(gpio_set_pull_mode(SENSOR_GPIO, GPIO_PULLUP_ONLY));
}

/**
 * Turn off our sensor.
 */
static void sensor_off(void)
{
    /* We've now gotten the temperature, turn off the power and bus lines,
     * and pull the bus line down avoid current leakage across the pullup.
     */
    ESP_ERROR_CHECK(gpio_set_level(SENSOR_GPIO, 0));
    ESP_ERROR_CHECK(gpio_set_level(POWER_GPIO, 0));
    ESP_ERROR_CHECK(gpio_set_pull_mode(SENSOR_GPIO, GPIO_PULLDOWN_ONLY));
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

#if CHECK_DS18B20_CONFIG
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
        // The config register is byte index 4
        if (scratchpad[4] == SENSOR_CONFIG_REG_VALUE) {
            ESP_LOGI(TAG, "Temperature sensor config is correct.");
            success = true;
        }
        else {
            scratchpad[4] = SENSOR_CONFIG_REG_VALUE;

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
#endif // CHECK_DS18B20_CONFIG

float c_to_f(float celsius)
{
    return celsius * 1.8 + 32;
}

/**
 * Temperature polling task.
 *
 * @param pvParameters Parameters for the function (unused)
 */
static void temp_task(void *pvParameters)
{
    bool comms_success;
    int retries;
    TickType_t last_wake_time = xTaskGetTickCount();
    TickType_t now;
    uint64_t interval_microseconds = 0;

    float temp_temp;

#if CHECK_DS18B20_CONFIG
    // Check and fix our sensor config.
    comms_success = false;
    retries = 5;
    while (!comms_success && retries > 0 ) {
        comms_success = check_and_fix_18b20_configuration();
        --retries;
    }
#endif // CHECK_DS18B20_CONFIG

    while (true) {
        comms_success = false;
        retries = 5; // prevent infinite tight loop

        ESP_LOGW(TAG, "Starting temperature sampling.");

        while (!comms_success && retries > 0 ) {
            comms_success = read_temperature(&temp_temp);
            if (comms_success && temp_temp == 85) {
                /* 85 is the power on reset value and sometimes our call to
                 * kick off a temperature conversion silently fails, resulting
                 * in no conversion happening. Since this is for residential
                 * HVAC, if we're ever in that temperature range, no amount of * AC is ever going to save us. So, it's fair to treat this as * out of range and sample again, even though doing so might
                 * put us into an infinite loop if it were ever to get that
                 * warm. I'm willing to take that risk.
                */
                comms_success = false;
                ESP_LOGW(TAG, "Discarding default reading of 85");
            }
            --retries;
        }

        ESP_LOGI(TAG, "Waiting for MQTT subscription.");
        // Wait for mqtt to be subscribed before attempting to send our message
        wait_for_mqtt_subscribed();

        if (comms_success) {
            // good temp, update
            if (current_config.use_celsius) {
                last_temp = temp_temp;
                ESP_LOGW(TAG, "Read temp: %.1f°C", last_temp);
            }
            else {
                last_temp = c_to_f(temp_temp);
                ESP_LOGW(TAG, "Read temp: %.1f°F", last_temp);
            }

            // We've read our temperature, wake up our WiFi task to send it.
            wifi_send_mqtt_temperature();
        }
        else {
            ESP_LOGE(TAG, "Temperature read invalid - not doing anything.");
        }

        ESP_LOGI(TAG, "Waiting for MQTT queue to be empty.");
        // While there are any MQTT messages outstanding, sleep for 10ms until
        // they are sent.
        wait_for_mqtt_queue_empty();

        // Once the queue is empty, we can disable WiFi.

        ESP_LOGI(TAG, "Waiting for WiFi off.");
        // we're about to go sleep; disable wifi so it comes down gracefully,
        // and then wait for it to actually be down.
        wifi_disable();
        wait_for_wifi_off();

        // and we need to account for the time we spent executing, above.
        now = xTaskGetTickCount();

        // Calculate our periodic time delay, making sure to get the poll time
        // from the config so it's always up to date, and subtracting off the
        // time we spent executing.
        interval_microseconds =
            current_config.poll_time_sec * 1000000 -
            (now - last_wake_time) * portTICK_PERIOD_MS * 1000;

        if (use_deep_sleep) {
            ESP_LOGW(TAG, "Deep sleep for %lld us.", interval_microseconds);

            esp_deep_sleep(interval_microseconds);
        }
        else {
            ESP_LOGW(TAG, "Normal sleep for %lld us.", interval_microseconds);

            vTaskDelay((interval_microseconds / 1000) / portTICK_PERIOD_MS);
        }

        // reset last_wake_time
        last_wake_time = xTaskGetTickCount();

        ESP_LOGI(TAG, "Waking up");
        // Enable WiFi again, then loop.
        wifi_enable();
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

void disable_deep_sleep(void)
{
    use_deep_sleep = false;
}