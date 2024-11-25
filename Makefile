all: Press-F.z64
.PHONY: all

CFLAGS += \
	-DPF_BIG_ENDIAN=1 \
	-DPF_HAVE_HLE_BIOS=0 \
	-DPF_SOUND_FREQUENCY=22050 \
	-DPF_ROMC=0 \
	-DPRESS_F_ULTRA_PREVIEW=1 \
	-O2 -funroll-loops \
	-std=c89 -Wall -Wextra

BUILD_DIR = build
SRC_DIR = src
include $(N64_INST)/include/n64.mk

OBJS = \
	$(BUILD_DIR)/src/emu.o \
	$(BUILD_DIR)/src/main.o \
	$(BUILD_DIR)/src/menu.o \
	$(BUILD_DIR)/src/libpressf/src/debug.o \
	$(BUILD_DIR)/src/libpressf/src/dma.o \
	$(BUILD_DIR)/src/libpressf/src/emu.o \
	$(BUILD_DIR)/src/libpressf/src/font.o \
	$(BUILD_DIR)/src/libpressf/src/hle.o \
	$(BUILD_DIR)/src/libpressf/src/input.o \
	$(BUILD_DIR)/src/libpressf/src/romc.o \
	$(BUILD_DIR)/src/libpressf/src/screen.o \
	$(BUILD_DIR)/src/libpressf/src/software.o \
	$(BUILD_DIR)/src/libpressf/src/wave.o \
	$(BUILD_DIR)/src/libpressf/src/hw/2102.o \
	$(BUILD_DIR)/src/libpressf/src/hw/2114.o \
	$(BUILD_DIR)/src/libpressf/src/hw/3850.o \
	$(BUILD_DIR)/src/libpressf/src/hw/3851.o \
	$(BUILD_DIR)/src/libpressf/src/hw/beeper.o \
	$(BUILD_DIR)/src/libpressf/src/hw/f8_device.o \
	$(BUILD_DIR)/src/libpressf/src/hw/fairbug_parallel.o \
	$(BUILD_DIR)/src/libpressf/src/hw/hand_controller.o \
	$(BUILD_DIR)/src/libpressf/src/hw/schach_led.o \
	$(BUILD_DIR)/src/libpressf/src/hw/selector_control.o \
	$(BUILD_DIR)/src/libpressf/src/hw/system.o \
	$(BUILD_DIR)/src/libpressf/src/hw/vram.o

# Get the current git version
GIT_VERSION := $(shell git describe --tags --dirty --always)

# Define the N64 ROM title with the git version
N64_ROM_TITLE_WITH_VERSION := "Press F $(GIT_VERSION)"

Press-F.z64: N64_ROM_TITLE = $(N64_ROM_TITLE_WITH_VERSION)

$(BUILD_DIR)/Press-F.elf: $(OBJS)

clean:
	rm -rf $(BUILD_DIR) *.z64
.PHONY: clean

-include $(wildcard $(BUILD_DIR)/*.d)
