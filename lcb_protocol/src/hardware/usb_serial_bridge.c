/**
 * @file usb_serial_bridge.c
 * @brief Layer_5 hardware — socketpair bridge between a USB serial chip and Layer_4
 */

#define _GNU_SOURCE     /* pthread_setname_np() on glibc */

#include "usb_serial_bridge.h"
#include "hw_logging.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#define TAG "UsbSerialBridge"

struct usb_serial_bridge {
    usb_serial_io_t io;
    int             upper_fd;       /* sv[0] — Layer_4 side (O_NONBLOCK) */
    int             lower_fd;       /* sv[1] — pump side (O_NONBLOCK) */
    pthread_t       rx_thread;
    pthread_t       tx_thread;
    bool            rx_started;
    bool            tx_started;
    atomic_bool     running;
    atomic_int      error;          /* first fatal -errno, 0 = healthy */
};

/* ============================================================================
 * Helpers
 * ============================================================================ */

static void set_error(usb_serial_bridge_t *b, int err)
{
    int expected = 0;
    if (atomic_compare_exchange_strong(&b->error, &expected, err))
        HW_LOGE(TAG, "Bridge I/O error %d (%s)", err, strerror(-err));
}

static bool set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    return flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

/* Push USB RX bytes into the socket; waits for room while running. */
static int send_all(usb_serial_bridge_t *b, const uint8_t *data, size_t length)
{
    size_t sent = 0;
    while (sent < length && atomic_load(&b->running)) {
        ssize_t n = send(b->lower_fd, data + sent, length - sent, MSG_NOSIGNAL);
        if (n > 0) {
            sent += (size_t)n;
        } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            struct pollfd p = { .fd = b->lower_fd, .events = POLLOUT, .revents = 0 };
            poll(&p, 1, USB_SERIAL_BRIDGE_POLL_MS);
        } else if (n < 0 && errno == EINTR) {
            continue;
        } else {
            return n < 0 ? -errno : -EPIPE;
        }
    }
    return 0;
}

/* Write socket bytes to USB. -ETIMEDOUT drops the rest of this chunk. */
static int usb_write_all(usb_serial_bridge_t *b, const uint8_t *data, size_t length)
{
    size_t written = 0;
    while (written < length) {
        int n = b->io.write(b->io.dev, data + written, length - written,
                            USB_SERIAL_BRIDGE_WRITE_TIMEOUT_MS);
        if (n == -ETIMEDOUT || n == 0) {
            HW_LOGE(TAG, "USB write timeout, dropped %zu bytes", length - written);
            return 0;
        }
        if (n < 0)
            return n;
        written += (size_t)n;
    }
    return 0;
}

/* ============================================================================
 * Pump threads
 * ============================================================================ */

static void* rx_pump(void *arg)
{
    usb_serial_bridge_t *b = (usb_serial_bridge_t*)arg;
    uint8_t buffer[USB_SERIAL_BRIDGE_BUFFER_SIZE];
    size_t chunk = b->io.max_packet_size;
    if (chunk == 0 || chunk > sizeof(buffer))
        chunk = sizeof(buffer);

    while (atomic_load(&b->running)) {
        int n = b->io.read(b->io.dev, buffer, chunk, USB_SERIAL_BRIDGE_POLL_MS);
        if (n < 0) {
            set_error(b, n);
            break;
        }
        if (n == 0)
            continue;
        int ret = send_all(b, buffer, (size_t)n);
        if (ret < 0) {
            set_error(b, ret);
            break;
        }
    }
    return NULL;
}

