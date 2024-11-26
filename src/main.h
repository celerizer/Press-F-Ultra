#ifndef PRESS_F_ULTRA_MAIN_H
#define PRESS_F_ULTRA_MAIN_H

#include "libpressf/src/emu.h"

#include "menu.h"

typedef enum
{
  PFU_SCALING_1_1 = 0,
  PFU_SCALING_4_3,

  PFU_SCALING_SIZE
} pfu_scaling_type;

typedef enum
{
  PFU_STATE_INVALID = 0,

  PFU_STATE_MENU,
  PFU_STATE_EMU,

  PFU_STATE_SIZE
} pfu_state_type;

typedef struct
{
  u16* video_buffer;
  surface_t video_frame;
  pfu_scaling_type video_scaling;
  pfu_state_type state;
  f8_system_t system;
  bool bios_a_loaded;
  bool bios_b_loaded;
  pfu_menu_ctx_t menu_roms;
  pfu_menu_ctx_t menu_settings;
  pfu_menu_ctx_t *current_menu;
  unsigned frames;
  sprite_t *icon;
} pfu_emu_ctx_t;

extern pfu_emu_ctx_t emu;

void pfu_state_set(pfu_state_type state);

#endif
