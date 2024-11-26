#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>

#include <libdragon.h>

#include "libpressf/src/emu.h"
#include "libpressf/src/screen.h"

#include "main.h"
#include "emu.h"
#include "menu.h"

static const unsigned char bios_a[] = {};
static const unsigned int bios_a_size = 0;

static const unsigned char bios_b[] = {};
static const unsigned int bios_b_size = 0;

static const unsigned char rom[] = {};
static const unsigned int rom_size = 0;

pfu_emu_ctx_t emu;

void pfu_error_no_rom(void)
{
  console_clear();
  printf("Press F requires ROM data\n"
         "to be stored on the SD Card\n"
         "in the \"press-f\" directory.\n\n"
         "Please include the sl31253.bin\n"
         "and sl31254.bin BIOS images.\n\n"
         "Alternatively, it can be\n"
         "compiled in statically.\n\n"
         "See GitHub for instructions.");
  console_render();
  exit(1);
}

void pfu_state_set(pfu_state_type state)
{
  switch (emu.state)
  {
  case PFU_STATE_MENU:
    console_close();
    break;
  case PFU_STATE_EMU:
    break;
  default:
    break;
  }
  emu.state = state;
}

int main(void)
{
  /* Initialize controller */
#if PRESS_F_ULTRA_PREVIEW
  joypad_init();
#else
  controller_init();
#endif

  memset(&emu, 0, sizeof(emu));
  
  /* Initialize video */
  display_init(RESOLUTION_640x480, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);
  rdpq_init();
  emu.video_buffer = (uint16_t*)malloc_uncached_aligned(64, SCREEN_WIDTH * SCREEN_HEIGHT * 2);
  emu.video_frame = surface_make_linear(emu.video_buffer, FMT_RGBA16, SCREEN_WIDTH, SCREEN_HEIGHT);
  emu.video_scaling = PFU_SCALING_4_3;

  /* Initialize assets */
  dfs_init(DFS_DEFAULT_LOCATION);
  rdpq_font_t *font = rdpq_font_load("rom:/Tuffy_Bold.font64");
  rdpq_text_register_font(1, font);

  /* Initialize audio */
  audio_init(PF_SOUND_FREQUENCY, 2);

  /* Initialize emulator */
  pressf_init(&emu.system);
  f8_system_init(&emu.system, &pf_systems[0]);
  if (bios_a_size)
    f8_write(&emu.system, 0x0000, bios_a, bios_a_size);
  if (bios_b_size)
    f8_write(&emu.system, 0x0400, bios_b, bios_b_size);
  if (rom_size)
    f8_write(&emu.system, 0x0800, rom, rom_size);
  pfu_menu_init_roms();

  while (64)
  {
    switch (emu.state)
    {
    case PFU_STATE_MENU:
      pfu_menu_run();
      break;
    case PFU_STATE_EMU:
      pfu_emu_run();
      break;
    default:
      exit(0);
    }
    emu.frames++;
  }
}
