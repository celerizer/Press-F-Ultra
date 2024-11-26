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
  joypad_init();

  memset(&emu, 0, sizeof(emu));
  
  /* Initialize video */
  display_init(RESOLUTION_640x480, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);
  rdpq_init();
  emu.video_buffer = (uint16_t*)malloc_uncached_aligned(64, SCREEN_WIDTH * SCREEN_HEIGHT * 2);
  emu.video_frame = surface_make_linear(emu.video_buffer, FMT_RGBA16, SCREEN_WIDTH, SCREEN_HEIGHT);
  emu.video_scaling = PFU_SCALING_4_3;

  /* Initialize assets */
  dfs_init(DFS_DEFAULT_LOCATION);

  /* Initialize fonts */
  rdpq_font_t *font1 = rdpq_font_load("rom:/Tuffy_Bold.font64");
  rdpq_text_register_font(1, font1);
  rdpq_font_style(font1, 0, &(rdpq_fontstyle_t){
	                .color = RGBA32(255, 255, 255, 255),
	                .outline_color = RGBA32(0, 0, 0, 255)});
  rdpq_font_t *font2 = rdpq_font_load("rom:/Tuffy_Bold.font64");
  rdpq_text_register_font(2, font2);
  rdpq_font_style(font2, 0, &(rdpq_fontstyle_t){
	                .color = RGBA32(0, 0, 0, 127),
	                .outline_color = RGBA32(0, 0, 0, 127)});

  emu.icon = sprite_load("rom:/icon.sprite");
  debug_init_sdfs("sd:/", -1);

  /* Initialize audio */
  audio_init(PF_SOUND_FREQUENCY, 4);

  /* Initialize emulator */
  pressf_init(&emu.system);
  f8_system_init(&emu.system, &pf_systems[0]);
  pfu_menu_init();
  pfu_menu_switch_roms();

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
