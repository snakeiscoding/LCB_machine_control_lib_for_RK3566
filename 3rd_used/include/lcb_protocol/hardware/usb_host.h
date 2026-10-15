/**
 * @file usb_host.h
 * @brief Layer_5 hardware — USB host transport over Linux usbdevfs
 *
 * The usbdevfs file descriptor comes from open("/dev/bus/usb/BBB/DDD", O_RDWR)
 * (usb_rs485_find() / usb_rs485_open() locate and open it). Plain read()/write()
 * on that fd do not move serial data; transfers go through ioctl(USBDEVFS_*).
 * This module wraps those ioctls behind a small ops table so chip drivers
 * (ch340.c, ...) can be unit tested with a fake transport.
 *
 * Ported from the Android app, where the fd came from UsbDeviceConnection:
 *   UsbDeviceConnection             ->  usbdevfs
 *   controlTransfer()               ->  USBDEVFS_CONTROL
 *   bulkTransfer()                  ->  USBDEVFS_BULK
 *   claimInterface(iface, true)     ->  USBDEVFS_IOCTL{DISCONNECT} + USBDEVFS_CLAIMINTERFACE
 *   releaseInterface()              ->  USBDEVFS_RELEASEINTERFACE
 *   getRawDescriptors()             ->  lseek(fd, 0) + read(fd)
 */

#ifndef USB_HOST_H
#define USB_HOST_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Constants
 * ============================================================================ */

#define USB_DIR_OUT_VENDOR      0x40    /* bmRequestType: vendor, host-to-device */
#define USB_DIR_IN_VENDOR       0xC0    /* bmRequestType: vendor, device-to-host */

#define USB_ENDPOINT_DIR_IN     0x80
#define USB_DESC_TYPE_DEVICE    0x01
#define USB_DESC_TYPE_CONFIG    0x02
#define USB_DESC_TYPE_INTERFACE 0x04
#define USB_DESC_TYPE_ENDPOINT  0x05
#define USB_ENDPOINT_XFER_MASK  0x03
#define USB_ENDPOINT_XFER_BULK  0x02

#define USB_HOST_MAX_INTERFACES 8
#define USB_HOST_MAX_DESCRIPTORS 1024

/* ============================================================================
 * Transport Ops (injectable)
 * ============================================================================ */

/**
 * @brief USB transport operations.
 *
 * Every function returns the number of bytes transferred (control/bulk/
 * read_descriptors) or 0 (claim/release) on success, and -errno on failure.
 * A bulk transfer that times out returns -ETIMEDOUT.
 */
typedef struct usb_host_ops {
    int (*control)(void *ctx, uint8_t request_type, uint8_t request,
                   uint16_t value, uint16_t index,
                   uint8_t *data, uint16_t length, uint32_t timeout_ms);
    int (*bulk)(void *ctx, uint8_t endpoint, uint8_t *data, uint32_t length,
                uint32_t timeout_ms);
    int (*claim_interface)(void *ctx, int interface_number);
    int (*release_interface)(void *ctx, int interface_number);
    int (*read_descriptors)(void *ctx, uint8_t *buffer, size_t length);
} usb_host_ops_t;

/**
 * @brief Default ops backed by usbdevfs ioctls.
 *
 * The ctx argument is the usbdevfs fd cast with usb_host_fd_to_ctx().
 */
const usb_host_ops_t* usb_host_usbdevfs_ops(void);

/** @brief Pack a usbdevfs fd into an ops ctx pointer. */
void* usb_host_fd_to_ctx(int fd);

/* ============================================================================
 * Descriptor Parsing (pure, unit tested)
 * ============================================================================ */

typedef struct usb_bulk_endpoints {
    int      interface_number;   /* interface holding the bulk pair */
    int      interface_count;    /* interfaces in the active configuration */
    uint8_t  ep_in;              /* bulk IN endpoint address (0x8X) */
    uint8_t  ep_out;             /* bulk OUT endpoint address (0x0X) */
    uint16_t max_packet_size;    /* wMaxPacketSize of the bulk IN endpoint */
} usb_bulk_endpoints_t;

/**
 * @brief Find the bulk IN/OUT endpoint pair in raw descriptors.
 *
 * Walks the device + first configuration descriptor (alternate setting 0
 * only) and picks the first bulk IN and bulk OUT endpoint of the LAST
 * interface that has both (Java reference rule:
 * dataIface = getInterface(getInterfaceCount() - 1)).
 *
 * @param raw  Raw descriptors (device descriptor followed by configuration)
 * @param len  Number of bytes in raw
 * @param out  Result
 * @return true when both bulk endpoints were found
 */
bool usb_desc_find_bulk_endpoints(const uint8_t *raw, size_t len,
                                  usb_bulk_endpoints_t *out);

/**
 * @brief Fill out with the CH340 defaults (iface 0, IN 0x82, OUT 0x02, 32 bytes).
 */
void usb_bulk_endpoints_set_default(usb_bulk_endpoints_t *out);

#ifdef __cplusplus
}
#endif

#endif /* USB_HOST_H */
