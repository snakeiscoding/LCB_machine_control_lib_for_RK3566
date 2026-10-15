# LCB Protocol C Implementation

This directory contains the C implementation of the LCB (Lower Control Board) communication protocol and the machine-control layers built on top of it. It was converted from the Java implementation in `app/src/main/java/com/jht/lcbcommlib/` (plus chimera2 for the control layer).

The LCB protocol is used between the Upper Control Board (UCB) and the Lower Control Board (LCB) in fitness equipment: treadmills, bikes, ellipticals, steppers, climbmills, and so on.

## Architecture

The code is organised in a 5-layer layout. Each layer has a matching `inc/<layer>/` header directory and `src/<layer>/` source directory.

| Layer | Directory | Role |
|-------|-----------|------|
| Layer_1 | `capabilities/` | Per-model capability and calibration data: frame config, JSON loader, registry, unit conversion |
| Layer_2 | `adapters/` | Safety / business logic on top of the machine handlers (Java `<Machine>Interface.java`) |
| Layer_3 | `control/` | Machine control, such as workout and calibration flow (chimera2 `WorkoutControllerService`) |
| Layer_3 | `protocol/MachineHandler/` | Per-machine handlers: heartbeat, polling and state machines (Java `<Machine>Handler.java`) |
| Layer_4 | `protocol/Comm/` | LCB packet framing, CRC8, TX priority queue, RX parsing, response dispatch |
| Layer_5 | `hardware/` | USB-to-RS485 adapters (CH340/CH341) over usbdevfs, exposed to `lcb_comm` as a plain fd |

