# Build the Linux IVC userspace library, demos, and kernel module.

LIB_NAME 		:= ivc

ARCH 			?= arm64
CROSS 			?= aarch64-linux-musl-
CROSS_COMPILE 	?= aarch64-linux-gnu-
CCOMP 			?= $(CROSS)gcc
CAR 			?= $(CROSS)ar
KDIR 			?= /lib/modules/$(shell uname -r)/build

MAKEFILE_DIR 	:= $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR 		?= $(MAKEFILE_DIR)build
INCLUDE_DIR 	:= $(MAKEFILE_DIR)include
ULIB_SRC_DIR 	:= $(MAKEFILE_DIR)ulib
DEMO_SRC_DIR 	:= $(MAKEFILE_DIR)demo
KERNEL_DIR 		:= $(MAKEFILE_DIR)kernel_driver

ULIB_SRC 		:= $(wildcard $(ULIB_SRC_DIR)/*.c)
ULIB_OBJ 		:= $(patsubst $(ULIB_SRC_DIR)/%.c,$(BUILD_DIR)/ulib/%.o,$(ULIB_SRC))
ULIB_STATIC		:= $(BUILD_DIR)/lib$(LIB_NAME).a
ULIB_SHARED		:= $(BUILD_DIR)/lib$(LIB_NAME).so
DEMO_SRC 		:= $(wildcard $(DEMO_SRC_DIR)/*.c)
DEMO_BIN 		:= $(patsubst $(DEMO_SRC_DIR)/%.c,$(BUILD_DIR)/demo/%, $(DEMO_SRC))

CPPFLAGS 		:= -I$(INCLUDE_DIR)
COMMON_CFLAGS 	:= -std=gnu11 -Wall -Wextra -Werror
ULIB_CFLAGS 	?= $(COMMON_CFLAGS) -Os -fPIC
DEMO_CFLAGS 	?= $(COMMON_CFLAGS) -Os -Wl,--gc-sections -static

.PHONY: all ulib demo kernel_module clean rebuild output_dir

all: ulib demo

ulib: output_dir $(ULIB_STATIC) $(ULIB_SHARED)

demo: output_dir $(DEMO_BIN)

kernel_module:
	$(MAKE) -C $(KERNEL_DIR) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		KDIR=$(KDIR)

$(ULIB_STATIC): $(ULIB_OBJ)
	$(CAR) rcs $@ $^

$(ULIB_SHARED): $(ULIB_OBJ)
	$(CCOMP) -shared -o $@ $^

$(BUILD_DIR)/ulib/%.o: $(ULIB_SRC_DIR)/%.c
	$(CCOMP) $(CPPFLAGS) $(ULIB_CFLAGS) -c $< -o $@

$(BUILD_DIR)/demo/%: $(DEMO_SRC_DIR)/%.c $(ULIB_STATIC)
	$(CCOMP) $(CPPFLAGS) $(DEMO_CFLAGS) -o $@ $< $(ULIB_STATIC)

output_dir:
	@mkdir -p $(BUILD_DIR)/ulib
	@mkdir -p $(BUILD_DIR)/demo

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(KERNEL_DIR)/.tmp_versions
	rm -f $(KERNEL_DIR)/*.ko $(KERNEL_DIR)/*.mod $(KERNEL_DIR)/*.mod.c \
		$(KERNEL_DIR)/*.o $(KERNEL_DIR)/.module-common.o \
		$(KERNEL_DIR)/.*.cmd $(KERNEL_DIR)/..*.cmd \
		$(KERNEL_DIR)/Module.symvers $(KERNEL_DIR)/modules.order

rebuild: clean all
