/**
 * @file lcb_crc8.c
 * @brief LCB Protocol CRC8 Implementation
 *
 * Converted from: com/jht/lcbcommlib/LcbCrc8.java
 */

#include "lcb_crc8.h"

/* CRC8 Lookup Table */
static const uint8_t CRC8_TABLE[16] = {
    0x00, 0x31, 0x62, 0x53, 0xC4, 0xF5, 0xA6, 0x97,
    0xB9, 0x88, 0xDB, 0xEA, 0x7D, 0x4C, 0x1F, 0x2E
};

/**
 * @brief Calculate CRC8 for a buffer
 */
uint8_t lcb_crc8_calculate(const uint8_t *buffer, size_t length)
{
    uint8_t crc = 0;
    uint8_t temp;

    if (buffer == NULL) {
        return 0;
    }

    for (size_t i = 0; i < length; i++) {
        uint8_t data = buffer[i];

        /* Process high nibble */
        temp = crc >> 4;
        crc <<= 4;
        crc &= 0x00FF;
        crc ^= CRC8_TABLE[temp ^ (data >> 4)];

        /* Process low nibble */
        temp = crc >> 4;
        crc <<= 4;
        crc &= 0x00FF;
        crc ^= CRC8_TABLE[temp ^ (data & 0x0F)];
    }

    return crc;
}

/**
 * @brief Reset CRC8 context
 */
void lcb_crc8_reset(lcb_crc8_context_t *ctx)
{
    if (ctx != NULL) {
        ctx->crc = 0;
    }
}

/**
 * @brief Feed one byte into CRC8 calculation
 */
void lcb_crc8_feed_byte(lcb_crc8_context_t *ctx, uint8_t data)
{
    if (ctx == NULL) {
        return;
    }

    uint8_t temp;

    /* Process high nibble */
    temp = ctx->crc >> 4;
    ctx->crc <<= 4;
    ctx->crc &= 0x00FF;
    ctx->crc ^= CRC8_TABLE[temp ^ (data >> 4)];

    /* Process low nibble */
    temp = ctx->crc >> 4;
    ctx->crc <<= 4;
    ctx->crc &= 0x00FF;
    ctx->crc ^= CRC8_TABLE[temp ^ (data & 0x0F)];
}

/**
 * @brief Get current CRC8 value
 */
uint8_t lcb_crc8_get_value(const lcb_crc8_context_t *ctx)
{
    if (ctx == NULL) {
        return 0;
    }

    return ctx->crc;
}
