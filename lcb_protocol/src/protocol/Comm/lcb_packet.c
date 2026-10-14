/**
 * @file lcb_packet.c
 * @brief LCB Protocol Packet Implementation
 *
 * Converted from: com/jht/lcbcommlib/LcbPacket.java
 */

#include "lcb_packet.h"
#include "lcb_crc8.h"
#include "lcb_logging.h"
#include <stdlib.h>
#include <string.h>

#define TAG "lcb_packet"

/* Global packet counter for ID generation */
static uint64_t g_packet_counter = 0;

/**
 * @brief Initialize a packet
 */
int lcb_packet_init(lcb_packet_t *packet)
{
    if (packet == NULL) {
        return -1;
    }

    packet->priority = LCB_PRIORITY_NORMAL;
    packet->id = 0;
    packet->rx_lcb_status = 0;
    packet->command = LCB_CMD_GET_STATUS;
    packet->data = NULL;
    packet->data_length = 0;
    packet->special_operation = LCB_SPECIAL_NONE;
    packet->address = LCB_PACKET_ADDRESS_MAIN;

    return 0;
}

/**
 * @brief Free packet resources
 */
void lcb_packet_free(lcb_packet_t *packet)
{
    if (packet == NULL) {
        return;
    }

    if (packet->data != NULL) {
        free(packet->data);
        packet->data = NULL;
    }
    packet->data_length = 0;
}

/**
 * @brief Set packet command
 */
void lcb_packet_set_command(lcb_packet_t *packet, lcb_command_t command)
{
    if (packet != NULL) {
        packet->command = command;
        LCB_LOGD(TAG, "Packet command set to 0x%02X", command);
    }
}

/**
 * @brief Get packet command
 */
lcb_command_t lcb_packet_get_command(const lcb_packet_t *packet)
{
    if (packet == NULL) {
        return LCB_CMD_GET_STATUS;
    }
    return packet->command;
}

/**
 * @brief Set packet TX destination address
 */
void lcb_packet_set_address(lcb_packet_t *packet, uint8_t address)
{
    if (packet != NULL) {
        packet->address = address;
    }
}

/**
 * @brief Get packet TX destination address
 */
uint8_t lcb_packet_get_address(const lcb_packet_t *packet)
{
    if (packet == NULL) {
        return LCB_PACKET_ADDRESS_MAIN;
    }
    return packet->address;
}

/**
 * @brief Set packet data
 */
int lcb_packet_set_data(lcb_packet_t *packet, const uint8_t *data, uint8_t length)
{
    if (packet == NULL) {
        return -1;
    }

    /* Free existing data */
    if (packet->data != NULL) {
        LCB_LOGD(TAG, "Freed previous packet data (%u bytes)", packet->data_length);
        free(packet->data);
        packet->data = NULL;
        packet->data_length = 0;
    }

    /* Handle NULL or zero-length data */
    if (data == NULL || length == 0) {
        return 0;
    }

    /* Allocate new buffer */
    packet->data = (uint8_t*)malloc(length);
    if (packet->data == NULL) {
        LCB_LOGE(TAG, "Failed to allocate %u bytes for packet data", length);
        return -2;
    }

    /* Copy data */
    memcpy(packet->data, data, length);
    packet->data_length = length;
    LCB_LOGD(TAG, "Packet data set: %u bytes", length);

    return 0;
}

/**
 * @brief Get packet data
 */
const uint8_t* lcb_packet_get_data(const lcb_packet_t *packet, uint8_t *length)
{
    if (packet == NULL) {
        if (length != NULL) {
            *length = 0;
        }
        return NULL;
    }

    if (length != NULL) {
        *length = packet->data_length;
    }

    return packet->data;
}

/**
 * @brief Get RX LCB status byte
 */
uint8_t lcb_packet_get_rx_status(const lcb_packet_t *packet)
{
    if (packet == NULL) {
        return 0;
    }
    return packet->rx_lcb_status;
}

/**
 * @brief Set packet priority
 */
void lcb_packet_set_priority(lcb_packet_t *packet, uint8_t priority)
{
    if (packet == NULL) {
        return;
    }

    packet->priority = priority;
    packet->id = ++g_packet_counter;
    LCB_LOGD(TAG, "Packet priority=%u id=%llu", priority, (unsigned long long)packet->id);
}

