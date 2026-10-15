/**
 * @file lcb_packet_queue.h
 * @brief LCB Packet Priority Queue
 *
 * Converted from: com/jht/lcbcommlib/PacketQueue.java
 */

#ifndef LCB_PACKET_QUEUE_H
#define LCB_PACKET_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include "lcb_packet.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Packet queue node structure (internal)
 */
typedef struct lcb_queue_node {
    lcb_packet_t packet;
    struct lcb_queue_node *next;
} lcb_queue_node_t;

/**
 * @brief Packet queue structure
 */
typedef struct {
    lcb_queue_node_t *head;
    uint32_t size;
    void *mutex;  /* Platform-specific mutex pointer */
} lcb_packet_queue_t;

/**
 * @brief Initialize packet queue
 * @param queue Pointer to queue structure
 * @return 0 on success, negative on error
 */
int lcb_packet_queue_init(lcb_packet_queue_t *queue);

/**
 * @brief Free queue resources
 * @param queue Pointer to queue structure
 */
void lcb_packet_queue_free(lcb_packet_queue_t *queue);

/**
 * @brief Clear all packets from queue
 * @param queue Pointer to queue structure
 */
void lcb_packet_queue_clear(lcb_packet_queue_t *queue);

/**
 * @brief Add packet to queue (sorted by priority)
 * @param queue Pointer to queue structure
 * @param packet Pointer to packet to add
 * @return 0 on success, negative on error
 */
int lcb_packet_queue_add(lcb_packet_queue_t *queue, const lcb_packet_t *packet);

/**
 * @brief Remove and get highest priority packet
 * @param queue Pointer to queue structure
 * @param packet Pointer to store removed packet
 * @return 0 on success, -1 if queue is empty, negative on other errors
 */
int lcb_packet_queue_remove(lcb_packet_queue_t *queue, lcb_packet_t *packet);

/**
 * @brief Get highest priority packet without removing
 * @param queue Pointer to queue structure
 * @param packet Pointer to store packet copy
 * @return 0 on success, -1 if queue is empty, negative on other errors
 */
int lcb_packet_queue_poll(lcb_packet_queue_t *queue, lcb_packet_t *packet);

/**
 * @brief Replace all packets with same command
 *
 * Removes all occurrences of packets with the same command as newPacket
 * and adds the new packet with the highest priority of existing or new packet.
 *
 * @param queue Pointer to queue structure
 * @param new_packet Pointer to new packet
 * @return 0 on success, negative on error
 */
int lcb_packet_queue_replace_command(lcb_packet_queue_t *queue, const lcb_packet_t *new_packet);

/**
 * @brief Add packet only if same packet not already queued
 * @param queue Pointer to queue structure
 * @param packet Pointer to packet to add
 * @return 1 if added, 0 if duplicate found, negative on error
 */
int lcb_packet_queue_add_if_not_queued(lcb_packet_queue_t *queue, const lcb_packet_t *packet);

/**
 * @brief Get queue size
 * @param queue Pointer to queue structure
 * @return Number of packets in queue
 */
uint32_t lcb_packet_queue_size(const lcb_packet_queue_t *queue);

/**
 * @brief Check if queue is empty
 * @param queue Pointer to queue structure
 * @return true if empty, false otherwise
 */
bool lcb_packet_queue_is_empty(const lcb_packet_queue_t *queue);

#ifdef __cplusplus
}
#endif

#endif /* LCB_PACKET_QUEUE_H */
