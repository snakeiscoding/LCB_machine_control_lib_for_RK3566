# LCB Protocol Library - Build Guide

This guide covers building `lcb_protocol` with its own Makefile or CMakeLists.txt, cross-compiling, and integrating the sources into another build. See [README.md](README.md) for the architecture and API overview.

> To build the RK3566 board `.so`, use the top-level Makefile in the `rk3566_frankhung` container, as described in [../BUILD_LINUX.md](../BUILD_LINUX.md). It calls this directory's Makefile with the Buildroot SDK toolchain.

## Prerequisites

| Tool | Version | Notes |
|------|---------|-------|
| C compiler | GCC 7+ or Clang 6+ | Built as `-std=gnu11`. The code uses GNU extensions (`##__VA_ARGS__`, `__attribute__`) |
| GNU Make | 3.82+ | For the Makefile |
| CMake | 3.12+ | For CMakeLists.txt (`file(GLOB ... CONFIGURE_DEPENDS)`) |
| pthreads | — | POSIX threads |

## What Gets Built

Both build systems compile the same set of sources:

- every `*.c` in `src/protocol/Comm`, `src/protocol/MachineHandler`, `src/adapters`, `src/capabilities`, `src/control` and `src/hardware`
- `third_party/cJSON/cJSON.c`, which `frame_config_json_loader.c` needs

The include path is the matching six `inc/...` directories plus `third_party/cJSON`. Link with `-pthread -lm`.

## Method 1: Makefile

```bash
cd lcb_protocol

make            # static + shared library (default target "all")
make test       # build test programs
make example    # build examples/simple_usage
make clean      # remove obj/, lib/, bin/
make help       # list targets and variables
```

### Targets

| Target | Output |
|--------|--------|
| `all` (default) | `lib/liblcb_protocol.a` and `lib/liblcb_protocol.so` |
| `static` | `lib/liblcb_protocol.a` |
| `shared` | `lib/liblcb_protocol.so` |
| `test` | `bin/test_lcb`, `bin/test_constants` |
| `example` | `bin/simple_usage` |
| `install` | Libraries to `$(PREFIX)/lib`, public headers (no `*_internal.h`) to `$(PREFIX)/include/lcb_protocol` |
| `clean` | Removes `obj/`, `lib/`, `bin/` |

Object files mirror the source tree (`obj/src/protocol/Comm/lcb_comm.o`, ...). Dependency files (`*.d`) are generated, so editing a header rebuilds only the sources that include it.

### Variables

All variables can be overridden on the command line, e.g. `make CFLAGS="-O0 -g"`.

