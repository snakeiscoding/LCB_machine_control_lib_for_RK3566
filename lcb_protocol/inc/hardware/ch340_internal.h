/**
 * @file ch340_internal.h
 * @brief Internal struct, register constants and pure helpers of ch340.c
 *
 * NOT for external use (not installed) — exposed so the pure helpers can be
 * unit tested.
 */

#ifndef CH340_INTERNAL_H
#define CH340_INTERNAL_H

#include "ch340.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Vendor requests / registers
 * ============================================================================ */

#define CH340_REQ_READ_VERSION   0x5F
#define CH340_REQ_READ_REG       0x95
#define CH340_REQ_WRITE_REG      0x9A
#define CH340_REQ_SERIAL_INIT    0xA1
#define CH340_REQ_MODEM_CTRL     0xA4

#define CH340_REG_BAUD_FACTOR    0x1312
#define CH340_REG_BAUD_OFFSET    0x0F2C
#define CH340_REG_LCR            0x2518
#define CH340_REG_MODEM_STATUS   0x0706
#define CH340_REG_BREAK          0x1805

#define CH340_SERIAL_INIT_VALUE  0x501F
#define CH340_SERIAL_INIT_INDEX  0xD90A

/* Line control register bits */
#define CH340_LCR_ENABLE_RX      0x80
#define CH340_LCR_ENABLE_TX      0x40
#define CH340_LCR_MARK_SPACE     0x20
#define CH340_LCR_PAR_EVEN       0x10
#define CH340_LCR_ENABLE_PAR     0x08
#define CH340_LCR_STOP_BITS_2    0x04
#define CH340_LCR_CS8            0x03
#define CH340_LCR_CS7            0x02
#define CH340_LCR_CS6            0x01
#define CH340_LCR_CS5            0x00

/* Modem control (written inverted, active low) */
#define CH340_SCL_DTR            0x20
#define CH340_SCL_RTS            0x40

/* Modem status (read back, active low) */
#define CH340_GCL_CTS            0x01
#define CH340_GCL_DSR            0x02
#define CH340_GCL_RI             0x04
#define CH340_GCL_CD             0x08

#define CH340_USB_TIMEOUT_MS     5000
#define CH340_BAUDBASE_FACTOR    1532620800UL
#define CH340_BAUDBASE_DIVMAX    3
#define CH340_MAX_PACKET         512

/* ============================================================================
 * Internal Struct Definition
 * ============================================================================ */

struct ch340 {
    /* Transport — NOT owned */
    const usb_host_ops_t *ops;
    void                 *ctx;

    usb_bulk_endpoints_t  eps;
    int                   claimed_count;    /* interfaces 0..claimed_count-1 */
    uint8_t               chip_version;

    /* Control lines (cached, chip cannot be read back) */
    pthread_mutex_t       ctrl_mutex;
    bool                  dtr;
    bool                  rts;

    /* Reader-side leftover buffer (single reader) */
    uint8_t               rx_buf[CH340_MAX_PACKET];
    size_t                rx_off;
    size_t                rx_len;

    /* Writer-side bounce buffer (single writer) */
    uint8_t               tx_buf[CH340_MAX_PACKET];
};

/* ============================================================================
 * Pure helpers (extern, exposed for testing)
 * ============================================================================ */

/**
 * @brief Baud rate -> values for registers 0x1312 (factor|divisor) and 0x0F2C.
 * @return false for unsupported rates (0, too low, too high)
 */
bool ch340_calc_baud_divisor(uint32_t baud_rate, uint16_t *factor_value,
                             uint16_t *offset_value);

/** @brief Data bits / stop bits / parity -> LCR value written to 0x2518. */
bool ch340_calc_lcr(uint8_t data_bits, uint8_t stop_bits, ch340_parity_t parity,
                    uint8_t *lcr);

/** @brief wValue for request 0xA4 (bits inverted: 0xFFFF = both off). */
uint16_t ch340_control_lines_value(bool dtr, bool rts);

/** @brief Raw (active low) status byte -> CH340_MODEM_* bitmask. */
uint8_t ch340_decode_modem_status(uint8_t raw);

#ifdef __cplusplus
}
#endif

#endif /* CH340_INTERNAL_H */
