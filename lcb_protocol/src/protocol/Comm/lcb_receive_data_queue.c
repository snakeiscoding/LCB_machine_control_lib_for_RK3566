/**
 * @file lcb_receive_data_queue.c
 * @brief LCB Receive Data Queue Implementation
 *
 * Converted from: com/jht/lcbcommlib/ReceiveDataQueue.java
 *
 * Thread-safe FIFO queue using pthread_mutex.
 * Java equivalent used ConcurrentLinkedQueue + synchronized(_syncObject).
 */

#include "lcb_receive_data_queue.h"
#include <stdlib.h>
#include <pthread.h>
#include "lcb_logging.h"

#define TAG "lcb_rx_queue"

int lcb_receive_data_queue_init(lcb_receive_data_queue_t *queue)
{
    if (queue == NULL) {
        return -1;
    }

    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;

    /* Create mutex (same pattern as lcb_packet_queue.c) */
    pthread_mutex_t *mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (mutex == NULL) {
        LCB_LOGE(TAG, "Failed to allocate mutex");
        return -2;
    }

    if (pthread_mutex_init(mutex, NULL) != 0) {
        LCB_LOGE(TAG, "Failed to initialize mutex");
        free(mutex);
        return -3;
    }

    queue->mutex = mutex;
    return 0;
}

void lcb_receive_data_queue_free(lcb_receive_data_queue_t *queue)
{
    if (queue == NULL) {
        return;
    }

    lcb_receive_data_queue_clear(queue);

    if (queue->mutex != NULL) {
        pthread_mutex_destroy((pthread_mutex_t*)queue->mutex);
        free(queue->mutex);
        queue->mutex = NULL;
    }
}

void lcb_receive_data_queue_clear(lcb_receive_data_queue_t *queue)
{
    if (queue == NULL) {
        return;
    }

    /* Java: synchronized (_syncObject) { super.clear(); } */
    pthread_mutex_lock((pthread_mutex_t*)queue->mutex);

    LCB_LOGD(TAG, "Clearing %u buffered entries", queue->size);

    lcb_receive_data_node_t *current = queue->head;
    while (current != NULL) {
        lcb_receive_data_node_t *next = current->next;
        lcb_receive_data_buffer_free(&current->buffer);
        free(current);
        current = next;
    }

    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;

    pthread_mutex_unlock((pthread_mutex_t*)queue->mutex);
}

int lcb_receive_data_queue_add(lcb_receive_data_queue_t *queue,
                                const uint8_t *data, uint32_t length)
{
    if (queue == NULL || data == NULL || length == 0) {
        return -1;
    }

    LCB_LOGD(TAG, "Adding %u bytes to queue", length);

    /* Create new node */
    lcb_receive_data_node_t *new_node =
        (lcb_receive_data_node_t*)malloc(sizeof(lcb_receive_data_node_t));
    if (new_node == NULL) {
        LCB_LOGE(TAG, "Failed to allocate queue node");
        return -2;
    }

    /* Clone data into buffer
     * (Java: _receiveDataBuffer.clone(rxBuffer, bytes)) */
    lcb_receive_data_buffer_init(&new_node->buffer);
    if (lcb_receive_data_buffer_clone(&new_node->buffer, data, length) < 0) {
        LCB_LOGE(TAG, "Failed to clone buffer data");
        free(new_node);
        return -3;
    }
    new_node->next = NULL;

    /* Java: synchronized (_syncObject) { super.add(dataBuffer); } */
    pthread_mutex_lock((pthread_mutex_t*)queue->mutex);

    /* Append to tail (FIFO order) */
    if (queue->tail != NULL) {
        queue->tail->next = new_node;
    } else {
        queue->head = new_node;
    }
    queue->tail = new_node;
    queue->size++;

    LCB_LOGD(TAG, "Queue size after add: %u", queue->size);

    pthread_mutex_unlock((pthread_mutex_t*)queue->mutex);

    return 0;
}

int lcb_receive_data_queue_remove(lcb_receive_data_queue_t *queue,
                                   lcb_receive_data_buffer_t *out)
{
    if (queue == NULL || out == NULL) {
        return -2;
    }

    /* Java: synchronized (_syncObject) { return super.remove(); } */
    pthread_mutex_lock((pthread_mutex_t*)queue->mutex);

    if (queue->head == NULL) {
        pthread_mutex_unlock((pthread_mutex_t*)queue->mutex);
        return -1;  /* Queue is empty */
    }

    /* Detach head node */
    lcb_receive_data_node_t *node = queue->head;
    queue->head = node->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    queue->size--;

    /* Transfer buffer ownership to caller (no deep copy needed) */
    *out = node->buffer;

    LCB_LOGD(TAG, "Removed %u bytes from queue (size=%u)", out->length, queue->size);

    /* Free node only (not the buffer data - caller now owns it) */
    free(node);

    pthread_mutex_unlock((pthread_mutex_t*)queue->mutex);

    return 0;
}

uint32_t lcb_receive_data_queue_size(const lcb_receive_data_queue_t *queue)
{
    if (queue == NULL) {
        return 0;
    }

    pthread_mutex_lock((pthread_mutex_t*)queue->mutex);
    uint32_t size = queue->size;
    pthread_mutex_unlock((pthread_mutex_t*)queue->mutex);

    return size;
}

bool lcb_receive_data_queue_is_empty(const lcb_receive_data_queue_t *queue)
{
    return (lcb_receive_data_queue_size(queue) == 0);
}
