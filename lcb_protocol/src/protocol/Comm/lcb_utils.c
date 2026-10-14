/**
 * @file lcb_utils.c
 * @brief LCB Protocol Utility Functions Implementation
 */

#include "lcb_utils.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/**
 * @brief Convert byte array to hex string
 */
int lcb_bytes_to_hex_string(const uint8_t *data, size_t length,
                             char *output, size_t output_size)
{
    if (data == NULL || output == NULL || output_size == 0) {
        return -1;
    }

    size_t required_size = (length * 3) + 1; /* "XX " for each byte + null */
    if (output_size < required_size) {
        return -2;
    }

    size_t pos = 0;
    for (size_t i = 0; i < length; i++) {
        int written = snprintf(&output[pos], output_size - pos, "%02X ", data[i]);
        if (written < 0 || written >= (int)(output_size - pos)) {
            return -3;
        }
        pos += written;
    }

    /* Remove trailing space */
    if (pos > 0 && output[pos - 1] == ' ') {
        output[pos - 1] = '\0';
        pos--;
    }

    return (int)pos;
}

/**
 * @brief Convert hex character to value
 */
static int hex_char_to_value(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return -1;
}

/**
 * @brief Convert hex string to byte array
 */
int lcb_hex_string_to_bytes(const char *hex_string, uint8_t *data, size_t max_length)
{
    if (hex_string == NULL || data == NULL || max_length == 0) {
        return -1;
    }

    size_t len = strlen(hex_string);
    size_t byte_count = 0;
    size_t i = 0;

    while (i < len && byte_count < max_length) {
        /* Skip whitespace */
        while (i < len && isspace(hex_string[i])) {
            i++;
        }

        if (i >= len) {
            break;
        }

        /* Read two hex digits */
        int high = hex_char_to_value(hex_string[i]);
        if (high < 0) {
            return -2; /* Invalid hex character */
        }
        i++;

        if (i >= len) {
            return -3; /* Incomplete byte */
        }

        int low = hex_char_to_value(hex_string[i]);
        if (low < 0) {
            return -2; /* Invalid hex character */
        }
        i++;

        data[byte_count] = (uint8_t)((high << 4) | low);
        byte_count++;
    }

    return (int)byte_count;
}