| Variable | Default | Purpose |
|----------|---------|---------|
| `CC` | `gcc` | Compiler |
| `AR` | `ar` | Archiver (use the cross toolchain's `ar` when cross-compiling) |
| `CFLAGS` | `-O2 -g` | Optimisation/debug flags. `-std=gnu11 -Wall -Wextra -pthread -fPIC` are always added |
| `CPPFLAGS`, `LDFLAGS`, `LDLIBS` | empty | Extra preprocessor / linker flags / libraries |
| `OBJDIR`, `LIBDIR`, `BINDIR` | `obj`, `lib`, `bin` | Output directories |
| `PREFIX`, `DESTDIR` | `/usr/local`, empty | Install location |

### Native Build (Linux)

```bash
sudo apt-get install build-essential     # Ubuntu/Debian
make
make test
./bin/test_lcb
./bin/test_constants
sudo make install                        # optional
```

Useful flag sets:

```bash
make CFLAGS="-O0 -g -fsanitize=address" LDFLAGS="-fsanitize=address"   # debugging
make CFLAGS="-Os"                                                      # size
```

### Cross Build

Pass the cross compiler and archiver. Use separate output directories per target, or run `make clean` between targets:

```bash
# RK3566 Buildroot SDK (inside the rk3566_frankhung container)
SDK=/opt/rk3566-dev/sdk
make CC=$SDK/bin/aarch64-buildroot-linux-gnu-gcc AR=$SDK/bin/aarch64-buildroot-linux-gnu-ar \
     OBJDIR=obj-aarch64 LIBDIR=lib-aarch64 BINDIR=bin-aarch64

# Generic distribution cross compilers
sudo apt-get install gcc-aarch64-linux-gnu                              # Ubuntu/Debian
make CC=aarch64-linux-gnu-gcc AR=aarch64-linux-gnu-ar \
     OBJDIR=obj-aarch64 LIBDIR=lib-aarch64 BINDIR=bin-aarch64           # aarch64 Linux
make CC=arm-linux-gnueabihf-gcc AR=arm-linux-gnueabihf-ar \
     OBJDIR=obj-armhf LIBDIR=lib-armhf BINDIR=bin-armhf                 # 32-bit ARM Linux
```

The target's glibc must provide every `GLIBC_x.y` version the `.so` references. Check with `objdump -T liblcb_protocol.so | grep -o 'GLIBC_[0-9.]*' | sort -uV`. Building with the board's own Buildroot SDK guarantees a match.

## Method 2: CMake

```bash
cd lcb_protocol
cmake -S . -B build
cmake --build build
(cd build && ctest)
sudo cmake --install build          # optional
```

### Options

| Option | Default | Effect |
|--------|---------|--------|
| `BUILD_SHARED_LIBS` | `OFF` | Also build `liblcb_protocol.so` (target `lcb_protocol_shared`) |
| `BUILD_TESTS` | `ON` | Build `test_lcb`, `test_constants` and register them with CTest |
| `BUILD_EXAMPLES` | `OFF` | Build `simple_usage` |
| `CMAKE_BUILD_TYPE` | — | e.g. `Release`, `Debug` |
| `CMAKE_INSTALL_PREFIX` | system default | Install location |

The static library target `lcb_protocol_static` (output name `lcb_protocol`) is always built. Its include directories, `Threads::Threads` and `m` are `PUBLIC`, so anything linking it inherits them.

### CMake Cross Build

```bash
cmake -S . -B build-aarch64 \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
    -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc
cmake --build build-aarch64
```

### Using as a Subdirectory

```cmake
add_subdirectory(lcb_protocol)
target_link_libraries(myapp PRIVATE lcb_protocol_static)   # include dirs + libs come along
```

## Method 3: Compiling the Sources into Another Build

If the sources are compiled directly into another target, add every module directory and cJSON:

```bash
LCB=./lcb_protocol
gcc -std=gnu11 -o myapp myapp.c \
    -I$LCB/inc/protocol/Comm -I$LCB/inc/protocol/MachineHandler -I$LCB/inc/adapters \
    -I$LCB/inc/capabilities -I$LCB/inc/control -I$LCB/inc/hardware -I$LCB/third_party/cJSON \
    $LCB/lib/liblcb_protocol.a -pthread -lm
```

To link the shared library instead, replace the `.a` with `-L$LCB/lib -llcb_protocol`. At run time, set `LD_LIBRARY_PATH=$LCB/lib`.

## Adding Code

- **New file in an existing layer** (for example a new machine handler in `protocol/MachineHandler`): put the `.c` in `src/<layer>/` and the `.h` in `inc/<layer>/`. The Makefile and CMake both pick it up automatically. CMake re-globs at build time thanks to `CONFIGURE_DEPENDS`.
- **New layer directory**: add it to `MODULES` in the Makefile and to `LCB_MODULES` in CMakeLists.txt, and to the include list of any external build.
- Include headers by file name only (`#include "treadmill_handler.h"`). Keep private structs in `*_internal.h`; these are not installed.

## Troubleshooting

| Symptom | Cause / Fix |
|---------|-------------|
| `xxx.h: No such file or directory` | A module include directory is missing; add all seven directories listed above |
| `undefined reference to cJSON_*` | `third_party/cJSON/cJSON.c` is missing from the sources |
| `undefined reference to pthread_create` | Add `-pthread` |
| `ar: ... file format not recognized` when cross-compiling | Pass the cross toolchain's archiver, e.g. `AR=aarch64-linux-gnu-ar` |
| `error while loading shared libraries: liblcb_protocol.so` | `export LD_LIBRARY_PATH=/path/to/lib:$LD_LIBRARY_PATH` |
| Permission denied on the serial port | `sudo usermod -a -G dialout $USER`, then log in again |

## Development Workflow

```bash
make clean && make && make test && ./bin/test_lcb && ./bin/test_constants

# Static analysis
cppcheck --enable=all -Iinc/protocol/Comm -Iinc/protocol/MachineHandler -Iinc/adapters \
         -Iinc/capabilities -Iinc/control src/

# Memory check
valgrind --leak-check=full ./bin/test_lcb

# Formatting
clang-format -i $(find src inc -name '*.[ch]')
```

---

**Last Updated**: 2026-10-13
**Version**: 1.0.0
