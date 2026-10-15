/*
 * lcb_demo.c - liblcb_protocol usage example (RK3566 / aarch64 Linux)
 *
 * 1. Opens the LCB link: a UART (9600 8N1, raw, RS-485) or a USB RS485
 *    adapter (CH340/CH341, through usb_rs485_open()).
 * 2. Runs the LCB handshake with general_handler.
 * 3. For -m tm / -m bike, hands the running link over to the machine handler
 *    once the handshake finishes.
 * 4. Prints what the LCB reports, then shuts everything down.
 *
 * It only reads status: it never sends motor-speed or incline commands.
 *
 *   ./lcb_demo [-d device] [-m general|tm|bike] [-t seconds] [-n] [-v]
 *     -d  /dev/ttyS4 (default) or another tty,
 *         usb  = USB RS485 adapter, auto-detected,
 *         usb:/dev/bus/usb/001/004 = USB RS485 adapter on that node
 *     -m  general | tm | bike               (default general)
 *     -t  run time in seconds               (default 10)
 *     -n  do not enable kernel RS-485 mode  (tty only, for RS-232/USB tty adapters)
 *     -v  enable the library's debug log
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/serial.h>

#include "lcb_comm.h"
#include "lcb_logging.h"
#include "general_handler.h"
#include "treadmill_interface.h"
#include "bike_handler.h"
#include "usb_rs485.h"

typedef enum { MODE_GENERAL, MODE_TM, MODE_BIKE } demo_mode_t;

static demo_mode_t g_mode = MODE_GENERAL;

static lcb_comm_t            *g_comm;
static general_handler_t     *g_gh;
static treadmill_interface_t *g_ti;
static bike_handler_t        *g_bh;

static atomic_bool g_initial_finished;
static atomic_bool g_handed_off;          /* gh detached from g_comm */
static volatile sig_atomic_t g_quit;

/* ------------------------------------------------------------------------- */
/* Serial port                                                               */
/* ------------------------------------------------------------------------- */

static int open_lcb_port(const char *path, int use_rs485)
{
    int fd = open(path, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        fprintf(stderr, "open %s: %s\n", path, strerror(errno));
        return -1;
    }

    struct termios tio;
    if (tcgetattr(fd, &tio) != 0) {
        fprintf(stderr, "tcgetattr %s: %s\n", path, strerror(errno));
        close(fd);
        return -1;
    }
    cfmakeraw(&tio);
    tio.c_cflag &= ~(CSIZE | PARENB | CSTOPB | CRTSCTS);
    tio.c_cflag |= CS8 | CREAD | CLOCAL;
    /* read() returns after at most 100 ms without data. lcb_comm's RX thread
     * needs this to notice lcb_comm_stop(); with VMIN=1 it can block forever. */
    tio.c_cc[VMIN]  = 0;
    tio.c_cc[VTIME] = 1;
    cfsetispeed(&tio, B9600);
    cfsetospeed(&tio, B9600);
    if (tcsetattr(fd, TCSANOW, &tio) != 0) {
        fprintf(stderr, "tcsetattr %s: %s\n", path, strerror(errno));
        close(fd);
        return -1;
    }

    if (use_rs485) {
        /* RTS low while sending, high after sending (same as the reference app) */
        struct serial_rs485 rs485;
        memset(&rs485, 0, sizeof(rs485));
        rs485.flags = SER_RS485_ENABLED | SER_RS485_RTS_AFTER_SEND;
        if (ioctl(fd, TIOCSRS485, &rs485) != 0)
            fprintf(stderr, "warning: TIOCSRS485 on %s failed (%s), continuing\n",
                    path, strerror(errno));
    }

    tcflush(fd, TCIOFLUSH);
    return fd;
}

/* ------------------------------------------------------------------------- */
/* Machine handler callbacks (run on library threads: keep them short)       */
/* ------------------------------------------------------------------------- */

static void on_tm_rpm(void *ud, uint16_t rpm)
{
    static int last = -1;
    (void)ud;
    if (rpm != last) {
        printf("[tm] motor RPM %u\n", rpm);
        last = rpm;
    }
}

static void on_tm_incline_adc(void *ud, uint16_t adc)
{
    static int last = -1;
    (void)ud;
    if (adc != last) {
        printf("[tm] incline ADC %u\n", adc);
        last = adc;
    }
}

static void on_tm_safetykey(void *ud, bool locked)
{
    (void)ud;
    printf("[tm] safety key %s\n", locked ? "LOCKED" : "released");
}

static void on_tm_error_code(void *ud, uint16_t code)
{
    (void)ud;
    printf("[tm] LCB error code 0x%04X\n", code);
}

static void on_bike_rpm(void *ud, uint16_t rpm)
{
    static int last = -1;
    (void)ud;
    if (rpm != last) {
        printf("[bike] RPM %u\n", rpm);
        last = rpm;
    }
}