static void* tx_pump(void *arg)
{
    usb_serial_bridge_t *b = (usb_serial_bridge_t*)arg;
    uint8_t buffer[USB_SERIAL_BRIDGE_BUFFER_SIZE];

    while (atomic_load(&b->running)) {
        struct pollfd p = { .fd = b->lower_fd, .events = POLLIN, .revents = 0 };
        int r = poll(&p, 1, USB_SERIAL_BRIDGE_POLL_MS);
        if (r < 0) {
            if (errno == EINTR)
                continue;
            set_error(b, -errno);
            break;
        }
        if (r == 0)
            continue;
        if ((p.revents & POLLIN) == 0) {
            if (p.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                set_error(b, -EPIPE);
                break;
            }
            continue;
        }

        ssize_t n = recv(b->lower_fd, buffer, sizeof(buffer), 0);
        if (n > 0) {
            int ret = usb_write_all(b, buffer, (size_t)n);
            if (ret < 0) {
                set_error(b, ret);
                break;
            }
        } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            set_error(b, -errno);
            break;
        }
    }
    return NULL;
}

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

usb_serial_bridge_t* usb_serial_bridge_start(const usb_serial_io_t *io)
{
    if (io == NULL || io->read == NULL || io->write == NULL)
        return NULL;

    usb_serial_bridge_t *b = (usb_serial_bridge_t*)calloc(1, sizeof(usb_serial_bridge_t));
    if (b == NULL)
        return NULL;
    b->io = *io;
    b->upper_fd = -1;
    b->lower_fd = -1;
    atomic_init(&b->running, true);
    atomic_init(&b->error, 0);

    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv) < 0) {
        HW_LOGE(TAG, "socketpair failed: %s", strerror(errno));
        free(b);
        return NULL;
    }
    b->upper_fd = sv[0];
    b->lower_fd = sv[1];
    if (!set_nonblocking(b->upper_fd) || !set_nonblocking(b->lower_fd)) {
        HW_LOGE(TAG, "fcntl O_NONBLOCK failed: %s", strerror(errno));
        usb_serial_bridge_stop(b);
        return NULL;
    }

    if (pthread_create(&b->rx_thread, NULL, rx_pump, b) != 0) {
        usb_serial_bridge_stop(b);
        return NULL;
    }
    b->rx_started = true;
    pthread_setname_np(b->rx_thread, "usb_bridge_rx");

    if (pthread_create(&b->tx_thread, NULL, tx_pump, b) != 0) {
        usb_serial_bridge_stop(b);
        return NULL;
    }
    b->tx_started = true;
    pthread_setname_np(b->tx_thread, "usb_bridge_tx");

    HW_LOGI(TAG, "Bridge started (upper fd %d)", b->upper_fd);
    return b;
}

void usb_serial_bridge_stop(usb_serial_bridge_t *bridge)
{
    if (bridge == NULL) return;

    atomic_store(&bridge->running, false);
    if (bridge->rx_started)
        pthread_join(bridge->rx_thread, NULL);
    if (bridge->tx_started)
        pthread_join(bridge->tx_thread, NULL);
    if (bridge->lower_fd >= 0)
        close(bridge->lower_fd);
    if (bridge->upper_fd >= 0)
        close(bridge->upper_fd);
    free(bridge);
    HW_LOGI(TAG, "Bridge stopped");
}

/* ============================================================================
 * Access
 * ============================================================================ */

int usb_serial_bridge_get_fd(usb_serial_bridge_t *bridge)
{
    return bridge == NULL ? -1 : bridge->upper_fd;
}

int usb_serial_bridge_dup_fd(usb_serial_bridge_t *bridge)
{
    if (bridge == NULL || bridge->upper_fd < 0)
        return -EBADF;
    int fd = fcntl(bridge->upper_fd, F_DUPFD_CLOEXEC, 0);
    return fd < 0 ? -errno : fd;
}

bool usb_serial_bridge_is_healthy(usb_serial_bridge_t *bridge)
{
    return bridge != NULL && atomic_load(&bridge->error) == 0;
}

int usb_serial_bridge_get_error(usb_serial_bridge_t *bridge)
{
    return bridge == NULL ? -EINVAL : atomic_load(&bridge->error);
}
