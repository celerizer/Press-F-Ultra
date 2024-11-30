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
static unsigned pfu_plugin_rom_address(void)
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
      return NULL; // This is an emulator or something else.
  }
}

/**
 * Verifies a section of a supposed Channel F ROM in memory by testing if
 * its contents contain a significant (and arbitrary) number of zeroes.
 * Currently, the arbitrary limit is to fail if 75% of the contents are zero.
 */
static bool pfu_plugin_verify_section(const u8 *buffer, unsigned size)
{
  unsigned zeroes = 0;
  unsigned i;

  for (i = 0; i < size; i++)
  {
    if (buffer[i] == 0)
      zeroes++;
  }

  return zeroes < (size / 4) * 3;
}

/**
 * Attempts to load a Channel F ROM stamped in memory by the previous program
 * loader. Returns true if a ROM was successfully read, if the plugin feature
 * is available.
 * @todo Perhaps test for the 0x55 identifier
 */
static bool pfu_plugin_read_rom(void)
{
  const unsigned base = pfu_plugin_rom_address();
  unsigned address = 0;

  if (!base)
    return false;
  else
  {
    u8 buffer[0x0200];
    bool success;

    do
    {
      dma_read_async(&buffer, base + address, sizeof(buffer));
      dma_wait();

      success = pfu_plugin_verify_section(buffer, sizeof(buffer));
      if (success)
      {
        f8_write(&emu.system, 0x0800 + address, buffer, sizeof(buffer));
        address += sizeof(buffer);
      }
    } while (success && address < 0xF800);
  }

  return address > 0;
}

int main(void)
{
  /* Initialize flashcart detection */
  cart_init();

  /* Initialize controller */
  joypad_init();

  memset(&emu, 0, sizeof(emu));
  
  /* Initialize video */
  display_init(RESOLUTION_640x480, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);
  rdpq_init();
  emu.video_buffer = (u16*)malloc_uncached_aligned(64, SCREEN_WIDTH * SCREEN_HEIGHT * 2);
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
