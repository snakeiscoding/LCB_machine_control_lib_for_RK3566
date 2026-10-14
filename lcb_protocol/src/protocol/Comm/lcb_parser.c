/**
 * @file lcb_parser.c
 * @brief LCB Protocol Parser Implementation
 *
 * Converted from: com/jht/lcbcommlib/LcbParser.java
 */

#include "lcb_parser.h"
#include <stdlib.h>
#include <string.h>
#include "lcb_logging.h"

#define TAG "lcb_parser"

/**
 * @brief Initialize parser
 */
int lcb_parser_init(lcb_parser_t *parser)
{
    if (parser == NULL) {
        return -1;
    }

    lcb_crc8_reset(&parser->crc_ctx);
    lcb_packet_init(&parser->packet);
    parser->parse_result = LCB_PARSE_INCOMPLETE;
    parser->parse_state = LCB_PARSE_STATE_START_BYTE;
    parser->parse_field_index = 0;

    return 0;
}

/**
 * @brief Free parser resources
 */
void lcb_parser_free(lcb_parser_t *parser)
{
    if (parser == NULL) {
        return;
    }

    lcb_packet_free(&parser->packet);
}

/**
 * @brief Reset parser state
 */
void lcb_parser_reset(lcb_parser_t *parser)
{
    if (parser == NULL) {
        return;
    }

    lcb_crc8_reset(&parser->crc_ctx);
    lcb_packet_free(&parser->packet);
    lcb_packet_init(&parser->packet);
    parser->packet.command = LCB_CMD_GET_STATUS;
    parser->parse_result = LCB_PARSE_INCOMPLETE;
    parser->parse_state = LCB_PARSE_STATE_START_BYTE;
    parser->parse_field_index = 0;
}

/**
 * @brief Parse one byte
 */
lcb_parse_result_t lcb_parser_parse_byte(lcb_parser_t *parser, uint8_t data)
{
    if (parser == NULL) {
        return LCB_PARSE_UNKNOWN_ERROR;
    }

    switch (parser->parse_state) {
        case LCB_PARSE_STATE_START_BYTE:
            if (data == LCB_PACKET_START_BYTE_RX) {
                parser->parse_state = LCB_PARSE_STATE_STATUS;
                lcb_crc8_reset(&parser->crc_ctx);
                lcb_crc8_feed_byte(&parser->crc_ctx, data);
                LCB_LOGD(TAG, "State: START_BYTE -> STATUS");
                return LCB_PARSE_INCOMPLETE;
            } else {
                LCB_LOGE(TAG, "Invalid start byte: 0x%02X (expected 0x%02X)", data, LCB_PACKET_START_BYTE_RX);
                return LCB_PARSE_INVALID_START_BYTE;
            }

        case LCB_PARSE_STATE_STATUS:
            lcb_crc8_feed_byte(&parser->crc_ctx, data);
            parser->packet.rx_lcb_status = data;
            parser->parse_state = LCB_PARSE_STATE_MESSAGE_ID;
            LCB_LOGD(TAG, "State: STATUS (0x%02X) -> MESSAGE_ID", data);
            return LCB_PARSE_INCOMPLETE;

        case LCB_PARSE_STATE_MESSAGE_ID:
            lcb_crc8_feed_byte(&parser->crc_ctx, data);
            parser->packet.command = (lcb_command_t)data;

            /* Validate command - in C we can't easily validate enum,
               so we just accept any byte value as command */
            parser->parse_state = LCB_PARSE_STATE_LENGTH;
            LCB_LOGD(TAG, "State: MESSAGE_ID (0x%02X) -> LENGTH", data);
            return LCB_PARSE_INCOMPLETE;

        case LCB_PARSE_STATE_LENGTH:
            lcb_crc8_feed_byte(&parser->crc_ctx, data);

            /* Allocate data buffer if needed */
            if (data > 0) {
                if (lcb_packet_set_data(&parser->packet, NULL, 0) < 0) {
                    return LCB_PARSE_UNKNOWN_ERROR;
                }

                parser->packet.data = (uint8_t*)malloc(data);
                if (parser->packet.data == NULL) {
                    LCB_LOGE(TAG, "Failed to allocate %u bytes for packet data", data);
                    return LCB_PARSE_UNKNOWN_ERROR;
                }
                parser->packet.data_length = data;
                parser->parse_state = LCB_PARSE_STATE_DATA;
                LCB_LOGD(TAG, "State: LENGTH (%u) -> DATA", data);
            } else {
                parser->packet.data_length = 0;
                parser->parse_state = LCB_PARSE_STATE_CRC;
                LCB_LOGD(TAG, "State: LENGTH (%u) -> CRC", data);
            }

            parser->parse_field_index = 0;
            return LCB_PARSE_INCOMPLETE;

        case LCB_PARSE_STATE_DATA:
            lcb_crc8_feed_byte(&parser->crc_ctx, data);
            parser->packet.data[parser->parse_field_index] = data;
            parser->parse_field_index++;

            if (parser->parse_field_index >= parser->packet.data_length) {
                parser->parse_state = LCB_PARSE_STATE_CRC;
                LCB_LOGD(TAG, "State: DATA [%u/%u] -> CRC", parser->parse_field_index, parser->packet.data_length);
            } else {
                LCB_LOGD(TAG, "State: DATA [%u/%u] -> DATA", parser->parse_field_index, parser->packet.data_length);
            }
            return LCB_PARSE_INCOMPLETE;

        case LCB_PARSE_STATE_CRC:
            parser->parse_state = LCB_PARSE_STATE_START_BYTE;

            /* Validate CRC */
            uint8_t calculated_crc = lcb_crc8_get_value(&parser->crc_ctx);
            if (data != calculated_crc) {
                LCB_LOGE(TAG, "CRC mismatch: expected=0x%02X calculated=0x%02X", data, calculated_crc);
                return LCB_PARSE_INVALID_CRC;
            }

            LCB_LOGD(TAG, "Packet complete: cmd=0x%02X len=%u", parser->packet.command, parser->packet.data_length);
            return LCB_PARSE_SUCCESS;

        default:
            return LCB_PARSE_UNKNOWN_ERROR;
    }
}

