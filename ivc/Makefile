# Build the Linux IVC userspace library, demos, host regressions, and kernel module.

LIB_NAME := ivc

ARCH ?= arm64
CROSS ?= aarch64-linux-musl-
CROSS_COMPILE ?= aarch64-linux-gnu-
HOST_CC ?= cc
CCOMP := $(CROSS)gcc
CAR := $(CROSS)ar
KDIR ?= /lib/modules/$(shell uname -r)/build

MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR ?= $(MAKEFILE_DIR)build
INCLUDE_DIR := $(MAKEFILE_DIR)include
ULIB_SRC_DIR := $(MAKEFILE_DIR)ulib
DEMO_SRC_DIR := $(MAKEFILE_DIR)demo
TEST_SRC_DIR := $(MAKEFILE_DIR)tests
KERNEL_DIR := $(MAKEFILE_DIR)kernel_driver

ULIB_SRC := $(wildcard $(ULIB_SRC_DIR)/*.c)
ULIB_OBJ := $(patsubst $(ULIB_SRC_DIR)/%.c,$(BUILD_DIR)/ulib/%.o,$(ULIB_SRC))
ULIB_STATIC := $(BUILD_DIR)/lib$(LIB_NAME).a
ULIB_SHARED := $(BUILD_DIR)/lib$(LIB_NAME).so
DEMO_SRC := $(wildcard $(DEMO_SRC_DIR)/*.c)
DEMO_BIN := $(patsubst $(DEMO_SRC_DIR)/%.c,$(BUILD_DIR)/demo/%,$(DEMO_SRC))

APP_PROTO_TEST := $(BUILD_DIR)/tests/ivc-app-proto
WRITE_TEST := $(BUILD_DIR)/tests/ivc-write-all-zero-progress
OPEN_ROLLBACK_TEST := $(BUILD_DIR)/tests/ivc-open-failure-rolls-back
TEARDOWN_TEST := $(BUILD_DIR)/tests/ivc-teardown-best-effort
MANAGER_TEST := $(BUILD_DIR)/tests/ivc-manager-lifecycle
TEST_BIN := $(APP_PROTO_TEST) $(WRITE_TEST) $(OPEN_ROLLBACK_TEST) $(TEARDOWN_TEST) $(MANAGER_TEST)

CPPFLAGS := -I$(INCLUDE_DIR)
COMMON_CFLAGS := -std=gnu11 -Wall -Wextra -Werror
ULIB_CFLAGS ?= $(COMMON_CFLAGS) -Os -fPIC
DEMO_CFLAGS ?= $(COMMON_CFLAGS) -Os -Wl,--gc-sections -static
HOST_CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O2

.PHONY: all ulib demo test kernel-module clean rebuild output-dirs

all: ulib demo

ulib: $(ULIB_STATIC) $(ULIB_SHARED)

demo: $(DEMO_BIN)

test: $(TEST_BIN)
	@set -e; for test_bin in $(TEST_BIN); do "$$test_bin"; done

kernel-module:
	$(MAKE) -C $(KERNEL_DIR) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		KDIR=$(KDIR)

$(ULIB_STATIC): $(ULIB_OBJ)
	$(CAR) rcs $@ $^

$(ULIB_SHARED): $(ULIB_OBJ)
	$(CCOMP) -shared -o $@ $^

$(BUILD_DIR)/ulib/%.o: $(ULIB_SRC_DIR)/%.c | output-dirs
	$(CCOMP) $(CPPFLAGS) $(ULIB_CFLAGS) -c $< -o $@

$(BUILD_DIR)/demo/%: $(DEMO_SRC_DIR)/%.c $(ULIB_STATIC) | output-dirs
	$(CCOMP) $(CPPFLAGS) $(DEMO_CFLAGS) -o $@ $< $(ULIB_STATIC)

$(APP_PROTO_TEST): $(TEST_SRC_DIR)/test_app_proto.c $(ULIB_SRC_DIR)/app_proto.c | output-dirs
	$(HOST_CC) $(CPPFLAGS) $(HOST_CFLAGS) -o $@ $^

$(WRITE_TEST): $(TEST_SRC_DIR)/ivc_write_all_zero_progress.c $(ULIB_SRC_DIR)/ivc_ulib.c | output-dirs
	$(HOST_CC) $(CPPFLAGS) $(HOST_CFLAGS) -Wl,--wrap=write -o $@ $^

$(OPEN_ROLLBACK_TEST): $(TEST_SRC_DIR)/ivc_open_failure_rolls_back.c $(ULIB_SRC_DIR)/ivc_ulib.c | output-dirs
	$(HOST_CC) $(CPPFLAGS) $(HOST_CFLAGS) -Wl,--wrap=open -Wl,--wrap=ioctl -o $@ $^

$(TEARDOWN_TEST): $(TEST_SRC_DIR)/ivc_teardown_best_effort.c $(ULIB_SRC_DIR)/ivc_ulib.c | output-dirs
	$(HOST_CC) $(CPPFLAGS) $(HOST_CFLAGS) \
		-Wl,--wrap=close -Wl,--wrap=ioctl -Wl,--wrap=free -o $@ $^

$(MANAGER_TEST): $(TEST_SRC_DIR)/ivc_manager_lifecycle.c $(ULIB_SRC_DIR)/ivc_ulib.c | output-dirs
	$(HOST_CC) $(CPPFLAGS) $(HOST_CFLAGS) \
		-Wl,--wrap=open -Wl,--wrap=close -Wl,--wrap=ioctl -Wl,--wrap=free -o $@ $^

output-dirs:
	@mkdir -p $(BUILD_DIR)/ulib $(BUILD_DIR)/demo $(BUILD_DIR)/tests

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(KERNEL_DIR)/.tmp_versions
	rm -f $(KERNEL_DIR)/*.ko $(KERNEL_DIR)/*.mod $(KERNEL_DIR)/*.mod.c \
		$(KERNEL_DIR)/*.o $(KERNEL_DIR)/.module-common.o \
		$(KERNEL_DIR)/.*.cmd $(KERNEL_DIR)/..*.cmd \
		$(KERNEL_DIR)/Module.symvers $(KERNEL_DIR)/modules.order

rebuild: clean all
