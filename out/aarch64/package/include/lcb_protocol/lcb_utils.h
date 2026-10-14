/**
 * @file lcb_utils.h
 * @brief LCB Protocol Utility Functions
 *
 * Converted from: com/jht/lcbcommlib/DataUtils.java
 */

#ifndef LCB_UTILS_H
#define LCB_UTILS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert unsigned byte to int
 * @param byte Byte value
 * @return Integer value (0-255)
 */
static inline int lcb_unsigned_byte_to_int(uint8_t byte) {
    return (int)(byte & 0xFF);
}

/**
 * @brief Convert bytes to uint16 (Big-Endian)
 * @param data Pointer to data buffer (at least 2 bytes)
 * @return 16-bit unsigned integer
 */
static inline uint16_t lcb_bytes_to_uint16_be(const uint8_t *data) {
    return ((uint16_t)data[0] << 8) | (uint16_t)data[1];
}

/**
 * @brief Convert bytes to uint32 (Big-Endian)
 * @param data Pointer to data buffer (at least 4 bytes)
 * @return 32-bit unsigned integer
 */
static inline uint32_t lcb_bytes_to_uint32_be(const uint8_t *data) {
    return ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) |
           (uint32_t)data[3];
}

/**
 * @brief Convert uint16 to bytes (Big-Endian)
 * @param value 16-bit unsigned integer
 * @param data Output buffer (at least 2 bytes)
 */
static inline void lcb_uint16_to_bytes_be(uint16_t value, uint8_t *data) {
    data[0] = (uint8_t)(value >> 8);
    data[1] = (uint8_t)(value & 0xFF);
}

/**
 * @brief Convert uint32 to bytes (Big-Endian)
 * @param value 32-bit unsigned integer
 * @param data Output buffer (at least 4 bytes)
 */
static inline void lcb_uint32_to_bytes_be(uint32_t value, uint8_t *data) {
    data[0] = (uint8_t)(value >> 24);
    data[1] = (uint8_t)(value >> 16);
    data[2] = (uint8_t)(value >> 8);
    data[3] = (uint8_t)(value & 0xFF);
}

/**
 * @brief Convert byte array to hex string
 * @param data Pointer to data buffer
 * @param length Length of data
 * @param output Output string buffer
 * @param output_size Size of output buffer
 * @return Number of characters written (excluding null terminator)
 */
int lcb_bytes_to_hex_string(const uint8_t *data, size_t length,
                             char *output, size_t output_size);

/**
 * @brief Convert hex string to byte array
 * @param hex_string Input hex string
 * @param data Output data buffer
 * @param max_length Maximum length of output buffer
 * @return Number of bytes written, or negative on error
 */
int lcb_hex_string_to_bytes(const char *hex_string, uint8_t *data, size_t max_length);

#ifdef __cplusplus
}
#endif

#endif /* LCB_UTILS_H */
