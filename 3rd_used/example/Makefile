# Builds lcb_demo against this kit (../include, ../lib).
#
#   make                                    # aarch64-buildroot-linux-gnu-gcc from PATH
#   make CROSS_COMPILE=/opt/rk3566-dev/sdk/bin/aarch64-buildroot-linux-gnu-
#
# The binary is linked with rpath $ORIGIN: copy lcb_demo and
# liblcb_protocol.so into the same directory on the board.

CROSS_COMPILE ?= aarch64-buildroot-linux-gnu-
CC  = $(CROSS_COMPILE)gcc
KIT ?= ..

# Headers are grouped by layer and include each other by file name,
# so every layer directory goes on the include path.
LCB_INC    = $(KIT)/include/lcb_protocol
LCB_LAYERS = capabilities adapters control protocol/Comm protocol/MachineHandler \
             hardware third_party/cJSON

CFLAGS  ?= -O2 -g
CFLAGS  += -std=gnu11 -Wall -Wextra $(addprefix -I$(LCB_INC)/,$(LCB_LAYERS))
LDFLAGS += -L$(KIT)/lib -Wl,-rpath,'$$ORIGIN'
LDLIBS  += -llcb_protocol -pthread -lm

.PHONY: all clean

all: lcb_demo

lcb_demo: lcb_demo.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f lcb_demo
