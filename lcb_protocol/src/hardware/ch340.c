/**
 * @file ch340.c
 * @brief Layer_5 hardware — CH340 / CH341 USB-to-serial driver
 *
 * Wire protocol re-implemented from the CH34x driver used by
 * SimpleUsbTerminal (Ch34xSerialDriver.java): init, setBaudRate,
 * setParameters, setControlLines, getStatus, setBreak.
 */

#include "ch340_internal.h"
#include "hw_logging.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define TAG "Ch340"

#define CHECK_ANY (-1)  /* "don't care" byte in check_state() */

/* ============================================================================
 * Pure helpers
 * ============================================================================ */

bool ch340_calc_baud_divisor(uint32_t baud_rate, uint16_t *factor_value,
                             uint16_t *offset_value)
{
    if (baud_rate == 0 || factor_value == NULL || offset_value == NULL)
        return false;

    uint32_t factor;
    uint32_t divisor;

    if (baud_rate == 921600) {
        divisor = 7;
        factor  = 0xF300;
    } else {
        uint64_t f = CH340_BAUDBASE_FACTOR / baud_rate;
        divisor = CH340_BAUDBASE_DIVMAX;
        while (f > 0xFFF0 && divisor > 0) {
            f >>= 3;
            divisor--;
        }
        if (f > 0xFFF0 || f == 0)
            return false;
        factor = (uint32_t)(0x10000 - f);
    }

    divisor |= 0x0080;  /* else ch341a waits until buffer full */
    *factor_value = (uint16_t)((factor & 0xFF00) | divisor);
    *offset_value = (uint16_t)(factor & 0x00FF);
    return true;
}

bool ch340_calc_lcr(uint8_t data_bits, uint8_t stop_bits, ch340_parity_t parity,
                    uint8_t *lcr)
{
    if (lcr == NULL)
        return false;

    uint8_t value = CH340_LCR_ENABLE_RX | CH340_LCR_ENABLE_TX;

    switch (data_bits) {
    case 5: value |= CH340_LCR_CS5; break;
    case 6: value |= CH340_LCR_CS6; break;
    case 7: value |= CH340_LCR_CS7; break;
    case 8: value |= CH340_LCR_CS8; break;
    default: return false;
    }

    switch (parity) {
    case CH340_PARITY_NONE:
        break;
    case CH340_PARITY_ODD:
        value |= CH340_LCR_ENABLE_PAR;
        break;
    case CH340_PARITY_EVEN:
        value |= CH340_LCR_ENABLE_PAR | CH340_LCR_PAR_EVEN;
        break;
    case CH340_PARITY_MARK:
        value |= CH340_LCR_ENABLE_PAR | CH340_LCR_MARK_SPACE;
        break;
    case CH340_PARITY_SPACE:
        value |= CH340_LCR_ENABLE_PAR | CH340_LCR_MARK_SPACE | CH340_LCR_PAR_EVEN;
        break;
    default:
        return false;
    }

    switch (stop_bits) {
    case 1: break;
    case 2: value |= CH340_LCR_STOP_BITS_2; break;
    default: return false;  /* 1.5 stop bits unsupported */
    }

    *lcr = value;
    return true;
}

uint16_t ch340_control_lines_value(bool dtr, bool rts)
{
    uint16_t bits = (uint16_t)((dtr ? CH340_SCL_DTR : 0) | (rts ? CH340_SCL_RTS : 0));
    return (uint16_t)~bits;
}

uint8_t ch340_decode_modem_status(uint8_t raw)
{
    uint8_t lines = 0;
    if ((raw & CH340_GCL_CTS) == 0) lines |= CH340_MODEM_CTS;
    if ((raw & CH340_GCL_DSR) == 0) lines |= CH340_MODEM_DSR;
    if ((raw & CH340_GCL_RI)  == 0) lines |= CH340_MODEM_RI;
    if ((raw & CH340_GCL_CD)  == 0) lines |= CH340_MODEM_CD;
    return lines;
}

