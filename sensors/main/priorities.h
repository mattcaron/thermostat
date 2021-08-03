/**
 * @file
 * Lists of priorities for all tasks.
 */

#include "freertos/FreeRTOSConfig.h"

#ifndef __PRIORITIES_H_
#define __PRIORITIES_H_

#define CONSOLE_TASK_PRIORITY  ((UBaseType_t)(configMAX_PRIORITIES - 2))
#define WIFI_TASK_PRIORITY     ((UBaseType_t)(configMAX_PRIORITIES - 3))
#define TEMP_TASK_PRIORITY     ((UBaseType_t)(configMAX_PRIORITIES - 4))

#endif // __PRIORITIES_H_
