/**
 * @file usb_rs485.c
 * @brief Layer_5 hardware — one-call USB RS485 port (sysfs lookup + chip + bridge)
 */

#include "usb_rs485.h"
#include "usb_host.h"
#include "usb_serial_bridge.h"
#include "hw_logging.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TAG "UsbRs485"

#define SYSFS_USB_DEVICES "/sys/bus/usb/devices"
#define USB_DEVICE_DESC_LEN 18

struct usb_rs485 {
    int                  usb_fd;     /* usbdevfs fd, owned */
    ch340_t             *ch340;
    usb_serial_bridge_t *bridge;
    char                 node[USB_RS485_NODE_MAX];
};

/* ============================================================================
 * Device lookup
 * ============================================================================ */

/* Read a sysfs attribute of dir as a number in the given base. */
static bool read_sysfs_number(const char *dir, const char *attr, int base,
                              unsigned long *out)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, attr);
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
        return false;
    char buf[32];
    bool ok = fgets(buf, sizeof(buf), fp) != NULL;
    fclose(fp);
    if (!ok)
        return false;

    char *end;
    errno = 0;
    *out = strtoul(buf, &end, base);
    return errno == 0 && end != buf;
}

int usb_rs485_find(char *node, size_t node_len)
{
    if (node == NULL || node_len == 0)
        return -EINVAL;

    DIR *dir = opendir(SYSFS_USB_DEVICES);
    if (dir == NULL)
        return -errno;

    int ret = -ENODEV;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;
        char dev[300];
        snprintf(dev, sizeof(dev), "%s/%s", SYSFS_USB_DEVICES, entry->d_name);

        unsigned long vid, pid, bus, num;
        if (!read_sysfs_number(dev, "idVendor", 16, &vid) ||
            !read_sysfs_number(dev, "idProduct", 16, &pid))
            continue;           /* interfaces have no idVendor */
        if (!ch340_is_supported((uint16_t)vid, (uint16_t)pid))
            continue;
        if (!read_sysfs_number(dev, "busnum", 10, &bus) ||
            !read_sysfs_number(dev, "devnum", 10, &num))
            continue;

        int n = snprintf(node, node_len, "/dev/bus/usb/%03lu/%03lu", bus, num);
        ret = (n > 0 && (size_t)n < node_len) ? 0 : -ENAMETOOLONG;
        break;
    }
    closedir(dir);
    return ret;
}

/* Read VID/PID from the device descriptor at the start of a usbdevfs node. */
static int read_vid_pid(int usb_fd, uint16_t *vid, uint16_t *pid)
{
    uint8_t desc[USB_DEVICE_DESC_LEN];
    int n = usb_host_usbdevfs_ops()->read_descriptors(usb_host_fd_to_ctx(usb_fd),
                                                      desc, sizeof(desc));
    if (n < 0)
        return n;
    if (n < USB_DEVICE_DESC_LEN || desc[1] != USB_DESC_TYPE_DEVICE)
        return -EIO;
    *vid = (uint16_t)(desc[8]  | (desc[9]  << 8));
    *pid = (uint16_t)(desc[10] | (desc[11] << 8));
    return 0;
}

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

static usb_rs485_t* open_fail(usb_rs485_t *port, int err)
{
    if (port->usb_fd >= 0)
        close(port->usb_fd);
    free(port);
    errno = err;
    return NULL;
}

usb_rs485_t* usb_rs485_open(const char *usb_node, uint32_t baud_rate)
{
    usb_rs485_t *port = (usb_rs485_t*)calloc(1, sizeof(usb_rs485_t));
    if (port == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    port->usb_fd = -1;

    if (usb_node != NULL && usb_node[0] != '\0') {
        if (strlen(usb_node) >= sizeof(port->node))
            return open_fail(port, ENAMETOOLONG);
        strcpy(port->node, usb_node);
    } else {
        int ret = usb_rs485_find(port->node, sizeof(port->node));
        if (ret < 0) {
            HW_LOGE(TAG, "No USB RS485 adapter (CH340/CH341) found: %s", strerror(-ret));
            return open_fail(port, -ret);
        }
    }

    port->usb_fd = open(port->node, O_RDWR | O_CLOEXEC);
    if (port->usb_fd < 0) {
        int err = errno;
        HW_LOGE(TAG, "open %s: %s", port->node, strerror(err));
        return open_fail(port, err);
    }

    /* Pick the chip driver by VID/PID (only CH340/CH341 so far) */
    uint16_t vid = 0, pid = 0;
    int ret = read_vid_pid(port->usb_fd, &vid, &pid);
    if (ret < 0) {
        HW_LOGE(TAG, "%s: cannot read device descriptor: %s", port->node, strerror(-ret));
        return open_fail(port, -ret);
    }
    if (!ch340_is_supported(vid, pid)) {
        HW_LOGE(TAG, "%s: %04x:%04x is not a supported USB RS485 chip", port->node, vid, pid);
        return open_fail(port, ENOTSUP);
    }

    ch340_config_t cfg;
    ch340_config_init(&cfg);
    if (baud_rate != 0)
        cfg.baud_rate = baud_rate;
    port->ch340 = ch340_open(port->usb_fd, &cfg);
    if (port->ch340 == NULL) {
        HW_LOGE(TAG, "%s: CH340 initialization failed", port->node);
        return open_fail(port, EIO);
    }

    usb_serial_io_t io;
    ch340_get_serial_io(port->ch340, &io);
    port->bridge = usb_serial_bridge_start(&io);
    if (port->bridge == NULL) {
        HW_LOGE(TAG, "%s: bridge start failed", port->node);
        ch340_close(port->ch340);
        return open_fail(port, EIO);
    }

    HW_LOGI(TAG, "USB RS485 ready on %s (%04x:%04x, %u baud, fd %d)", port->node,
            vid, pid, cfg.baud_rate, usb_serial_bridge_get_fd(port->bridge));
    return port;
}

void usb_rs485_close(usb_rs485_t *port)
{
    if (port == NULL) return;

    usb_serial_bridge_stop(port->bridge);
    ch340_close(port->ch340);
    if (port->usb_fd >= 0)
        close(port->usb_fd);
    HW_LOGI(TAG, "USB RS485 closed (%s)", port->node);
    free(port);
}

/* ============================================================================
 * Access
 * ============================================================================ */

int usb_rs485_get_fd(const usb_rs485_t *port)
{
    return port == NULL ? -1 : usb_serial_bridge_get_fd(port->bridge);
}

bool usb_rs485_is_healthy(const usb_rs485_t *port)
{
    return port != NULL && usb_serial_bridge_is_healthy(port->bridge);
}

int usb_rs485_get_error(const usb_rs485_t *port)
{
    return port == NULL ? -EINVAL : usb_serial_bridge_get_error(port->bridge);
}

const char* usb_rs485_get_node(const usb_rs485_t *port)
{
    return port == NULL ? NULL : port->node;
}

ch340_t* usb_rs485_get_ch340(const usb_rs485_t *port)
{
    return port == NULL ? NULL : port->ch340;
}
