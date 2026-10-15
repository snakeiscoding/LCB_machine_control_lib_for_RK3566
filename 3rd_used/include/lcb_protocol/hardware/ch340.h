/**
 * @file ch340.h
 * @brief Layer_5 hardware — CH340 / CH341 USB-to-serial (RS485) driver
 *
 * Protocol reference (not linked, re-implemented in C):
 *   - SimpleUsbTerminal (kai-morich) connect flow: TerminalFragment.connect(),
 *     SerialSocket.connect()/disconnect()
 *   - Ch34xSerialDriver.java / CommonUsbSerialPort.java of the CH34x driver
 *     used by SimpleUsbTerminal (vendor requests 0x5F/0x95/0x9A/0xA1/0xA4)
 *
 * The driver talks to the chip through usb_host_ops_t, normally backed by a
 * usbdevfs fd (open("/dev/bus/usb/BBB/DDD")). RS485 direction switching is
 * done by the adapter hardware (auto-direction), so no RTS toggling per frame.
 *
 * Most applications use usb_rs485.h instead, which finds the adapter, opens
 * it and returns an fd for serial_port_t in one call.
 *
 * Threading: control-line calls are serialized by an internal mutex.
 * ch340_read() and ch340_write() may run concurrently with each other (one
 * reader thread + one writer thread), but each must have a single caller.
 */

#ifndef CH340_H
#define CH340_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "usb_host.h"
#include "usb_serial_bridge.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * USB IDs
 * ============================================================================ */

#define CH340_VENDOR_ID            0x1A86
#define CH340_PRODUCT_ID_CH340     0x7523
#define CH340_PRODUCT_ID_CH341A    0x5523

/* ============================================================================
 * Line settings
 * ============================================================================ */

typedef enum {
    CH340_PARITY_NONE  = 0,
    CH340_PARITY_ODD   = 1,
    CH340_PARITY_EVEN  = 2,
    CH340_PARITY_MARK  = 3,
    CH340_PARITY_SPACE = 4,
} ch340_parity_t;

/* Modem status bits returned by ch340_get_modem_status() (1 = asserted) */
#define CH340_MODEM_CTS 0x01
#define CH340_MODEM_DSR 0x02
#define CH340_MODEM_RI  0x04
#define CH340_MODEM_CD  0x08

typedef struct ch340_config {
    uint32_t       baud_rate;   /* e.g. 9600 */
    uint8_t        data_bits;   /* 5..8 */
    uint8_t        stop_bits;   /* 1 or 2 (1.5 is not supported) */
    ch340_parity_t parity;
    bool           dtr;         /* DTR state after open */
    bool           rts;         /* RTS state after open */
} ch340_config_t;

/* ============================================================================
 * Opaque Type
 * ============================================================================ */

typedef struct ch340 ch340_t;

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

/** @brief Defaults: 9600 8N1, DTR and RTS on (LCB RS485 settings). */
void ch340_config_init(ch340_config_t *cfg);

/** @brief True when vid/pid is a CH340/CH341 handled by this driver. */
bool ch340_is_supported(uint16_t vendor_id, uint16_t product_id);

/**
 * @brief Open a CH340 on a usbdevfs fd.
 *
 * Reads the descriptors, claims the interfaces, runs the chip init sequence
 * and applies cfg. The fd stays owned by the caller.
 *
 * @return Driver instance, or NULL on failure (see the "Ch340" lines on stderr)
 */
ch340_t* ch340_open(int usb_fd, const ch340_config_t *cfg);

/** @brief Same as ch340_open() but on an injected transport (unit tests). */
ch340_t* ch340_open_with_ops(const usb_host_ops_t *ops, void *ctx,
                             const ch340_config_t *cfg);

/** @brief Drop DTR/RTS, release the interfaces and free. Does not close the fd. */
void ch340_close(ch340_t *dev);

/* ============================================================================
 * Configuration / control lines
 * ============================================================================ */

bool ch340_set_parameters(ch340_t *dev, uint32_t baud_rate, uint8_t data_bits,
                          uint8_t stop_bits, ch340_parity_t parity);
bool ch340_set_dtr(ch340_t *dev, bool on);
bool ch340_set_rts(ch340_t *dev, bool on);
bool ch340_get_dtr(ch340_t *dev);
bool ch340_get_rts(ch340_t *dev);
bool ch340_set_break(ch340_t *dev, bool on);

/**
 * @brief Read CTS/DSR/RI/CD.
 * @return Bitmask of CH340_MODEM_* (>= 0), or -errno
 */
int ch340_get_modem_status(ch340_t *dev);

/** @brief Chip version byte read during init (e.g. 0x30), -1 if dev is NULL. */
int ch340_get_chip_version(ch340_t *dev);

/** @brief wMaxPacketSize of the bulk IN endpoint (0 if dev is NULL). */
uint16_t ch340_get_max_packet_size(ch340_t *dev);

/* ============================================================================
 * Data
 * ============================================================================ */

/**
 * @brief Write bytes, one bulk OUT transfer per max-packet-size chunk.
 * @param timeout_ms Timeout applied to each chunk
 * @return Bytes written (may be short on a timeout after the first chunk),
 *         or -errno when nothing could be written
 */
int ch340_write(ch340_t *dev, const uint8_t *data, size_t length, uint32_t timeout_ms);

/**
 * @brief Read up to length bytes.
 *
 * Each USB transfer requests exactly wMaxPacketSize bytes (usbfs discards
 * partially received data when a bulk transfer times out); bytes that do not
 * fit into the caller's buffer are kept for the next call.
 *
 * @return Bytes read, 0 on timeout, or -errno (e.g. -ENODEV after unplug)
 */
int ch340_read(ch340_t *dev, uint8_t *buffer, size_t length, uint32_t timeout_ms);

/**
 * @brief Fill a usb_serial_io_t so usb_serial_bridge can drive this chip.
 */
void ch340_get_serial_io(ch340_t *dev, usb_serial_io_t *io);

#ifdef __cplusplus
}
#endif

#endif /* CH340_H */
