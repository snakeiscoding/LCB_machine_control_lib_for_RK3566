# Makefile for LCB Protocol Library
# Converted from Java to C implementation
#
# Native build (Linux gcc):
#   make
# Cross build (e.g. aarch64 Linux):
#   make CC=aarch64-linux-gnu-gcc AR=aarch64-linux-gnu-ar

ifeq ($(origin CC),default)
CC = gcc
endif
AR ?= ar

CFLAGS ?= -O2 -g
LCB_CFLAGS = -std=gnu11 -Wall -Wextra -pthread -fPIC

OBJDIR ?= obj
LIBDIR ?= lib
BINDIR ?= bin
PREFIX ?= /usr/local

# 5-layer module layout: inc/<module>/*.h and src/<module>/*.c
MODULES = protocol/Comm protocol/MachineHandler adapters capabilities control hardware
SRCDIRS = $(addprefix src/,$(MODULES))
INCDIRS = $(addprefix inc/,$(MODULES)) third_party/cJSON
INCLUDES = $(addprefix -I,$(INCDIRS))

# Source files (every module directory under src/ plus bundled cJSON)
SOURCES = $(wildcard $(addsuffix /*.c,$(SRCDIRS))) third_party/cJSON/cJSON.c
OBJECTS = $(patsubst %.c,$(OBJDIR)/%.o,$(SOURCES))
DEPS = $(OBJECTS:.o=.d)

# Public headers for install (internal headers stay private)
HEADERS = $(filter-out %_internal.h,$(wildcard $(addsuffix /*.h,$(INCDIRS))))

LIBS = -pthread -lm

# Library name
LIB_NAME = liblcb_protocol
STATIC_LIB = $(LIBDIR)/$(LIB_NAME).a
SHARED_LIB = $(LIBDIR)/$(LIB_NAME).so

# Test / example programs
TEST_BINS = $(BINDIR)/test_lcb $(BINDIR)/test_constants
EXAMPLE_BINS = $(BINDIR)/simple_usage

.PHONY: all clean static shared test example install help

# Default target
all: static shared

# Static library
static: $(STATIC_LIB)

$(STATIC_LIB): $(OBJECTS)
	@mkdir -p $(dir $@)
	@echo "Creating static library: $@"
	$(AR) rcs $@ $^

# Shared library
shared: $(SHARED_LIB)

$(SHARED_LIB): $(OBJECTS)
	@mkdir -p $(dir $@)
	@echo "Creating shared library: $@"
	$(CC) -shared $(LDFLAGS) -o $@ $^ $(LIBS) $(LDLIBS)

# Compile source files (object tree mirrors the source tree)
$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling: $<"
	$(CC) $(LCB_CFLAGS) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Test programs
test: $(TEST_BINS)

$(BINDIR)/test_%: test/test_%.c $(STATIC_LIB)
	@mkdir -p $(dir $@)
	@echo "Building test program: $@"
	$(CC) $(LCB_CFLAGS) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS) $(STATIC_LIB) $(LIBS) $(LDLIBS)

# Example programs
example: $(EXAMPLE_BINS)

$(BINDIR)/%: examples/%.c $(STATIC_LIB)
	@mkdir -p $(dir $@)
	@echo "Building example program: $@"
	$(CC) $(LCB_CFLAGS) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS) $(STATIC_LIB) $(LIBS) $(LDLIBS)

# Clean
clean:
	@echo "Cleaning..."
	rm -rf $(OBJDIR) $(LIBDIR) $(BINDIR)
	rm -f test/test_lcb

# Install (optional): libraries to $(PREFIX)/lib, public headers to $(PREFIX)/include/lcb_protocol
install: all
	@echo "Installing to $(DESTDIR)$(PREFIX)..."
	install -d $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include/lcb_protocol
	install -m 644 $(STATIC_LIB) $(SHARED_LIB) $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(HEADERS) $(DESTDIR)$(PREFIX)/include/lcb_protocol/

# Help
help:
	@echo "LCB Protocol Library Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build static and shared library (default)"
	@echo "  static  - Build static library  ($(STATIC_LIB))"
	@echo "  shared  - Build shared library  ($(SHARED_LIB))"
	@echo "  test    - Build test programs   ($(TEST_BINS))"
	@echo "  example - Build example program ($(EXAMPLE_BINS))"
	@echo "  clean   - Remove build artifacts"
	@echo "  install - Install library and public headers to $(PREFIX)"
	@echo "  help    - Show this help message"
	@echo ""
	@echo "Variables: CC AR CFLAGS CPPFLAGS LDFLAGS LDLIBS OBJDIR LIBDIR BINDIR PREFIX DESTDIR"

-include $(DEPS)