/* ------------------------------------------------------------------------- */
/* Hand-off: called from on_lcb_initial_finish, i.e. on gh's heartbeat thread */
/* ------------------------------------------------------------------------- */

static void hand_off_to_treadmill(LcbType_t type)
{
    /* Never general_handler_auto_stop()/destroy() here: we are running on
     * gh's own thread (self-join deadlock / use-after-free). */
    general_handler_request_stop(g_gh);
    atomic_store(&g_handed_off, true);

    treadmill_interface_t *ti = treadmill_interface_create();
    if (ti == NULL) {
        fprintf(stderr, "treadmill_interface_create failed\n");
        return;
    }
    treadmill_interface_set_lcb_type(ti, type);

    treadmill_interface_callbacks_t cb;
    treadmill_interface_callbacks_init(&cb);
    cb.on_rpm                  = on_tm_rpm;
    cb.on_incline_adc          = on_tm_incline_adc;
    cb.on_safetykey_lock_state = on_tm_safetykey;
    cb.on_error_code           = on_tm_error_code;
    treadmill_interface_set_callbacks(ti, &cb);

    /* Reuses the running g_comm: does not restart serial communication */
    if (!treadmill_interface_start(ti, g_comm)) {
        fprintf(stderr, "treadmill_interface_start failed\n");
        treadmill_interface_destroy(ti);
        return;
    }
    treadmill_interface_set_auto_reply_rpm(ti, true);
    treadmill_interface_set_auto_reply_incline(ti, true);
    g_ti = ti;
    printf("[demo] treadmill_interface attached\n");
}

static void hand_off_to_bike(LcbType_t type)
{
    /* The control type (cmd 0x6C) must be sent while gh still owns the link */
    general_handler_set_control_type(g_gh, GH_CONTROL_TYPE_BIKE);
    general_handler_request_stop(g_gh);
    atomic_store(&g_handed_off, true);

    bike_handler_t *bh = bike_handler_create();
    if (bh == NULL) {
        fprintf(stderr, "bike_handler_create failed\n");
        return;
    }
    bike_handler_set_lcb_type(bh, type);

    bike_handler_callbacks_t cb;
    bike_handler_callbacks_init(&cb);
    cb.on_bike_rpm = on_bike_rpm;
    bike_handler_set_callbacks(bh, &cb);

    if (!bike_handler_start(bh, g_comm)) {
        fprintf(stderr, "bike_handler_start failed\n");
        bike_handler_destroy(bh);
        return;
    }
    bike_handler_set_auto_reply_rpm(bh, true);
    g_bh = bh;
    printf("[demo] bike_handler attached\n");
}

/* ------------------------------------------------------------------------- */
/* general_handler callbacks                                                 */
/* ------------------------------------------------------------------------- */

/* The 0x73 version word is split into (LCB type, firmware version) */
static void on_gh_version(void *ud, int lcb_type, int fw_version)
{
    (void)ud;
    printf("[gh] LCB version: type 0x%02X, firmware %d\n", lcb_type, fw_version);
}

static void on_gh_lcb_type(void *ud, uint8_t type)
{
    (void)ud;
    printf("[gh] LCB type 0x%02X\n", type);
}

static void on_gh_error_code(void *ud, uint16_t code)
{
    (void)ud;
    printf("[gh] LCB error code 0x%04X\n", code);
}

static void on_gh_lcb_status(void *ud, uint8_t status)
{
    static int last = -1;
    (void)ud;
    if (status != last) {
        printf("[gh] LCB status 0x%02X\n", status);
        last = status;
    }
}

static void on_gh_initial_finish(void *ud)
{
    (void)ud;
    LcbType_t type = general_handler_get_lcb_type(g_gh);   /* read before request_stop */
    printf("[gh] LCB handshake finished, type 0x%02X\n", type);

    if (g_mode == MODE_TM)
        hand_off_to_treadmill(type);
    else if (g_mode == MODE_BIKE)
        hand_off_to_bike(type);
    /* MODE_GENERAL: keep general_handler running (1 s status heartbeat) */

    atomic_store(&g_initial_finished, true);
}

/* ------------------------------------------------------------------------- */

static void on_signal(int sig)
{
    (void)sig;
    g_quit = 1;
}

static void usage(const char *prog)
{
    fprintf(stderr,
            "usage: %s [-d /dev/ttyXX | usb | usb:/dev/bus/usb/BBB/DDD] "
            "[-m general|tm|bike] [-t seconds] [-n] [-v]\n",
            prog);
}

/* ------------------------------------------------------------------------- */
/* Link: tty fd or USB RS485 adapter                                         */
/* ------------------------------------------------------------------------- */

static usb_rs485_t *g_usb;      /* non-NULL when running over USB RS485 */

