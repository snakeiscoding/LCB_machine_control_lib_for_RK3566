/**
 * @file usb_serial_bridge.h
 * @brief Layer_5 hardware — expose a USB serial chip as a plain stream fd
 *
 * protocol/Comm/lcb_comm.c (Layer_4) does read()/write() directly on
 * serial_port_t.fd. A usbdevfs fd cannot be used that way, so the bridge runs
 * two pump threads between the USB driver and one end of a socketpair and
 * hands the other end ("upper fd") to Layer_4. Layer_4 stays unchanged.
 *
 *   lcb_comm  write(upper) -> [socketpair] -> TX pump -> io.write -> bulk OUT
 *   lcb_comm  read(upper)  <- [socketpair] <- RX pump <- io.read  <- bulk IN
 *
 * The upper fd is O_NONBLOCK: lcb_comm's RX thread sees EAGAIN instead of
 * blocking forever, so lcb_comm_stop() can join it.
 */

#ifndef USB_SERIAL_BRIDGE_H
#define USB_SERIAL_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Constants
 * ============================================================================ */

#define USB_SERIAL_BRIDGE_POLL_MS          100   /* pump wake-up / stop latency */
#define USB_SERIAL_BRIDGE_WRITE_TIMEOUT_MS 1000  /* per USB write */
#define USB_SERIAL_BRIDGE_BUFFER_SIZE      512

/* ============================================================================
 * Serial I/O interface (implemented by chip drivers, e.g. ch340)
 * ============================================================================ */

typedef struct usb_serial_io {
    void *dev;
    /** @return bytes read, 0 on timeout, -errno on failure */
    int (*read)(void *dev, uint8_t *buffer, size_t length, uint32_t timeout_ms);
    /** @return bytes written, -errno on failure (-ETIMEDOUT is not fatal) */
    int (*write)(void *dev, const uint8_t *data, size_t length, uint32_t timeout_ms);
    size_t max_packet_size;     /* read chunk size; 0 = USB_SERIAL_BRIDGE_BUFFER_SIZE */
} usb_serial_io_t;

/* ============================================================================
 * Opaque Type
 * ============================================================================ */

typedef struct usb_serial_bridge usb_serial_bridge_t;

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

/**
 * @brief Create the socketpair and start the RX/TX pump threads.
 * @param io Chip driver I/O (copied; io->dev must outlive the bridge)
 * @return Bridge, or NULL on failure
 */
usb_serial_bridge_t* usb_serial_bridge_start(const usb_serial_io_t *io);

/**
 * @brief Stop the pumps (returns within ~USB_SERIAL_BRIDGE_POLL_MS), close
 *        the socketpair and free. Fds obtained from usb_serial_bridge_dup_fd()
 *        stay valid (reads return EOF) until their owner closes them.
 */
void usb_serial_bridge_stop(usb_serial_bridge_t *bridge);

/* ============================================================================
 * Access
 * ============================================================================ */

/** @brief Upper fd owned by the bridge (do not close), -1 if bridge is NULL. */
int usb_serial_bridge_get_fd(usb_serial_bridge_t *bridge);

/**
 * @brief dup() of the upper fd for Layer_4 (serial_port_t.fd). The caller owns
 *        and closes it; it shares the O_NONBLOCK flag.
 * @return New fd, or -errno
 */
int usb_serial_bridge_dup_fd(usb_serial_bridge_t *bridge);

/** @brief False after a USB I/O error (e.g. device unplugged). */
bool usb_serial_bridge_is_healthy(usb_serial_bridge_t *bridge);

/** @brief First fatal error (-errno), 0 when healthy. */
int usb_serial_bridge_get_error(usb_serial_bridge_t *bridge);

#ifdef __cplusplus
}
#endif

#endif /* USB_SERIAL_BRIDGE_H */
