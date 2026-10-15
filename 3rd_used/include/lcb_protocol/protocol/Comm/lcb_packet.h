/**
 * @file lcb_packet.h
 * @brief LCB Protocol Packet Structure
 *
 * Converted from: com/jht/lcbcommlib/LcbPacket.java
 */

#ifndef LCB_PACKET_H
#define LCB_PACKET_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "lcb_commands.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Packet priorities */
#define LCB_PRIORITY_HIGHEST    1
#define LCB_PRIORITY_HIGH       2
#define LCB_PRIORITY_NORMAL     3
#define LCB_PRIORITY_LOW        4
#define LCB_PRIORITY_LOWEST     5

/* Special operations */
#define LCB_SPECIAL_NONE        0
#define LCB_SPECIAL_400MS_PULSE 1  /* ERP wake up pulse must be issued before packet is sent */

/* Packet structure constants */
#define LCB_PACKET_START_BYTE_TX    0x00
#define LCB_PACKET_START_BYTE_RX    0x01
#define LCB_PACKET_HEADER_SIZE      4
#define LCB_PACKET_CRC_SIZE         1
#define LCB_PACKET_MIN_SIZE         (LCB_PACKET_HEADER_SIZE + LCB_PACKET_CRC_SIZE)
#define LCB_PACKET_STATUS_BYTE      0xFF
#define LCB_PACKET_MAX_DATA_SIZE    255

/* Wire bus addresses — byte 1 of a TX frame doubles as the destination
 * address (chimera2: LcbPacket.PACKET_DEFAULT_ADDRESS / PACKET_IR_ADDRESS).
 * LCB_PACKET_STATUS_BYTE (0xFF) IS the main-board address; this alias just
 * names that fact for new address-aware call sites. The IR Address board
 * (the "Hybrid TM" IR beam E-Stop sensor, LCB cmd 0x73/0x99/0x7A) is a
 * second device on the same physical bus at 0x26 — same packet framing and
 * CRC8, only this one byte differs. */
#define LCB_PACKET_ADDRESS_MAIN     LCB_PACKET_STATUS_BYTE
#define LCB_PACKET_ADDRESS_IR_BOARD 0x26

/**
 * @brief LCB Packet structure
 */
typedef struct {
    uint8_t priority;           /* Lower value = higher priority */
    uint64_t id;                /* Unique packet ID (for sorting) */
    uint8_t rx_lcb_status;      /* Received LCB status byte */
    lcb_command_t command;      /* Command ID */
    uint8_t *data;              /* Pointer to data buffer */
    uint8_t data_length;        /* Length of data */
    uint8_t special_operation;  /* Special operation flag */
    uint8_t address;            /* TX destination address (default LCB_PACKET_ADDRESS_MAIN) */
} lcb_packet_t;

/**
 * @brief Initialize a packet
 * @param packet Pointer to packet structure
 * @return 0 on success, negative on error
 */
int lcb_packet_init(lcb_packet_t *packet);

/**
 * @brief Free packet resources
 * @param packet Pointer to packet structure
 */
void lcb_packet_free(lcb_packet_t *packet);

/**
 * @brief Set packet command
 * @param packet Pointer to packet structure
 * @param command Command ID
 */
void lcb_packet_set_command(lcb_packet_t *packet, lcb_command_t command);

/**
 * @brief Set packet TX destination address (default LCB_PACKET_ADDRESS_MAIN)
 * @param packet Pointer to packet structure
 * @param address LCB_PACKET_ADDRESS_MAIN or LCB_PACKET_ADDRESS_IR_BOARD
 */
void lcb_packet_set_address(lcb_packet_t *packet, uint8_t address);

/**
 * @brief Get packet TX destination address
 * @param packet Pointer to packet structure
 * @return Address byte
 */
uint8_t lcb_packet_get_address(const lcb_packet_t *packet);

/**
 * @brief Get packet command
 * @param packet Pointer to packet structure
 * @return Command ID
 */
lcb_command_t lcb_packet_get_command(const lcb_packet_t *packet);

/**
 * @brief Set packet data
 * @param packet Pointer to packet structure
 * @param data Pointer to data buffer
 * @param length Data length
 * @return 0 on success, negative on error
 */
int lcb_packet_set_data(lcb_packet_t *packet, const uint8_t *data, uint8_t length);

/**
 * @brief Get packet data
 * @param packet Pointer to packet structure
 * @param length Pointer to store data length
 * @return Pointer to data buffer or NULL
 */
const uint8_t* lcb_packet_get_data(const lcb_packet_t *packet, uint8_t *length);

/**
 * @brief Get RX LCB status byte
 * @param packet Pointer to packet structure
 * @return LCB status byte
 */
uint8_t lcb_packet_get_rx_status(const lcb_packet_t *packet);

/**
 * @brief Set packet priority
 * @param packet Pointer to packet structure
 * @param priority Priority value (1-5, lower is higher priority)
 */
void lcb_packet_set_priority(lcb_packet_t *packet, uint8_t priority);

/**
 * @brief Get packet priority
 * @param packet Pointer to packet structure
 * @return Priority value
 */
uint8_t lcb_packet_get_priority(const lcb_packet_t *packet);

/**
 * @brief Build transmit packet
 * @param packet Pointer to packet structure
 * @param buffer Pointer to output buffer
 * @param buffer_size Size of output buffer
 * @param packet_size Pointer to store actual packet size
 * @return 0 on success, negative on error
 */
int lcb_packet_build_tx(const lcb_packet_t *packet, uint8_t *buffer,
                        size_t buffer_size, size_t *packet_size);

/**
 * @brief Clone a packet
 * @param dest Destination packet
 * @param src Source packet
 * @return 0 on success, negative on error
 */
int lcb_packet_clone(lcb_packet_t *dest, const lcb_packet_t *src);

/**
 * @brief Compare two packets for equality
 * @param packet1 First packet
 * @param packet2 Second packet
 * @return true if equal, false otherwise
 */
bool lcb_packet_equals(const lcb_packet_t *packet1, const lcb_packet_t *packet2);

/**
 * @brief Compare two packets for priority sorting
 * @param packet1 First packet
 * @param packet2 Second packet
 * @return < 0 if packet1 has higher priority, > 0 if packet2 has higher priority, 0 if equal
 */
int lcb_packet_compare(const lcb_packet_t *packet1, const lcb_packet_t *packet2);

#ifdef __cplusplus
}
#endif

#endif /* LCB_PACKET_H */
