/**
 * @file
 * Header file for temperature.c.
 */

#ifndef __TEMPERATURE_H_
#define __TEMPERATURE_H_

/**
 * Convert Celsius to Farenheit.
 *
 * @param celsius The temperature in Celsius.
 *
 * @return The temperature in Farenheit.
 */
float c_to_f(float celsius);

/**
 * Retrieve our last sampled temperature.
 *
 * This his so the CLI can report it without triggering another read,
 * because then we'd have to guard the hardware access with a mutex and
 * that's just unnecessary for a rarely used feature.
 *
 * @return the last value of the last sampled temperature.
 */
float get_last_temp(void);

/**
 * Start our temperature polling task.
 */
void start_temp_polling(void);

#endif // __TEMPERATURE_H_