/* "usb" or "usb:<node>" selects the USB RS485 adapter */
static int open_link(const char *device, int use_rs485)
{
    if (strncmp(device, "usb", 3) != 0 || (device[3] != '\0' && device[3] != ':'))
        return open_lcb_port(device, use_rs485);

    const char *node = device[3] == ':' ? device + 4 : NULL;   /* NULL = auto-detect */
    g_usb = usb_rs485_open(node, 9600);
    if (g_usb == NULL) {
        int err = errno;
        fprintf(stderr, "USB RS485 open failed: %s\n", strerror(err));
        if (err == EACCES || err == EPERM)
            fprintf(stderr, "  no permission on /dev/bus/usb: run as root or add a udev rule\n");
        return -1;
    }
    return usb_rs485_get_fd(g_usb);
}

/* Only after lcb_comm has stopped */
static void close_link(int fd)
{
    if (g_usb != NULL)
        usb_rs485_close(g_usb);   /* also closes the bridge fd */
    else
        close(fd);                /* lcb_comm never closes the fd */
}

int main(int argc, char **argv)
{
    const char *device = "/dev/ttyS4";
    int seconds = 10;
    int use_rs485 = 1;
    int verbose = 0;
    int opt;

    while ((opt = getopt(argc, argv, "d:m:t:nvh")) != -1) {
        switch (opt) {
        case 'd': device = optarg; break;
        case 'm':
            if (strcmp(optarg, "general") == 0)   g_mode = MODE_GENERAL;
            else if (strcmp(optarg, "tm") == 0)   g_mode = MODE_TM;
            else if (strcmp(optarg, "bike") == 0) g_mode = MODE_BIKE;
            else { usage(argv[0]); return 2; }
            break;
        case 't': seconds = atoi(optarg); break;
        case 'n': use_rs485 = 0; break;
        case 'v': verbose = 1; break;
        default:  usage(argv[0]); return 2;
        }
    }

    setvbuf(stdout, NULL, _IOLBF, 0);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* Library log goes to stderr; debug level is very verbose */
    lcb_log_set_debug(verbose != 0);

    int fd = open_link(device, use_rs485);
    if (fd < 0)
        return 1;

    /* lcb_comm keeps a pointer to this struct: it must outlive the link */
    static serial_port_t port;
    memset(&port, 0, sizeof(port));
    port.fd = fd;
    strncpy(port.device_path, g_usb != NULL ? usb_rs485_get_node(g_usb) : device,
            sizeof(port.device_path) - 1);
    port.config.baud_rate = 9600;
    port.config.data_bits = 8;
    port.config.stop_bits = 1;
    port.config.parity    = 'N';

    g_comm = lcb_comm_create();
    g_gh   = general_handler_create();
    if (g_comm == NULL || g_gh == NULL) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    general_handler_callbacks_t cb;
    general_handler_callbacks_init(&cb);
    cb.on_lcb_version        = on_gh_version;
    cb.on_lcb_type           = on_gh_lcb_type;
    cb.on_error_code         = on_gh_error_code;
    cb.on_lcb_status         = on_gh_lcb_status;
    cb.on_lcb_initial_finish = on_gh_initial_finish;
    general_handler_set_callbacks(g_gh, &cb);

    /* Registers gh on g_comm, starts lcb_comm and the handshake */
    if (!general_handler_auto_start(g_gh, g_comm, &port)) {
        fprintf(stderr, "general_handler_auto_start failed\n");
        general_handler_destroy(g_gh);
        lcb_comm_destroy(g_comm);
        close_link(fd);
        return 1;
    }
    printf("[demo] %s opened, waiting for the LCB handshake (%d s)...\n",
           port.device_path, seconds);

    for (int i = 0; i < seconds * 10 && !g_quit; i++) {
        if (g_usb != NULL && !usb_rs485_is_healthy(g_usb)) {
            printf("[demo] USB RS485 adapter error (%s), stopping\n",
                   strerror(-usb_rs485_get_error(g_usb)));
            break;
        }
        usleep(100 * 1000);
    }

    int ok = atomic_load(&g_initial_finished);
    if (!ok)
        printf("[demo] no handshake: check the device path, wiring and LCB power\n");

    /* --- Shutdown -------------------------------------------------------- */
    /* Joins gh's heartbeat thread (so no gh callback is still running) and
     * stops lcb_comm. After a hand-off gh no longer owns g_comm, so this
     * returns without stopping it. */
    general_handler_auto_stop(g_gh);

    if (atomic_load(&g_handed_off)) {
        if (g_ti != NULL) {
            treadmill_interface_stop(g_ti);
            treadmill_interface_destroy(g_ti);
        }
        if (g_bh != NULL) {
            bike_handler_stop(g_bh);
            bike_handler_destroy(g_bh);
        }
        lcb_comm_stop(g_comm);
        /* g_gh is intentionally not destroyed after a hand-off: its heartbeat
         * thread exits on its own and cannot be joined (see README). */
    } else {
        general_handler_destroy(g_gh);
    }

    lcb_comm_destroy(g_comm);
    close_link(fd);

    printf("[demo] done\n");
    return ok ? 0 : 1;
}