/**
 * @brief Get parsed packet
 */
const lcb_packet_t* lcb_parser_get_packet(const lcb_parser_t *parser)
{
    if (parser == NULL) {
        return NULL;
    }
    return &parser->packet;
}

/**
 * @brief Get RX LCB status from parsed packet
 */
uint8_t lcb_parser_get_rx_status(const lcb_parser_t *parser)
{
    if (parser == NULL) {
        return 0;
    }
    return parser->packet.rx_lcb_status;
}

/**
 * @brief Get command from parsed packet
 */
lcb_command_t lcb_parser_get_command(const lcb_parser_t *parser)
{
    if (parser == NULL) {
        return LCB_CMD_GET_STATUS;
    }
    return parser->packet.command;
}

/**
 * @brief Get data from parsed packet
 */
const uint8_t* lcb_parser_get_data(const lcb_parser_t *parser, uint8_t *length)
{
    if (parser == NULL) {
        if (length != NULL) {
            *length = 0;
        }
        return NULL;
    }

    if (length != NULL) {
        *length = parser->packet.data_length;
    }

    return parser->packet.data;
}

/**
 * @brief Clone parsed packet to destination
 */
int lcb_parser_clone_packet(lcb_packet_t *dest, const lcb_parser_t *parser)
{
    if (dest == NULL || parser == NULL) {
        return -1;
    }

    return lcb_packet_clone(dest, &parser->packet);
}

/**
 * @brief Parse a buffer of data bytes
 *
 * Java equivalent: receiveDataHandler() byte-by-byte parser loop in LcbComm.java
 */
int lcb_parser_parse_data(lcb_parser_t *parser,
                           const uint8_t *data, uint32_t length,
                           lcb_parser_on_packet_t on_packet, void *user_data)
{
    if (parser == NULL || data == NULL) {
        return -1;
    }

    int packet_count = 0;

    for (uint32_t i = 0; i < length; i++) {
        lcb_parse_result_t result = lcb_parser_parse_byte(parser, data[i]);

        if (result == LCB_PARSE_SUCCESS) {
            /* Complete packet parsed - notify callback */
            if (on_packet != NULL) {
                on_packet(&parser->packet, user_data);
            }
            packet_count++;
            lcb_parser_reset(parser);
        } else if (result != LCB_PARSE_INCOMPLETE) {
            /* Parse error - reset and continue */
            lcb_parser_reset(parser);
        }
    }

    return packet_count;
}
