/**
 * @file
 * Queue code
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "queues.h"

QueueHandle_t wifi_queue = NULL;

bool create_queues(void)
{
    wifi_queue = xQueueCreate(WIFI_QUEUE_LENGTH, sizeof(uint8_t));

    if (wifi_queue == NULL) {
        return false;
    }

    return true;
}