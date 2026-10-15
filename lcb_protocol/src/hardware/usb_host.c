/**
 * @file usb_host.c
 * @brief Layer_5 hardware — usbdevfs transport + descriptor parsing
 */

#include "usb_host.h"
#include "hw_logging.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>

#define TAG "UsbHost"

/* ============================================================================
 * usbdevfs ops
 * ============================================================================ */

static int ctx_to_fd(void *ctx)
{
    return (int)(intptr_t)ctx;
}

void* usb_host_fd_to_ctx(int fd)
{
    return (void*)(intptr_t)fd;
}

static int usbdevfs_ioctl_retry(int fd, unsigned long request, void *arg)
{
    int ret;
    do {
        ret = ioctl(fd, request, arg);
    } while (ret < 0 && errno == EINTR);
    return ret < 0 ? -errno : ret;
}

static int usbdevfs_control(void *ctx, uint8_t request_type, uint8_t request,
                            uint16_t value, uint16_t index,
                            uint8_t *data, uint16_t length, uint32_t timeout_ms)
{
    struct usbdevfs_ctrltransfer ct;
    memset(&ct, 0, sizeof(ct));
    ct.bRequestType = request_type;
    ct.bRequest     = request;
    ct.wValue       = value;
    ct.wIndex       = index;
    ct.wLength      = length;
    ct.timeout      = timeout_ms;
    ct.data         = data;
    return usbdevfs_ioctl_retry(ctx_to_fd(ctx), USBDEVFS_CONTROL, &ct);
}

static int usbdevfs_bulk(void *ctx, uint8_t endpoint, uint8_t *data,
                         uint32_t length, uint32_t timeout_ms)
{
    struct usbdevfs_bulktransfer bt;
    memset(&bt, 0, sizeof(bt));
    bt.ep      = endpoint;
    bt.len     = length;
    bt.timeout = timeout_ms;
    bt.data    = data;
    return usbdevfs_ioctl_retry(ctx_to_fd(ctx), USBDEVFS_BULK, &bt);
}

static int usbdevfs_claim_interface(void *ctx, int interface_number)
{
    int fd = ctx_to_fd(ctx);

    /* Detach a kernel driver (e.g. ch341 -> /dev/ttyUSB0) first, like Java's
     * claimInterface(iface, force = true). ENODATA means no driver bound. */
    struct usbdevfs_ioctl cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.ifno       = interface_number;
    cmd.ioctl_code = USBDEVFS_DISCONNECT;
    cmd.data       = NULL;
    int ret = usbdevfs_ioctl_retry(fd, USBDEVFS_IOCTL, &cmd);
    if (ret < 0 && ret != -ENODATA) {
        HW_LOGD(TAG, "disconnect kernel driver iface %d: %d (ignored)", interface_number, ret);
    }

    unsigned int ifno = (unsigned int)interface_number;
    ret = usbdevfs_ioctl_retry(fd, USBDEVFS_CLAIMINTERFACE, &ifno);
    return ret < 0 ? ret : 0;
}

static int usbdevfs_release_interface(void *ctx, int interface_number)
{
    unsigned int ifno = (unsigned int)interface_number;
    int ret = usbdevfs_ioctl_retry(ctx_to_fd(ctx), USBDEVFS_RELEASEINTERFACE, &ifno);
    return ret < 0 ? ret : 0;
}

static int usbdevfs_read_descriptors(void *ctx, uint8_t *buffer, size_t length)
{
    int fd = ctx_to_fd(ctx);
    if (lseek(fd, 0, SEEK_SET) < 0)
        return -errno;
    ssize_t n;
    do {
        n = read(fd, buffer, length);
    } while (n < 0 && errno == EINTR);
    return n < 0 ? -errno : (int)n;
}

static const usb_host_ops_t s_usbdevfs_ops = {
    .control           = usbdevfs_control,
    .bulk              = usbdevfs_bulk,
    .claim_interface   = usbdevfs_claim_interface,
    .release_interface = usbdevfs_release_interface,
    .read_descriptors  = usbdevfs_read_descriptors,
};

const usb_host_ops_t* usb_host_usbdevfs_ops(void)
{
    return &s_usbdevfs_ops;
}

/* ============================================================================
 * Descriptor parsing
 * ============================================================================ */

typedef struct {
    int      number;
    uint8_t  ep_in;
    uint8_t  ep_out;
    uint16_t mps_in;
} iface_eps_t;

void usb_bulk_endpoints_set_default(usb_bulk_endpoints_t *out)
{
    if (out == NULL) return;
    out->interface_number = 0;
    out->interface_count  = 1;
    out->ep_in            = 0x82;
    out->ep_out           = 0x02;
    out->max_packet_size  = 32;
}

bool usb_desc_find_bulk_endpoints(const uint8_t *raw, size_t len,
                                  usb_bulk_endpoints_t *out)
{
    if (raw == NULL || out == NULL)
        return false;

    iface_eps_t ifaces[USB_HOST_MAX_INTERFACES];
    int iface_count = 0;
    int current = -1;           /* index into ifaces, -1 = not in an alt-0 interface */
    bool in_config = false;
    size_t pos = 0;

    memset(ifaces, 0, sizeof(ifaces));

    while (pos + 2 <= len) {
        uint8_t blen = raw[pos];
        uint8_t type = raw[pos + 1];
        if (blen < 2 || pos + blen > len)
            break;              /* truncated or malformed: stop walking */

        if (type == USB_DESC_TYPE_CONFIG) {
            if (in_config)
                break;          /* only the first configuration */
            in_config = true;
        } else if (in_config && type == USB_DESC_TYPE_INTERFACE && blen >= 9) {
            uint8_t number    = raw[pos + 2];
            uint8_t alternate = raw[pos + 3];
            current = -1;
            if (alternate == 0 && iface_count < USB_HOST_MAX_INTERFACES) {
                current = iface_count++;
                ifaces[current].number = number;
            }
        } else if (in_config && type == USB_DESC_TYPE_ENDPOINT && blen >= 7 && current >= 0) {
            uint8_t  address    = raw[pos + 2];
            uint8_t  attributes = raw[pos + 3];
            uint16_t mps        = (uint16_t)(raw[pos + 4] | (raw[pos + 5] << 8));
            if ((attributes & USB_ENDPOINT_XFER_MASK) == USB_ENDPOINT_XFER_BULK) {
                iface_eps_t *it = &ifaces[current];
                if ((address & USB_ENDPOINT_DIR_IN) != 0) {
                    if (it->ep_in == 0) {
                        it->ep_in  = address;
                        it->mps_in = mps;
                    }
                } else if (it->ep_out == 0) {
                    it->ep_out = address;
                }
            }
        }
        pos += blen;
    }

    /* Last interface that carries a complete bulk pair */
    for (int i = iface_count - 1; i >= 0; i--) {
        if (ifaces[i].ep_in != 0 && ifaces[i].ep_out != 0) {
            out->interface_number = ifaces[i].number;
            out->interface_count  = iface_count;
            out->ep_in            = ifaces[i].ep_in;
            out->ep_out           = ifaces[i].ep_out;
            out->max_packet_size  = ifaces[i].mps_in != 0 ? ifaces[i].mps_in : 32;
            return true;
        }
    }
    return false;
}
