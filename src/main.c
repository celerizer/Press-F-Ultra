#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include <libdragon.h>
#include "libcart/cart.h"

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

/**
 * Returns the memory location of a stamped ROM when loading as a plugin, or
 * 0 if doing so is not supported.
 */
static unsigned long pfu_plugin_rom_address(void)
{
  switch (cart_type)
  {
  case CART_CI:
  case CART_SC:
    return 0x10200000;
  case CART_EDX:
  case CART_ED:
    return 0xB0200000;
  default:
    return 0;
  }
}

/**
 * Attempts to load a Channel F ROM stamped in an address by the previous
 * program loader. Returns true if a ROM was successfully read, if the plugin
 * feature is available.
 * 
 * The Channel F sanity byte $55 is checked to ensure the ROM is valid, which
 * may exclude some older homebrew ROMs.
 * 
 * As the accurate ROMC mode is not used on Nintendo 64, a maximum-sized ROM
 * can be loaded contiguously into the entire F8 address space, then
 * overwritten later.
 */
static bool pfu_plugin_read_rom(void)
{
  u8 buffer[0xF800];
  const unsigned long base = pfu_plugin_rom_address();

  if (!base)
    return false;
  else
  {
    dma_read_async(buffer, base, sizeof(buffer));
    dma_wait();

    if (buffer[0] == 0x55)
    {
      f8_write(&emu.system, 0x0800, buffer, sizeof(buffer));
      memset(buffer, 0, sizeof(buffer));
      dma_write_raw_async(buffer, base, sizeof(buffer));
      dma_wait();

      return true;
    }
  }

  return false;
}

int main(void)
{
  memset(&emu, 0, sizeof(emu));

  /* Initialize controller */
  joypad_init();
  
  /* Initialize video */
  display_init(RESOLUTION_640x480, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);
  rdpq_init();
  emu.video_buffer = (u16*)malloc_uncached_aligned(64, SCREEN_WIDTH * SCREEN_HEIGHT * 2);
  emu.video_frame = surface_make_linear(emu.video_buffer, FMT_RGBA16, SCREEN_WIDTH, SCREEN_HEIGHT);
  emu.video_scaling = PFU_SCALING_4_3;

  /* Initialize assets */
  cart_init();
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

  /* If loaded as plugin, jump to loaded ROM, otherwise load ROM menu */
  if (pfu_plugin_read_rom())
    pfu_state_set(PFU_STATE_EMU);
  else
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
