/**
 * @file lcb_parser.h
 * @brief LCB Protocol Parser
 *
 * Converted from: com/jht/lcbcommlib/LcbParser.java
 */

#ifndef LCB_PARSER_H
#define LCB_PARSER_H

#include <stdint.h>
#include "lcb_packet.h"
#include "lcb_crc8.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parse result enumeration
 */
typedef enum {
    LCB_PARSE_SUCCESS,
    LCB_PARSE_INCOMPLETE,
    LCB_PARSE_INVALID_START_BYTE,
    LCB_PARSE_INVALID_RTN_COMMAND,
    LCB_PARSE_INVALID_CRC,
    LCB_PARSE_UNKNOWN_ERROR
} lcb_parse_result_t;

/**
 * @brief Parse state enumeration
 */
typedef enum {
    LCB_PARSE_STATE_START_BYTE,
    LCB_PARSE_STATE_STATUS,
    LCB_PARSE_STATE_MESSAGE_ID,
    LCB_PARSE_STATE_LENGTH,
    LCB_PARSE_STATE_DATA,
    LCB_PARSE_STATE_CRC
} lcb_parse_state_t;

/**
 * @brief Parser context structure
 */
typedef struct {
    lcb_crc8_context_t crc_ctx;
    lcb_packet_t packet;
    lcb_parse_result_t parse_result;
    lcb_parse_state_t parse_state;
    uint8_t parse_field_index;
} lcb_parser_t;

/**
 * @brief Initialize parser
 * @param parser Pointer to parser context
 * @return 0 on success, negative on error
 */
int lcb_parser_init(lcb_parser_t *parser);

/**
 * @brief Free parser resources
 * @param parser Pointer to parser context
 */
void lcb_parser_free(lcb_parser_t *parser);

/**
 * @brief Reset parser state
 * @param parser Pointer to parser context
 */
void lcb_parser_reset(lcb_parser_t *parser);

/**
 * @brief Parse one byte
 * @param parser Pointer to parser context
 * @param data Byte to parse
 * @return Parse result
 */
lcb_parse_result_t lcb_parser_parse_byte(lcb_parser_t *parser, uint8_t data);

/**
 * @brief Get parsed packet
 * @param parser Pointer to parser context
 * @return Pointer to packet (valid only after SUCCESS result)
 */
const lcb_packet_t* lcb_parser_get_packet(const lcb_parser_t *parser);

/**
 * @brief Get RX LCB status from parsed packet
 * @param parser Pointer to parser context
 * @return LCB status byte
 */
uint8_t lcb_parser_get_rx_status(const lcb_parser_t *parser);

/**
 * @brief Get command from parsed packet
 * @param parser Pointer to parser context
 * @return Command ID
 */
lcb_command_t lcb_parser_get_command(const lcb_parser_t *parser);

/**
 * @brief Get data from parsed packet
 * @param parser Pointer to parser context
 * @param length Pointer to store data length
 * @return Pointer to data buffer
 */
const uint8_t* lcb_parser_get_data(const lcb_parser_t *parser, uint8_t *length);

/**
 * @brief Clone parsed packet to destination
 * @param dest Destination packet
 * @param parser Source parser
 * @return 0 on success, negative on error
 */
int lcb_parser_clone_packet(lcb_packet_t *dest, const lcb_parser_t *parser);

/**
 * @brief Callback function type for parsed packet notification
 * @param packet Pointer to completed packet (valid only during callback)
 * @param user_data User-provided context pointer
 */
typedef void (*lcb_parser_on_packet_t)(const lcb_packet_t *packet, void *user_data);

/**
 * @brief Parse a buffer of data bytes
 *
 * Feeds each byte from the data buffer to the parser. For each complete
 * packet parsed, calls the on_packet callback. Automatically resets the
 * parser state after each successful packet or parse error.
 *
 * Java equivalent: receiveDataHandler() byte-by-byte parser loop
 *
 * @param parser Pointer to parser context
 * @param data Pointer to byte data
 * @param length Number of bytes in data
 * @param on_packet Callback invoked for each complete packet (may be NULL)
 * @param user_data User context passed to on_packet callback
 * @return Number of complete packets parsed, or negative on error
 */
int lcb_parser_parse_data(lcb_parser_t *parser,
                           const uint8_t *data, uint32_t length,
                           lcb_parser_on_packet_t on_packet, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* LCB_PARSER_H */