/**
 * @brief Get packet priority
 */
uint8_t lcb_packet_get_priority(const lcb_packet_t *packet)
{
    if (packet == NULL) {
        return LCB_PRIORITY_NORMAL;
    }
    return packet->priority;
}

/**
 * @brief Build transmit packet
 */
int lcb_packet_build_tx(const lcb_packet_t *packet, uint8_t *buffer,
                        size_t buffer_size, size_t *packet_size)
{
    if (packet == NULL || buffer == NULL || packet_size == NULL) {
        LCB_LOGE(TAG, "Invalid parameters to build_tx");
        return -1;
    }

    /* Calculate required size */
    size_t required_size = LCB_PACKET_MIN_SIZE + packet->data_length;
    if (buffer_size < required_size) {
        LCB_LOGE(TAG, "Buffer too small: required=%zu available=%zu", required_size, buffer_size);
        return -2;
    }

    size_t index = 0;

    /* Start byte */
    buffer[index++] = LCB_PACKET_START_BYTE_TX;

    /* Address byte (0xFF main LCB for single receiver, 0x26 IR Address board) */
    buffer[index++] = packet->address;

    /* Command ID */
    buffer[index++] = (uint8_t)packet->command;

    /* Data length */
    buffer[index++] = packet->data_length;

    /* Data */
    if (packet->data_length > 0 && packet->data != NULL) {
        memcpy(&buffer[index], packet->data, packet->data_length);
        index += packet->data_length;
    }

    /* CRC8 (calculate over all bytes except CRC itself) */
    buffer[index] = lcb_crc8_calculate(buffer, index);
    index++;

    *packet_size = index;
    LCB_LOGD(TAG, "Built TX packet: %zu bytes (cmd=0x%02X crc=0x%02X)", *packet_size, packet->command, buffer[*packet_size-1]);

    return 0;
}

/**
 * @brief Clone a packet
 */
int lcb_packet_clone(lcb_packet_t *dest, const lcb_packet_t *src)
{
    if (dest == NULL || src == NULL) {
        return -1;
    }

    /* Initialize destination */
    lcb_packet_init(dest);

    /* Copy simple fields */
    dest->command = src->command;
    dest->rx_lcb_status = src->rx_lcb_status;
    dest->priority = src->priority;
    dest->id = src->id;
    dest->special_operation = src->special_operation;
    dest->address = src->address;

    /* Copy data */
    if (src->data_length > 0 && src->data != NULL) {
        int result = lcb_packet_set_data(dest, src->data, src->data_length);
        if (result == 0) {
            LCB_LOGD(TAG, "Cloned packet: cmd=0x%02X len=%u", src->command, src->data_length);
        }
        return result;
    }

    LCB_LOGD(TAG, "Cloned packet: cmd=0x%02X len=%u", src->command, src->data_length);
    return 0;
}

/**
 * @brief Compare two packets for equality
 */
bool lcb_packet_equals(const lcb_packet_t *packet1, const lcb_packet_t *packet2)
{
    if (packet1 == NULL || packet2 == NULL) {
        return false;
    }

    /* Compare command */
    if (packet1->command != packet2->command) {
        return false;
    }

    /* Compare data length */
    if (packet1->data_length != packet2->data_length) {
        return false;
    }

    /* Compare data */
    if (packet1->data_length > 0) {
        if (packet1->data == NULL || packet2->data == NULL) {
            return false;
        }
        if (memcmp(packet1->data, packet2->data, packet1->data_length) != 0) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Compare two packets for priority sorting
 */
int lcb_packet_compare(const lcb_packet_t *packet1, const lcb_packet_t *packet2)
{
    if (packet1 == NULL || packet2 == NULL) {
        return 0;
    }

    /* Compare priority (lower value = higher priority) */
    if (packet1->priority < packet2->priority) {
        return -1;
    } else if (packet1->priority > packet2->priority) {
        return 1;
    }

    /* If priority is same, compare ID (lower ID came first) */
    if (packet1->id < packet2->id) {
        return -1;
    } else if (packet1->id > packet2->id) {
        return 1;
    }

    return 0;
}
