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

#endif // __TEMPERATURE_H_