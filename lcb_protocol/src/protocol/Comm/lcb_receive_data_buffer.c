/**
 * @file lcb_receive_data_buffer.c
 * @brief LCB Receive Data Buffer Implementation
 *
 * Converted from: com/jht/lcbcommlib/ReceiveDataBuffer.java
 */

#include "lcb_receive_data_buffer.h"
#include <stdlib.h>
#include <string.h>
#include "lcb_logging.h"

#define TAG "lcb_rx_buffer"

void lcb_receive_data_buffer_init(lcb_receive_data_buffer_t *buf)
{
    if (buf == NULL) {
        return;
    }
    buf->data = NULL;
    buf->length = 0;
}

void lcb_receive_data_buffer_free(lcb_receive_data_buffer_t *buf)
{
    if (buf == NULL) {
        return;
    }
    if (buf->data != NULL) {
        free(buf->data);
        buf->data = NULL;
    }
    buf->length = 0;
}

int lcb_receive_data_buffer_clone(lcb_receive_data_buffer_t *dest,
                                   const uint8_t *data, uint32_t length)
{
    if (dest == NULL || data == NULL || length == 0) {
        return -1;
    }

    LCB_LOGD(TAG, "Cloning %u bytes into buffer", length);

    /* Allocate buffer (Java: allocBufferSize(bufferLen)) */
    uint8_t *new_data = (uint8_t*)malloc(length);
    if (new_data == NULL) {
        LCB_LOGE(TAG, "Failed to allocate %u bytes for buffer", length);
        return -1;
    }

    /* Copy data (Java: for loop byte-by-byte copy) */
    memcpy(new_data, data, length);

    /* Free any existing data in dest */
    if (dest->data != NULL) {
        LCB_LOGD(TAG, "Freed previous buffer (%u bytes)", dest->length);
        free(dest->data);
    }

    dest->data = new_data;
    dest->length = length;

    LCB_LOGD(TAG, "Buffer cloned: %u bytes", length);
    return 0;
}

const uint8_t* lcb_receive_data_buffer_get_data(const lcb_receive_data_buffer_t *buf)
{
    if (buf == NULL) {
        return NULL;
    }
    return buf->data;
}

uint32_t lcb_receive_data_buffer_get_length(const lcb_receive_data_buffer_t *buf)
{
    if (buf == NULL) {
        return 0;
    }
    return buf->length;
}