/* ============================================================================
 * Transfers
 * ============================================================================ */

static int control_out(ch340_t *dev, uint8_t request, uint16_t value, uint16_t index)
{
    return dev->ops->control(dev->ctx, USB_DIR_OUT_VENDOR, request, value, index,
                             NULL, 0, CH340_USB_TIMEOUT_MS);
}

static int control_in(ch340_t *dev, uint8_t request, uint16_t value, uint16_t index,
                      uint8_t *buffer, uint16_t length)
{
    return dev->ops->control(dev->ctx, USB_DIR_IN_VENDOR, request, value, index,
                             buffer, length, CH340_USB_TIMEOUT_MS);
}

/* Read 2 bytes and compare them with expected (CHECK_ANY = don't care). */
static bool check_state(ch340_t *dev, const char *msg, uint8_t request,
                        uint16_t value, const int expected[2], uint8_t out[2])
{
    uint8_t buffer[2] = {0, 0};
    int ret = control_in(dev, request, value, 0, buffer, sizeof(buffer));
    if (ret != (int)sizeof(buffer)) {
        HW_LOGE(TAG, "%s: control in 0x%02X/0x%04X returned %d", msg, request, value, ret);
        return false;
    }
    for (int i = 0; i < 2; i++) {
        if (expected[i] != CHECK_ANY && buffer[i] != (uint8_t)expected[i]) {
            HW_LOGE(TAG, "%s: byte %d = 0x%02X, expected 0x%02X", msg, i, buffer[i], expected[i]);
            return false;
        }
    }
    if (out != NULL)
        memcpy(out, buffer, sizeof(buffer));
    return true;
}

static bool set_baud_rate(ch340_t *dev, uint32_t baud_rate)
{
    uint16_t factor_value, offset_value;
    if (!ch340_calc_baud_divisor(baud_rate, &factor_value, &offset_value)) {
        HW_LOGE(TAG, "Unsupported baud rate: %u", baud_rate);
        return false;
    }
    if (control_out(dev, CH340_REQ_WRITE_REG, CH340_REG_BAUD_FACTOR, factor_value) < 0 ||
        control_out(dev, CH340_REQ_WRITE_REG, CH340_REG_BAUD_OFFSET, offset_value) < 0) {
        HW_LOGE(TAG, "Error setting baud rate %u", baud_rate);
        return false;
    }
    return true;
}

static bool write_control_lines(ch340_t *dev, bool dtr, bool rts)
{
    if (control_out(dev, CH340_REQ_MODEM_CTRL, ch340_control_lines_value(dtr, rts), 0) < 0) {
        HW_LOGE(TAG, "Failed to set control lines dtr=%d rts=%d", dtr, rts);
        return false;
    }
    dev->dtr = dtr;
    dev->rts = rts;
    return true;
}

/* Init sequence (Ch34xSerialDriver.initialize) */
static bool chip_initialize(ch340_t *dev, uint32_t baud_rate)
{
    static const int expect_version[2] = { CHECK_ANY, 0x00 };
    static const int expect_lcr[2]     = { CHECK_ANY, 0x00 };
    static const int expect_any[2]     = { CHECK_ANY, CHECK_ANY };
    uint8_t version[2];

    if (!check_state(dev, "init #1", CH340_REQ_READ_VERSION, 0, expect_version, version))
        return false;
    dev->chip_version = version[0];

    if (control_out(dev, CH340_REQ_SERIAL_INIT, 0, 0) < 0) {
        HW_LOGE(TAG, "Init failed: #2");
        return false;
    }
    if (!set_baud_rate(dev, baud_rate))
        return false;
    if (!check_state(dev, "init #4", CH340_REQ_READ_REG, CH340_REG_LCR, expect_lcr, NULL))
        return false;
    if (control_out(dev, CH340_REQ_WRITE_REG, CH340_REG_LCR,
                    CH340_LCR_ENABLE_RX | CH340_LCR_ENABLE_TX | CH340_LCR_CS8) < 0) {
        HW_LOGE(TAG, "Init failed: #5");
        return false;
    }
    if (!check_state(dev, "init #6", CH340_REQ_READ_REG, CH340_REG_MODEM_STATUS, expect_any, NULL))
        return false;
    if (control_out(dev, CH340_REQ_SERIAL_INIT, CH340_SERIAL_INIT_VALUE,
                    CH340_SERIAL_INIT_INDEX) < 0) {
        HW_LOGE(TAG, "Init failed: #7");
        return false;
    }
    if (!set_baud_rate(dev, baud_rate))
        return false;
    if (!write_control_lines(dev, false, false))
        return false;
    if (!check_state(dev, "init #10", CH340_REQ_READ_REG, CH340_REG_MODEM_STATUS, expect_any, NULL))
        return false;
    return true;
}

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