Headers are included by file name only (`#include "lcb_comm.h"`). Every module directory must therefore be on the include path; see [Building](#building).

Installed and packaged headers keep the same layout: `make install`, `cmake --install` and the third-party kit (`../3rd_used/include/lcb_protocol/<layer>/`) all use one directory per layer. See [BUILD_GUIDE.md](BUILD_GUIDE.md#installed-header-layout).

## Directory Structure

```
lcb_protocol/
├── inc/                                  # Public (*.h) and internal (*_internal.h) headers
│   ├── protocol/
│   │   ├── Comm/                         # lcb_comm, lcb_packet, lcb_parser, lcb_constants, ...
│   │   └── MachineHandler/               # <machine>_handler.h + <machine>_handler_internal.h
│   ├── adapters/                         # treadmill_interface, treadmill_machine_adapter, tcv_fx_spin_bike_interface
│   ├── capabilities/                     # frame_config*, machine_config, calibration_table, value_range, ...
│   ├── control/                          # treadmill_control
│   └── hardware/                         # usb_rs485, ch340, usb_serial_bridge, usb_host (+ README.md)
├── src/                                  # Same layout as inc/
│   ├── protocol/Comm/                    # 16 .c files
│   ├── protocol/MachineHandler/          # <machine>_handler.c + <machine>_lcb_cb.c for 11 handlers
│   ├── adapters/                         # 3 .c files
│   ├── capabilities/                     # 11 .c files
│   ├── control/                          # 1 .c file
│   └── hardware/                         # 4 .c files + private hw_logging.h
├── third_party/cJSON/                    # Bundled cJSON (used by frame_config_json_loader.c)
├── test/                                 # Stand-alone C test programs
├── examples/                             # simple_usage.c
├── Makefile                              # GNU Make build
└── CMakeLists.txt                        # CMake build
```

## Modules

### protocol/Comm — LCB protocol core

| File | Description |
|------|-------------|
| `lcb_comm.h/.c` | Main communication object: TX/RX threads, retry, ERP mode, command API (`lcb_comm_*`) |
| `lcb_comm_callback.h` | `lcb_comm_callbacks_t` callback table for every response |
| `lcb_proc_comm.h/.c` | Per-command response processing, dispatching to the callbacks |
| `lcb_commands.h`, `lcb_constants.h/.c` | Command IDs, status bits, protocol constants |
| `lcb_packet.h/.c`, `lcb_parser.h/.c`, `lcb_crc8.h/.c` | Packet structure, RX state-machine parser, CRC8 |
| `lcb_packet_queue.h/.c` | Thread-safe priority TX queue |
| `lcb_receive_data_buffer.h/.c`, `lcb_receive_data_queue.h/.c` | RX byte buffering |
| `lcb_device_data.h`, `lcb_device_data_proc.c`, `lcb_device_data_tx.c` | Device Data sub-commands (command 0x50) |
| `lcb_universal_data.h`, `lcb_universal_data_proc.c`, `lcb_universal_data_tx.c` | Universal Data sub-commands (command 0xAA) |
| `lcb_type.h/.c` | LCB board type definitions |
| `lcb_utils.h/.c` | Byte/array helpers |
| `lcb_logging.h/.c` | `LCB_LOGI/D/E` logging with runtime on/off switches |

### protocol/MachineHandler — machine handlers

Each handler is made of `<machine>_handler.h`, `<machine>_handler_internal.h`, `<machine>_handler.c` and `<machine>_lcb_cb.c`. The `_lcb_cb.c` file bridges `lcb_comm_callbacks_t` to the handler's own callbacks.

| Handler | Machine |
|---------|---------|
| `general_handler` | Common LCB initialisation, heartbeat and status (used together with a machine handler) |
| `treadmill_handler` | Treadmill (TM) |
| `hybrid_tm_handler` | Hybrid treadmill |
| `bike_handler` | Bike |
| `indoor_bike_handler` | Indoor bike |
| `tcv_fx_spin_bike_handler` | TCV FX spin bike (manual resistance) |
| `elliptical_handler` | Elliptical |
| `ascent_handler` | Ascent trainer |
| `stepper_handler` | Stepper |
| `en_climb_handler` | EnClimbmill |
| `jht_ad_board_handler` | JHT A/D board |

### adapters, capabilities, control

| Layer | Files |
|-------|-------|
| adapters | `treadmill_interface` (ext-status, safety key, error code), `treadmill_machine_adapter` (machine-control commands), `tcv_fx_spin_bike_interface` (unit conversion) |
| capabilities | `frame_config_spec` / `frame_config_json_loader` (parse `frame_configs.json` with cJSON), `frame_config_registry` (lookup by model / serial / LCB type), `frame_config_data*` (resolved per-model data), `machine_config`, `machine_category`, `value_range`, `calibration_table`, `treadmill_machine_convert`, `frame_config` (older Bike-only schema) |
| control | `treadmill_control` (Play/Pause/Resume/Stop/EStop, calibration; call `treadmill_control_tick()` periodically) |

### hardware — USB RS485 (Layer_5)

Lets the LCB link run over a USB-to-RS485 adapter (CH340/CH341, `1a86:7523` / `1a86:5523`) instead of a UART. Ported from the ProjectOne Android app's Layer_5 driver. On Linux the usbdevfs node `/dev/bus/usb/BBB/DDD` is found through sysfs instead of Android's `UsbManager`.

| File | Description |
|------|-------------|
| `usb_rs485.h/.c` | One-call API: `usb_rs485_open(node or NULL, baud)` finds and opens the adapter and returns an fd for `serial_port_t.fd`; `usb_rs485_close()` after `lcb_comm` has stopped |
| `ch340.h/.c`, `ch340_internal.h` | CH340/CH341 driver: init sequence, baud divisor, LCR, DTR/RTS, modem status, break |
| `usb_serial_bridge.h/.c` | socketpair + RX/TX pump threads; the upper end is a non-blocking stream fd, so `lcb_comm` (Layer_4) is unchanged and `lcb_comm_stop()` returns promptly |
| `usb_host.h/.c` | usbdevfs ioctl transport (control/bulk/claim, kernel-driver detach) and descriptor parsing |
| `hw_logging.h` (in `src/hardware/`) | Private stderr logging; independent of `lcb_logging.h` to avoid an upward dependency |

Opening the node needs read/write access to `/dev/bus/usb` (root or a udev rule). See `../3rd_used/README.md` section 7.6 for usage.

## Building

**RK3566 board `.so`**: run `make` in the project root (`JHT_Ccode_LCB_machine_control_lib/`) inside the `rk3566_frankhung` container. It cross-compiles with the Buildroot SDK and writes `out/aarch64/lib/liblcb_protocol.so`. See [../BUILD_LINUX.md](../BUILD_LINUX.md) (Traditional Chinese) for the full procedure.

Building this directory directly:

```bash
cd lcb_protocol

# Native (Linux gcc): static + shared library
make
make test && ./bin/test_lcb && ./bin/test_constants

# Cross build, e.g. aarch64 Linux
make CC=aarch64-linux-gnu-gcc AR=aarch64-linux-gnu-ar

# CMake
cmake -S . -B build && cmake --build build && (cd build && ctest)
```

Both build systems compile every `.c` file under the six module directories plus `third_party/cJSON/cJSON.c`. A new file placed in a module directory is picked up automatically.

When the library is built by another build system, add these include directories:

```
inc/protocol/Comm  inc/protocol/MachineHandler  inc/adapters
inc/capabilities   inc/control                  inc/hardware
third_party/cJSON
```

Link with `-pthread -lm`.

See [BUILD_GUIDE.md](BUILD_GUIDE.md) for all targets, variables, cross-compiling and integration details.

### Logging

`lcb_logging.h` writes to `stderr`, formatted as `[I|D|E] <tag>: <message>`. Each level can be switched at runtime with `lcb_log_set_info()`, `lcb_log_set_debug()` and `lcb_log_set_error()`.

## Usage Example

```c
#include "lcb_comm.h"
#include "lcb_comm_callback.h"
#include "lcb_constants.h"

static void on_status(void *user_data, uint8_t lcb_status)
{
    if (lcb_status & LCB_STATUS_BIT_LCB_ERROR) {
        /* ... */
    }
}

/* Callbacks */
lcb_comm_callbacks_t callbacks;
lcb_comm_callbacks_init(&callbacks);
callbacks.user_data = my_context;
callbacks.on_lcb_status = on_status;

/* Create and configure */
lcb_comm_t *comm = lcb_comm_create();
lcb_comm_register_callbacks(comm, &callbacks);

/* Start: the caller opens and configures the serial port (open() + termios).
 * lcb_comm keeps the pointer, so `port` must outlive the communication. */
static serial_port_t port;
port.fd = fd;
lcb_comm_start(comm, &port);

/* Send commands */
lcb_comm_set_lcb_initial(comm);
lcb_comm_get_lcb_status(comm, LCB_QUEUE_MODE_ALWAYS_ADD);
lcb_comm_set_lcb_motor_rpm(comm, 1000);

/* Stop and cleanup */
lcb_comm_stop(comm);
lcb_comm_destroy(comm);
```

Machine-level code normally does not drive `lcb_comm` directly. It starts `general_handler` plus a machine handler on top of the same `lcb_comm_t`, for example `general_handler_auto_start()` and `treadmill_handler_start()`. Use a single `lcb_comm_t` and serial port per device: stop the running handlers before handing the port to another machine handler.

`examples/simple_usage.c` is a complete, buildable version of the example above (`make example`).

## Protocol Overview

### Packet Format

```
+------------+--------+-----------+--------+------+-------+
| Start Byte | Status | Command   | Length | Data | CRC8  |
|   (0x00)   | (0xFF) | (1 byte)  |(1 byte)| (N)  |(1 byte)|
+------------+--------+-----------+--------+------+-------+
      TX: 0x00 (UCB to LCB)
      RX: 0x01 (LCB to UCB)
```

- **CRC8**: polynomial-based error detection
- **Priority queue**: commands are prioritised (1 = highest, 5 = lowest)
- **Byte order**: multi-byte values are big-endian

### Command Categories

| Range | Category |
|-------|----------|
| 0x40–0x4F | Calibration (SPM, PWM, calibration scaling) |
| 0x50–0x5F | Device Data (indoor bike sensor data, spin down, ERG mode) |
| 0x60–0x6F | ECB (Electronic Control Board) |
| 0x70–0x7F | System (initialise, status, error code, version, calibrate) |
| 0x80–0x8F | Bike / climbmill (brake, watts, resistance) |
| 0x90–0x9F | Motor (specifications, power control) |
| 0xA0–0xAF | DCI (incline/speed ranges, Universal Data 0xAA) |
| 0xF0–0xFF | Treadmill (motor, speed, incline, operating status) |

## Threading Model

`lcb_comm` runs its own pthreads, following the Java implementation:

1. **Communication thread**: TX queue processing, retry and timeout handling
2. **RX thread**: reads from the serial port and feeds the parser
3. **ERP stop thread**: ERP (energy-saving mode) handling

The machine handlers add their own heartbeat/polling threads. Callbacks are invoked on those threads. For example, `general_handler`'s `on_lcb_initial_finish` runs on its heartbeat thread, so it must not stop the handler synchronously. Use `general_handler_request_stop()` there.

## Tests and Examples

| File | Built by | Description |
|------|----------|-------------|
| `test/test_lcb.c` | `make test`, CMake `BUILD_TESTS` | CRC8, packet build/parse, queue, utils |
| `test/test_constants.c` | `make test`, CMake `BUILD_TESTS` | Constants, status bits, command categories |
| `examples/simple_usage.c` | `make example`, CMake `BUILD_EXAMPLES` | API walkthrough without hardware |

The gtest sources in `../test/*.cpp` are kept, but no build file currently compiles them.

The hardware layer has no unit tests in this repository; the Android app's hardware gtests were not ported. `../3rd_used/example/lcb_demo -d usb` exercises it on a board with a CH340 adapter attached.

## Related Documents

- [../BUILD_LINUX.md](../BUILD_LINUX.md): building the RK3566 `.so` in the Linux build container (Traditional Chinese)
- [BUILD_GUIDE.md](BUILD_GUIDE.md): build systems, cross-compiling, integration

---

**Originally converted**: 2026-02-03 (Java → C11, package `com.jht.lcbcommlib`)
**Last updated**: 2026-10-13 (5-layer layout, build system update, Android build removed)
