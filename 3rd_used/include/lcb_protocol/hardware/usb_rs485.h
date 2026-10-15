/**
 * @file usb_rs485.h
 * @brief Layer_5 hardware — one-call USB RS485 port for lcb_comm
 *
 * Finds a supported USB-to-RS485 adapter (CH340 / CH341), opens its usbdevfs
 * node, initializes the chip and starts a usb_serial_bridge. The returned fd
 * goes into serial_port_t.fd, so lcb_comm and the handlers run over USB
 * unchanged:
 *
 *   usb_rs485_t *usb = usb_rs485_open(NULL, 9600);     // NULL = auto-detect
 *   port.fd = usb_rs485_get_fd(usb);
 *   general_handler_auto_start(gh, comm, &port);
 *   ...
 *   general_handler_auto_stop(gh);                     // stops lcb_comm
 *   usb_rs485_close(usb);                              // only after lcb_comm stopped
 *
 * Requires read/write access to /dev/bus/usb/BBB/DDD (root or a udev rule).
 * A kernel driver bound to the adapter (ch341 -> /dev/ttyUSBn) is detached
 * while the port is open. RS485 direction switching is done by the adapter.
 */

#ifndef USB_RS485_H
#define USB_RS485_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "ch340.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Buffer size for a usbdevfs node path ("/dev/bus/usb/BBB/DDD") */
#define USB_RS485_NODE_MAX 64

typedef struct usb_rs485 usb_rs485_t;

/**
 * @brief Find the first supported adapter by scanning /sys/bus/usb/devices.
 * @param node     Receives the usbdevfs node, e.g. "/dev/bus/usb/001/004"
 * @param node_len Size of node (USB_RS485_NODE_MAX is enough)
 * @return 0 when found, -ENODEV when no adapter is attached, other -errno on error
 */
int usb_rs485_find(char *node, size_t node_len);

/**
 * @brief Open the adapter and start the bridge (8N1, DTR and RTS on).
 * @param usb_node  usbdevfs node, or NULL / "" to use usb_rs485_find()
 * @param baud_rate Baud rate, 0 = 9600 (LCB default)
 * @return Handle, or NULL with errno set: ENODEV (no adapter), EACCES/EPERM
 *         (no permission on the node), ENOTSUP (not a supported chip),
 *         EIO (chip initialization or bridge start failed)
 */
usb_rs485_t* usb_rs485_open(const char *usb_node, uint32_t baud_rate);

/**
 * @brief Close the bridge, release the chip and close the usbdevfs fd.
 *
 * Call only after lcb_comm has stopped using the fd (lcb_comm_stop() or
 * general_handler_auto_stop()); the fd from usb_rs485_get_fd() is closed here.
 */
void usb_rs485_close(usb_rs485_t *port);

/**
 * @brief Stream fd for serial_port_t.fd (non-blocking socket).
 *        Owned by the handle: do not close it. -1 if port is NULL.
 */
int usb_rs485_get_fd(const usb_rs485_t *port);

/** @brief False after a USB I/O error, e.g. the adapter was unplugged. */
bool usb_rs485_is_healthy(const usb_rs485_t *port);

/** @brief First fatal USB error (-errno), 0 while healthy. */
int usb_rs485_get_error(const usb_rs485_t *port);

/** @brief usbdevfs node in use, e.g. "/dev/bus/usb/001/004". */
const char* usb_rs485_get_node(const usb_rs485_t *port);

/** @brief Underlying CH340 driver, for control lines / modem status (advanced). */
ch340_t* usb_rs485_get_ch340(const usb_rs485_t *port);

#ifdef __cplusplus
}
#endif

#endif /* USB_RS485_H */