void ch340_config_init(ch340_config_t *cfg)
{
    if (cfg == NULL) return;
    cfg->baud_rate = 9600;
    cfg->data_bits = 8;
    cfg->stop_bits = 1;
    cfg->parity    = CH340_PARITY_NONE;
    cfg->dtr       = true;
    cfg->rts       = true;
}

bool ch340_is_supported(uint16_t vendor_id, uint16_t product_id)
{
    return vendor_id == CH340_VENDOR_ID &&
           (product_id == CH340_PRODUCT_ID_CH340 || product_id == CH340_PRODUCT_ID_CH341A);
}

static void release_interfaces(ch340_t *dev)
{
    for (int i = 0; i < dev->claimed_count; i++) {
        if (dev->ops->release_interface != NULL)
            dev->ops->release_interface(dev->ctx, i);
    }
    dev->claimed_count = 0;
}

ch340_t* ch340_open_with_ops(const usb_host_ops_t *ops, void *ctx,
                             const ch340_config_t *cfg)
{
    if (ops == NULL || ops->control == NULL || ops->bulk == NULL || ops->claim_interface == NULL)
        return NULL;

    ch340_config_t config;
    if (cfg != NULL)
        config = *cfg;
    else
        ch340_config_init(&config);

    /* Validate settings before touching the device */
    uint16_t factor_value, offset_value;
    uint8_t lcr;
    if (!ch340_calc_baud_divisor(config.baud_rate, &factor_value, &offset_value) ||
        !ch340_calc_lcr(config.data_bits, config.stop_bits, config.parity, &lcr)) {
        HW_LOGE(TAG, "Invalid config %u %u%c%u", config.baud_rate, config.data_bits,
                "NOEMS"[config.parity <= CH340_PARITY_SPACE ? config.parity : 0],
                config.stop_bits);
        return NULL;
    }

    ch340_t *dev = (ch340_t*)calloc(1, sizeof(ch340_t));
    if (dev == NULL)
        return NULL;
    dev->ops = ops;
    dev->ctx = ctx;
    pthread_mutex_init(&dev->ctrl_mutex, NULL);

    /* Endpoints */
    usb_bulk_endpoints_set_default(&dev->eps);
    if (ops->read_descriptors != NULL) {
        uint8_t raw[USB_HOST_MAX_DESCRIPTORS];
        int n = ops->read_descriptors(ctx, raw, sizeof(raw));
        if (n <= 0 || !usb_desc_find_bulk_endpoints(raw, (size_t)n, &dev->eps)) {
            HW_LOGE(TAG, "Descriptor parse failed (%d), using default endpoints", n);
            usb_bulk_endpoints_set_default(&dev->eps);
        }
    }
    if (dev->eps.max_packet_size == 0 || dev->eps.max_packet_size > CH340_MAX_PACKET)
        dev->eps.max_packet_size = dev->eps.max_packet_size == 0 ? 32 : CH340_MAX_PACKET;

    /* Claim every interface (force) */
    for (int i = 0; i < dev->eps.interface_count; i++) {
        int ret = ops->claim_interface(ctx, i);
        if (ret < 0) {
            HW_LOGE(TAG, "Could not claim interface %d: %d", i, ret);
            release_interfaces(dev);
            pthread_mutex_destroy(&dev->ctrl_mutex);
            free(dev);
            return NULL;
        }
        dev->claimed_count = i + 1;
    }

    if (!chip_initialize(dev, config.baud_rate) ||
        !ch340_set_parameters(dev, config.baud_rate, config.data_bits,
                              config.stop_bits, config.parity) ||
        !write_control_lines(dev, config.dtr, config.rts)) {
        release_interfaces(dev);
        pthread_mutex_destroy(&dev->ctrl_mutex);
        free(dev);
        return NULL;
    }

    HW_LOGI(TAG, "CH340 opened: version=0x%02X iface=%d in=0x%02X out=0x%02X mps=%u baud=%u",
            dev->chip_version, dev->eps.interface_number, dev->eps.ep_in,
            dev->eps.ep_out, dev->eps.max_packet_size, config.baud_rate);
    return dev;
}

