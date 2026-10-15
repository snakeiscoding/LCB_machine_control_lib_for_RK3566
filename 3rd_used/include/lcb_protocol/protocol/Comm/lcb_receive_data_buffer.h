/**
 * @file lcb_receive_data_buffer.h
 * @brief LCB Receive Data Buffer
 *
 * Converted from: com/jht/lcbcommlib/ReceiveDataBuffer.java
 *
 * Simple container for raw byte data received from the serial port.
 * Each buffer holds a snapshot of one serial port read operation.
 */

#ifndef LCB_RECEIVE_DATA_BUFFER_H
#define LCB_RECEIVE_DATA_BUFFER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Receive data buffer structure
 *
 * Holds a dynamically allocated copy of raw bytes read from the serial port.
 * Java equivalent: ReceiveDataBuffer with _dataBuffer and _bufferLen fields.
 */
typedef struct {
    uint8_t *data;       /**< Dynamically allocated byte array (owned) */
    uint32_t length;     /**< Number of valid bytes in data */
} lcb_receive_data_buffer_t;

/**
 * @brief Initialize a receive data buffer
 * @param buf Pointer to buffer structure
 */
void lcb_receive_data_buffer_init(lcb_receive_data_buffer_t *buf);

/**
 * @brief Free a receive data buffer's resources
 * @param buf Pointer to buffer structure
 */
void lcb_receive_data_buffer_free(lcb_receive_data_buffer_t *buf);

/**
 * @brief Clone data into a receive data buffer (deep copy)
 *
 * Allocates a new internal buffer and copies the data.
 * Java equivalent: ReceiveDataBuffer.clone(byte[] data, int bufferLen)
 *
 * @param dest Pointer to destination buffer (will be initialized)
 * @param data Source byte array to copy
 * @param length Number of bytes to copy
 * @return 0 on success, -1 on error (NULL args or allocation failure)
 */
int lcb_receive_data_buffer_clone(lcb_receive_data_buffer_t *dest,
                                   const uint8_t *data, uint32_t length);

/**
 * @brief Get pointer to buffer data
 * @param buf Pointer to buffer structure
 * @return Pointer to internal data array, or NULL if buf is NULL
 */
const uint8_t* lcb_receive_data_buffer_get_data(const lcb_receive_data_buffer_t *buf);

/**
 * @brief Get buffer data length
 * @param buf Pointer to buffer structure
 * @return Number of valid bytes, or 0 if buf is NULL
 */
uint32_t lcb_receive_data_buffer_get_length(const lcb_receive_data_buffer_t *buf);

#ifdef __cplusplus
}
#endif

#endif /* LCB_RECEIVE_DATA_BUFFER_H */
