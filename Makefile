all: Press-F.z64
.PHONY: all

CFLAGS += \
	-DPF_BIG_ENDIAN=1 \
	-DPF_HAVE_HLE_BIOS=0 \
	-DPF_SOUND_FREQUENCY=44100 \
	-DPF_ROMC=0 \
	-O2 -funroll-loops \
	-std=c89 -Wall -Wextra

BUILD_DIR = build
SRC_DIR = src
include $(N64_INST)/include/n64.mk
include src/libpressf/libpressf.mk

assets_fnt = $(wildcard assets/*.fnt)
assets_ttf = $(wildcard assets/*.ttf)
assets_png = $(wildcard assets/*.png)

assets_bin = $(wildcard roms/*.bin)
assets_chf = $(wildcard roms/*.chf)
assets_rom = $(wildcard roms/*.rom)

assets_conv = \
	$(addprefix filesystem/,$(notdir $(assets_ttf:%.ttf=%.font64))) \
	$(addprefix filesystem/,$(notdir $(assets_fnt:%.fnt=%.font64))) \
    $(addprefix filesystem/,$(notdir $(assets_png:%.png=%.sprite))) \
	$(addprefix filesystem/roms/,$(notdir $(assets_bin:%.bin=%.bin))) \
	$(addprefix filesystem/roms/,$(notdir $(assets_chf:%.chf=%.chf))) \
	$(addprefix filesystem/roms/,$(notdir $(assets_rom:%.rom=%.rom)))

MKSPRITE_FLAGS ?=
MKFONT_FLAGS ?= --range all

src = \
	$(SRC_DIR)/emu.c \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/menu.c

src += $(PRESS_F_SOURCES)

filesystem/%.font64: assets/%.ttf
	@mkdir -p $(dir $@)
	@echo "    [FONT] $@"
	$(N64_MKFONT) $(MKFONT_FLAGS) -o filesystem "$<"

filesystem/%.font64: assets/%.fnt
	@mkdir -p $(dir $@)
	@echo "    [FONT] $@"
	$(N64_MKFONT) $(MKFONT_FLAGS) -o filesystem "$<"

filesystem/%.sprite: assets/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -o filesystem "$<"

filesystem/roms/%: roms/%
	@mkdir -p "$(dir $@)"
	@echo "    [ROM] $@"
	@cp "$<" "$@"

filesystem/Tuffy_Bold.font64: MKFONT_FLAGS += --size 18 --outline 1

$(BUILD_DIR)/Press-F.dfs: $(assets_conv) 
$(BUILD_DIR)/Press-F.elf: $(src:%.c=$(BUILD_DIR)/%.o)

# Get the current git version
GIT_VERSION := $(shell git describe --tags --dirty --always)

# Define the N64 ROM title with the git version
N64_ROM_TITLE_WITH_VERSION := "Press F $(GIT_VERSION)"

Press-F.z64: N64_ROM_TITLE = $(N64_ROM_TITLE_WITH_VERSION)
Press-F.z64: $(BUILD_DIR)/Press-F.dfs

clean:
	rm -rf $(BUILD_DIR) filesystem *.z64

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: clean
