/**
 * @file lcb_receive_data_queue.h
 * @brief LCB Receive Data Queue
 *
 * Converted from: com/jht/lcbcommlib/ReceiveDataQueue.java
 *
 * Thread-safe FIFO queue for raw serial port data buffers.
 * Decouples serial port reads from protocol parsing.
 */

#ifndef LCB_RECEIVE_DATA_QUEUE_H
#define LCB_RECEIVE_DATA_QUEUE_H

#include "lcb_receive_data_buffer.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Queue node structure (internal)
 */
typedef struct lcb_receive_data_node {
    lcb_receive_data_buffer_t buffer;
    struct lcb_receive_data_node *next;
} lcb_receive_data_node_t;

/**
 * @brief Receive data queue structure
 *
 * Thread-safe FIFO queue that holds raw serial port data.
 * Java equivalent: ReceiveDataQueue extends ConcurrentLinkedQueue<ReceiveDataBuffer>
 *
 * Synchronization uses pthread_mutex (stored as void* for platform flexibility,
 * same pattern as lcb_packet_queue_t).
 */
typedef struct {
    lcb_receive_data_node_t *head;   /**< Front of queue (dequeue here) */
    lcb_receive_data_node_t *tail;   /**< Back of queue (enqueue here) */
    uint32_t size;                    /**< Number of elements */
    void *mutex;                      /**< pthread_mutex_t* for thread safety */
} lcb_receive_data_queue_t;

/**
 * @brief Initialize receive data queue
 * @param queue Pointer to queue structure
 * @return 0 on success, negative on error
 */
int lcb_receive_data_queue_init(lcb_receive_data_queue_t *queue);

/**
 * @brief Free queue resources (clears all elements and destroys mutex)
 * @param queue Pointer to queue structure
 */
void lcb_receive_data_queue_free(lcb_receive_data_queue_t *queue);

/**
 * @brief Clear all elements from the queue
 *
 * Java equivalent: ReceiveDataQueue.clear()
 *
 * @param queue Pointer to queue structure
 */
void lcb_receive_data_queue_clear(lcb_receive_data_queue_t *queue);

/**
 * @brief Add raw data to the queue (clones the data internally)
 *
 * Creates a new ReceiveDataBuffer with a copy of the data and appends
 * it to the back of the queue.
 * Java equivalent: _receiveDataBufferQueue.add(_receiveDataBuffer.clone(rxBuffer, bytes))
 *
 * @param queue Pointer to queue structure
 * @param data Source byte array
 * @param length Number of bytes
 * @return 0 on success, negative on error
 */
int lcb_receive_data_queue_add(lcb_receive_data_queue_t *queue,
                                const uint8_t *data, uint32_t length);

/**
 * @brief Remove and retrieve the first element from the queue
 *
 * Removes the front element and transfers ownership of its buffer to the caller.
 * The caller must call lcb_receive_data_buffer_free() on the output when done.
 * Java equivalent: _receiveDataBufferQueue.remove()
 *
 * @param queue Pointer to queue structure
 * @param out Pointer to store removed buffer (caller owns the data)
 * @return 0 on success, -1 if queue is empty, negative on other errors
 */
int lcb_receive_data_queue_remove(lcb_receive_data_queue_t *queue,
                                   lcb_receive_data_buffer_t *out);

/**
 * @brief Get queue size
 * @param queue Pointer to queue structure
 * @return Number of elements in queue
 */
uint32_t lcb_receive_data_queue_size(const lcb_receive_data_queue_t *queue);

/**
 * @brief Check if queue is empty
 * @param queue Pointer to queue structure
 * @return true if empty, false otherwise
 */
bool lcb_receive_data_queue_is_empty(const lcb_receive_data_queue_t *queue);

#ifdef __cplusplus
}
#endif

#endif /* LCB_RECEIVE_DATA_QUEUE_H */