ch340_t* ch340_open(int usb_fd, const ch340_config_t *cfg)
{
    if (usb_fd < 0)
        return NULL;
    return ch340_open_with_ops(usb_host_usbdevfs_ops(), usb_host_fd_to_ctx(usb_fd), cfg);
}

void ch340_close(ch340_t *dev)
{
    if (dev == NULL) return;

    pthread_mutex_lock(&dev->ctrl_mutex);
    write_control_lines(dev, false, false);  /* best effort (device may be gone) */
    pthread_mutex_unlock(&dev->ctrl_mutex);

    release_interfaces(dev);
    pthread_mutex_destroy(&dev->ctrl_mutex);
    free(dev);
    HW_LOGI(TAG, "CH340 closed");
}

/* ============================================================================
 * Configuration / control lines
 * ============================================================================ */

bool ch340_set_parameters(ch340_t *dev, uint32_t baud_rate, uint8_t data_bits,
                          uint8_t stop_bits, ch340_parity_t parity)
{
    if (dev == NULL) return false;

    uint8_t lcr;
    if (!ch340_calc_lcr(data_bits, stop_bits, parity, &lcr)) {
        HW_LOGE(TAG, "Unsupported line settings %u data, %u stop, parity %d",
                data_bits, stop_bits, parity);
        return false;
    }

    pthread_mutex_lock(&dev->ctrl_mutex);
    bool ok = set_baud_rate(dev, baud_rate);
    if (ok && control_out(dev, CH340_REQ_WRITE_REG, CH340_REG_LCR, lcr) < 0) {
        HW_LOGE(TAG, "Error setting control byte 0x%02X", lcr);
        ok = false;
    }
    pthread_mutex_unlock(&dev->ctrl_mutex);
    return ok;
}

bool ch340_set_dtr(ch340_t *dev, bool on)
{
    if (dev == NULL) return false;
    pthread_mutex_lock(&dev->ctrl_mutex);
    bool ok = write_control_lines(dev, on, dev->rts);
    pthread_mutex_unlock(&dev->ctrl_mutex);
    return ok;
}

bool ch340_set_rts(ch340_t *dev, bool on)
{
    if (dev == NULL) return false;
    pthread_mutex_lock(&dev->ctrl_mutex);
    bool ok = write_control_lines(dev, dev->dtr, on);
    pthread_mutex_unlock(&dev->ctrl_mutex);
    return ok;
}

bool ch340_get_dtr(ch340_t *dev)
{
    return dev != NULL && dev->dtr;
}

bool ch340_get_rts(ch340_t *dev)
{
    return dev != NULL && dev->rts;
}

