/**
 * @file
 * Header file for temperature.c.
 */

#ifndef __TEMPERATURE_H_
#define __TEMPERATURE_H_

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
bool read_temperature(float *temperature);

/**
 * Check and (optionally) fix the configuration.
 *
 * @return true the configuration was or is now correct.
 * @return false if there was any sort of error.
 */
bool check_and_fix_18b20_configuration(void);

/**
 * Convert Celsius to Farenheit.
 *
 * @param celsius The temperature in Celsius.
 *
 * @return The temperature in Farenheit.
 */
float c_to_f(float celsius);

#endif // __TEMPERATURE_H_