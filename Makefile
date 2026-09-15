# Top-level orchestration Makefile.
#
# NOTE: this file is for human-facing targets only. The kernel module
# build rules live in Kbuild, which the kernel build system reads in
# preference to this file when invoked via:
#
#   make -C <kernel build tree> M=<this directory> modules

ARCH 			?= arm64
CROSS 			?= aarch64-linux-musl-
CROSS_COMPILE 	?= aarch64-linux-gnu-
CCOMP 			?= $(CROSS)gcc
CAR 			?= $(CROSS)ar
KDIR 			?= /lib/modules/$(shell uname -r)/build

MAKEFILE_DIR 	:= $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR 		?= $(MAKEFILE_DIR)build
UAPI_DIR 		:= $(MAKEFILE_DIR)ivc/uapi
LIB_SRC_DIR 	:= $(MAKEFILE_DIR)userspace/ivc
DEMO_SRC_DIR 	:= $(MAKEFILE_DIR)demo

LIB_SRC 		:= $(wildcard $(LIB_SRC_DIR)/*.c)
LIB_OBJ 		:= $(patsubst $(LIB_SRC_DIR)/%.c,$(BUILD_DIR)/libivc/%.o,$(LIB_SRC))
IVC_LIB_STATIC	:= $(BUILD_DIR)/libivc.a
IVC_LIB_SHARED	:= $(BUILD_DIR)/libivc.so
LIB_HEADERS 	:= $(wildcard $(UAPI_DIR)/*.h $(LIB_SRC_DIR)/*.h)
DEMO_COMMON 	:= $(DEMO_SRC_DIR)/app_proto.c
DEMO_SRC 		:= $(filter-out $(DEMO_COMMON),$(wildcard $(DEMO_SRC_DIR)/*.c))
DEMO_BIN 		:= $(patsubst $(DEMO_SRC_DIR)/%.c,$(BUILD_DIR)/demo/%, $(DEMO_SRC))

CPPFLAGS 		:= -I$(UAPI_DIR) -I$(LIB_SRC_DIR)
COMMON_CFLAGS 	:= -std=gnu11 -Wall -Wextra -Werror
LIB_CFLAGS 		?= $(COMMON_CFLAGS) -Os -fPIC
DEMO_CFLAGS 	?= $(COMMON_CFLAGS) -Os -Wl,--gc-sections -static

.PHONY: all libivc demo kernel_module clean rebuild output_dir

all: libivc demo

libivc: output_dir $(IVC_LIB_STATIC) $(IVC_LIB_SHARED)

demo: output_dir $(DEMO_BIN)

# Builds axvisor.ko in this directory (see Kbuild).
kernel_module:
	$(MAKE) -C $(KDIR) M=$(MAKEFILE_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

# Recreate the archive so removed sources (such as demo/app_proto.c) do
# not leave stale members in an existing libivc.a.
$(IVC_LIB_STATIC): $(LIB_OBJ) $(MAKEFILE_DIR)Makefile
	rm -f $@
	$(CAR) rcs $@ $(LIB_OBJ)

$(IVC_LIB_SHARED): $(LIB_OBJ) $(MAKEFILE_DIR)Makefile
	$(CCOMP) -shared -o $@ $(LIB_OBJ)

$(BUILD_DIR)/libivc/%.o: $(LIB_SRC_DIR)/%.c $(LIB_HEADERS) $(MAKEFILE_DIR)Makefile | output_dir
	$(CCOMP) $(CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

$(BUILD_DIR)/demo/%: $(DEMO_SRC_DIR)/%.c $(DEMO_COMMON) $(DEMO_SRC_DIR)/app_proto.h $(IVC_LIB_STATIC) $(LIB_HEADERS) | output_dir
	$(CCOMP) $(CPPFLAGS) $(DEMO_CFLAGS) -o $@ $< $(DEMO_COMMON) $(IVC_LIB_STATIC)

output_dir:
	@mkdir -p $(BUILD_DIR)/libivc
	@mkdir -p $(BUILD_DIR)/demo

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(MAKEFILE_DIR).tmp_versions
	rm -f $(MAKEFILE_DIR)*.ko $(MAKEFILE_DIR)*.mod $(MAKEFILE_DIR)*.mod.c \
		$(MAKEFILE_DIR)*.o $(MAKEFILE_DIR).*.cmd $(MAKEFILE_DIR).module-common.o \
		$(MAKEFILE_DIR)Module.symvers $(MAKEFILE_DIR)modules.order \
		$(MAKEFILE_DIR)ivc/*.o $(MAKEFILE_DIR)ivc/.*.cmd \
		$(MAKEFILE_DIR)uio_ivshmem/*.o $(MAKEFILE_DIR)uio_ivshmem/.*.cmd

rebuild: clean all