bool ch340_set_break(ch340_t *dev, bool on)
{
    if (dev == NULL) return false;

    pthread_mutex_lock(&dev->ctrl_mutex);
    uint8_t req[2] = {0, 0};
    bool ok = control_in(dev, CH340_REQ_READ_REG, CH340_REG_BREAK, 0, req, sizeof(req)) == (int)sizeof(req);
    if (ok) {
        if (on) {
            req[0] &= (uint8_t)~1;
            req[1] &= (uint8_t)~0x40;
        } else {
            req[0] |= 1;
            req[1] |= 0x40;
        }
        uint16_t value = (uint16_t)((req[1] << 8) | req[0]);
        ok = control_out(dev, CH340_REQ_WRITE_REG, CH340_REG_BREAK, value) >= 0;
    }
    pthread_mutex_unlock(&dev->ctrl_mutex);
    if (!ok)
        HW_LOGE(TAG, "Error setting break %d", on);
    return ok;
}

int ch340_get_modem_status(ch340_t *dev)
{
    if (dev == NULL) return -EINVAL;

    uint8_t buffer[2] = {0, 0};
    pthread_mutex_lock(&dev->ctrl_mutex);
    int ret = control_in(dev, CH340_REQ_READ_REG, CH340_REG_MODEM_STATUS, 0, buffer, sizeof(buffer));
    pthread_mutex_unlock(&dev->ctrl_mutex);
    if (ret < 0)
        return ret;
    if (ret != (int)sizeof(buffer))
        return -EIO;
    return ch340_decode_modem_status(buffer[0]);
}

int ch340_get_chip_version(ch340_t *dev)
{
    return dev == NULL ? -1 : dev->chip_version;
}

uint16_t ch340_get_max_packet_size(ch340_t *dev)
{
    return dev == NULL ? 0 : dev->eps.max_packet_size;
}

/* ============================================================================
 * Data
 * ============================================================================ */

int ch340_write(ch340_t *dev, const uint8_t *data, size_t length, uint32_t timeout_ms)
{
    if (dev == NULL || (data == NULL && length > 0))
        return -EINVAL;

    size_t written = 0;
    while (written < length) {
        size_t chunk = length - written;
        if (chunk > dev->eps.max_packet_size)
            chunk = dev->eps.max_packet_size;
        memcpy(dev->tx_buf, data + written, chunk);

        int n = dev->ops->bulk(dev->ctx, dev->eps.ep_out, dev->tx_buf, (uint32_t)chunk, timeout_ms);
        if (n <= 0) {
            if (written > 0)
                break;
            return n < 0 ? n : -ETIMEDOUT;
        }
        written += (size_t)n;
    }
    return (int)written;
}

int ch340_read(ch340_t *dev, uint8_t *buffer, size_t length, uint32_t timeout_ms)
{
    if (dev == NULL || buffer == NULL)
        return -EINVAL;
    if (length == 0)
        return 0;

    if (dev->rx_len == 0) {
        int n = dev->ops->bulk(dev->ctx, dev->eps.ep_in, dev->rx_buf,
                               dev->eps.max_packet_size, timeout_ms);
        if (n == -ETIMEDOUT)
            return 0;
        if (n <= 0)
            return n;
        dev->rx_off = 0;
        dev->rx_len = (size_t)n;
    }

    size_t take = length < dev->rx_len ? length : dev->rx_len;
    memcpy(buffer, dev->rx_buf + dev->rx_off, take);
    dev->rx_off += take;
    dev->rx_len -= take;
    return (int)take;
}

static int serial_io_read(void *dev, uint8_t *buffer, size_t length, uint32_t timeout_ms)
{
    return ch340_read((ch340_t*)dev, buffer, length, timeout_ms);
}

static int serial_io_write(void *dev, const uint8_t *data, size_t length, uint32_t timeout_ms)
{
    return ch340_write((ch340_t*)dev, data, length, timeout_ms);
}

void ch340_get_serial_io(ch340_t *dev, usb_serial_io_t *io)
{
    if (io == NULL) return;
    io->dev             = dev;
    io->read            = serial_io_read;
    io->write           = serial_io_write;
    io->max_packet_size = ch340_get_max_packet_size(dev);
}
