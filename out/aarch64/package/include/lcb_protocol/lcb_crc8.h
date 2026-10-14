/**
 * @file lcb_crc8.h
 * @brief LCB Protocol CRC8 Calculation
 *
 * Converted from: com/jht/lcbcommlib/LcbCrc8.java
 */

#ifndef LCB_CRC8_H
#define LCB_CRC8_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculate CRC8 for a buffer
 * @param buffer Pointer to data buffer
 * @param length Number of bytes to calculate CRC for
 * @return CRC8 value
 */
uint8_t lcb_crc8_calculate(const uint8_t *buffer, size_t length);

/**
 * @brief CRC8 context for incremental calculation
 */
typedef struct {
    uint8_t crc;
} lcb_crc8_context_t;

/**
 * @brief Reset CRC8 context
 * @param ctx Pointer to CRC8 context
 */
void lcb_crc8_reset(lcb_crc8_context_t *ctx);

/**
 * @brief Feed one byte into CRC8 calculation
 * @param ctx Pointer to CRC8 context
 * @param data Byte to process
 */
void lcb_crc8_feed_byte(lcb_crc8_context_t *ctx, uint8_t data);

/**
 * @brief Get current CRC8 value
 * @param ctx Pointer to CRC8 context
 * @return Current CRC8 value
 */
uint8_t lcb_crc8_get_value(const lcb_crc8_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* LCB_CRC8_H */
