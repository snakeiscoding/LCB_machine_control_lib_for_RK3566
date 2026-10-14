/**
 * @file lcb_packet_queue.c
 * @brief LCB Packet Priority Queue Implementation
 *
 * Converted from: com/jht/lcbcommlib/PacketQueue.java
 */

#include "lcb_packet_queue.h"
#include "lcb_logging.h"
#include <stdlib.h>
#include <pthread.h>

#define TAG "lcb_pkt_queue"

/**
 * @brief Initialize packet queue
 */
int lcb_packet_queue_init(lcb_packet_queue_t *queue)
{
    if (queue == NULL) {
        return -1;
    }

    queue->head = NULL;
    queue->size = 0;

    /* Create mutex */
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

/**
 * @brief Free queue resources
 */
void lcb_packet_queue_free(lcb_packet_queue_t *queue)
{
    if (queue == NULL) {
        return;
    }

    lcb_packet_queue_clear(queue);

    if (queue->mutex != NULL) {
        pthread_mutex_destroy((pthread_mutex_t*)queue->mutex);
        free(queue->mutex);
        queue->mutex = NULL;
    }
}

/**
 * @brief Clear all packets from queue
 */
void lcb_packet_queue_clear(lcb_packet_queue_t *queue)
{
    if (queue == NULL) {
        return;
    }

    pthread_mutex_lock((pthread_mutex_t*)queue->mutex);

    LCB_LOGD(TAG, "Clearing %u packets from queue", queue->size);

    lcb_queue_node_t *current = queue->head;
    while (current != NULL) {
        lcb_queue_node_t *next = current->next;
        lcb_packet_free(&current->packet);
        free(current);
        current = next;
    }

    queue->head = NULL;
    queue->size = 0;

    pthread_mutex_unlock((pthread_mutex_t*)queue->mutex);
}

/**
 * @brief Add packet to queue (sorted by priority)
 */
int lcb_packet_queue_add(lcb_packet_queue_t *queue, const lcb_packet_t *packet)
{
    if (queue == NULL || packet == NULL) {
        return -1;
    }

    LCB_LOGD(TAG, "Adding packet: cmd=0x%02X pri=%u", packet->command, packet->priority);

    /* Create new node */
    lcb_queue_node_t *new_node = (lcb_queue_node_t*)malloc(sizeof(lcb_queue_node_t));
    if (new_node == NULL) {
        LCB_LOGE(TAG, "Failed to allocate queue node");
        return -2;
    }

    /* Clone packet */
    if (lcb_packet_clone(&new_node->packet, packet) < 0) {
        LCB_LOGE(TAG, "Failed to clone packet");
        free(new_node);
        return -3;
    }
    new_node->next = NULL;

    pthread_mutex_lock((pthread_mutex_t*)queue->mutex);

    /* Insert into sorted position */
    if (queue->head == NULL || lcb_packet_compare(packet, &queue->head->packet) < 0) {
        /* Insert at head */
        new_node->next = queue->head;
        queue->head = new_node;
    } else {
        /* Find insertion point */
        lcb_queue_node_t *current = queue->head;
        while (current->next != NULL &&
               lcb_packet_compare(&current->next->packet, packet) <= 0) {
            current = current->next;
        }
        new_node->next = current->next;
        current->next = new_node;
    }

    queue->size++;

    LCB_LOGD(TAG, "Queue size after add: %u", queue->size);

    pthread_mutex_unlock((pthread_mutex_t*)queue->mutex);

    return 0;
}

/**
 * @brief Remove and get highest priority packet
 */
int lcb_packet_queue_remove(lcb_packet_queue_t *queue, lcb_packet_t *packet)
{
    if (queue == NULL || packet == NULL) {
        return -2;
    }

    pthread_mutex_lock((pthread_mutex_t*)queue->mutex);

    if (queue->head == NULL) {
        pthread_mutex_unlock((pthread_mutex_t*)queue->mutex);
        return -1;  /* Queue is empty */
    }

    /* Remove head node */
    lcb_queue_node_t *node = queue->head;
    queue->head = node->next;
    queue->size--;

    /* Move packet data to output */
    *packet = node->packet;

    LCB_LOGD(TAG, "Removed packet: cmd=0x%02X (queue size=%u)", packet->command, queue->size);

    /* Free node (but not packet data since we moved it) */
    free(node);

    pthread_mutex_unlock((pthread_mutex_t*)queue->mutex);

    return 0;
}

/**
 * @brief Get highest priority packet without removing
 */
int lcb_packet_queue_poll(lcb_packet_queue_t *queue, lcb_packet_t *packet)
{
    if (queue == NULL || packet == NULL) {
        return -2;
    }

    pthread_mutex_lock((pthread_mutex_t*)queue->mutex);

    if (queue->head == NULL) {
        pthread_mutex_unlock((pthread_mutex_t*)queue->mutex);
        return -1;  /* Queue is empty */
    }

    /* Clone packet */
    int result = lcb_packet_clone(packet, &queue->head->packet);

    pthread_mutex_unlock((pthread_mutex_t*)queue->mutex);

    return result;
}

/**
 * @brief Replace all packets with same command
 */
int lcb_packet_queue_replace_command(lcb_packet_queue_t *queue, const lcb_packet_t *new_packet)
{
    if (queue == NULL || new_packet == NULL) {
        return -1;
    }

    LCB_LOGD(TAG, "Replacing packets with cmd=0x%02X", new_packet->command);

    pthread_mutex_lock((pthread_mutex_t*)queue->mutex);

    /* Find highest priority packet with matching command */
    lcb_packet_t highest_packet;
    lcb_packet_clone(&highest_packet, new_packet);

    /* Remove all packets with same command and find highest priority */
    lcb_queue_node_t *current = queue->head;
    lcb_queue_node_t *prev = NULL;
    int removed_count = 0;

    while (current != NULL) {
        if (current->packet.command == new_packet->command) {
            removed_count++;
            /* Check if this packet has higher priority */
            if (lcb_packet_compare(&current->packet, &highest_packet) < 0) {
                /* This packet has higher priority, keep it but with new data.
                 * Only priority and id need updating — data is already
                 * new_packet's data from the clone at line 215. */
                highest_packet.priority = current->packet.priority;
                highest_packet.id = current->packet.id;
            }

            /* Remove this node */
            if (prev == NULL) {
                queue->head = current->next;
                lcb_packet_free(&current->packet);
                free(current);
                current = queue->head;
            } else {
                prev->next = current->next;
                lcb_packet_free(&current->packet);
                free(current);
                current = prev->next;
            }
            queue->size--;
        } else {
            prev = current;
            current = current->next;
        }
    }

    pthread_mutex_unlock((pthread_mutex_t*)queue->mutex);

    LCB_LOGD(TAG, "Replaced %d matching packets", removed_count);

    /* Add the highest priority packet */
    int result = lcb_packet_queue_add(queue, &highest_packet);
    lcb_packet_free(&highest_packet);
    return result;
}

/**
 * @brief Add packet only if same packet not already queued
 */
int lcb_packet_queue_add_if_not_queued(lcb_packet_queue_t *queue, const lcb_packet_t *packet)
{
    if (queue == NULL || packet == NULL) {
        return -1;
    }

    pthread_mutex_lock((pthread_mutex_t*)queue->mutex);

    /* Check for duplicates */
    lcb_queue_node_t *current = queue->head;
    while (current != NULL) {
        if (lcb_packet_equals(&current->packet, packet)) {
            pthread_mutex_unlock((pthread_mutex_t*)queue->mutex);
            return 0;  /* Duplicate found */
        }
        current = current->next;
    }

    pthread_mutex_unlock((pthread_mutex_t*)queue->mutex);

    /* No duplicate, add packet */
    if (lcb_packet_queue_add(queue, packet) == 0) {
        return 1;  /* Added */
    }

    return -2;  /* Error adding */
}

/**
 * @brief Get queue size
 */
uint32_t lcb_packet_queue_size(const lcb_packet_queue_t *queue)
{
    if (queue == NULL) {
        return 0;
    }

    pthread_mutex_lock((pthread_mutex_t*)queue->mutex);
    uint32_t size = queue->size;
    pthread_mutex_unlock((pthread_mutex_t*)queue->mutex);

    return size;
}

/**
 * @brief Check if queue is empty
 */
bool lcb_packet_queue_is_empty(const lcb_packet_queue_t *queue)
{
    return (lcb_packet_queue_size(queue) == 0);
}
