/**
 * @file
 * Header for queues.c.
 */

#ifndef __QUEUES_H_
#define __QUEUES_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// 2 should be enough - we'll queue and off and on without blocking.
#define WIFI_QUEUE_LENGTH 2

/**
 * Our wifi message queue.
 */
extern QueueHandle_t wifi_queue;

/**
 * Create our queues.
 *
 * @return true on success
 * @return false on failure
 */
bool create_queues(void);

#endif // __QUEUES_H_