#include <libdragon.h>

#include "libpressf/src/emu.h"
#include "libpressf/src/font.h"

#include "main.h"
#include "menu.h"

enum
{
  PFU_SOURCE_INVALID = 0,

  PFU_SOURCE_ROMFS,
  PFU_SOURCE_SD_CARD,

  PFU_SOURCE_SIZE
};

int pfu_load_rom(unsigned address, const char *path, unsigned source)
{
  FILE *file;
  char buffer[0x0400];
  char fullpath[256];
  int success = 0;

  if (source == PFU_SOURCE_INVALID || source >= PFU_SOURCE_SIZE)
    return 0;

  snprintf(fullpath, sizeof(fullpath), "%s/%s", (source == PFU_SOURCE_SD_CARD) ? "sd:/press-f" : "rom:/roms", path);
  file = fopen(fullpath, "r");
  if (file)
  {
    int file_size;
    int i;

    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);

    for (i = 0; i < file_size; i += 0x0400)
    {
      int bytes_read = fread(buffer, sizeof(char), 0x0400, file);

      f8_write(&emu.system, address + i, buffer, bytes_read);
    }
    fclose(file);
    success = 1;
  }

  return success;
}

static void pfu_menu_option_bool(pfu_menu_entry_t *entry, bool value)
{
  if (!entry)
    return;
  else switch (entry->key)
  {
  case PFU_ENTRY_KEY_PIXEL_PERFECT:
    emu.video_scaling = value ? PFU_SCALING_1_1 : PFU_SCALING_4_3;
    break;
  default:
    return;
  }
  entry->current_value = value;
}

static void pfu_menu_option_choice(pfu_menu_entry_t *entry, signed value)
{
  if (!entry)
    return;
  else switch (entry->key)
  {
  case PFU_ENTRY_KEY_SYSTEM_MODEL:
    switch (value)
    {
    case 0:
      emu.system.settings.f3850_clock_speed = F8_CLOCK_CHANNEL_F_NTSC;
      break;
    case 1:
      emu.system.settings.f3850_clock_speed = F8_CLOCK_CHANNEL_F_PAL_GEN_1;
      break;
    case 2:
      emu.system.settings.f3850_clock_speed = F8_CLOCK_CHANNEL_F_PAL_GEN_2;
      break;
    default:
      return;
    }
    break;
  case PFU_ENTRY_KEY_FONT:
    switch (value)
    {
    case 0:
      font_load(&emu.system, FONT_FAIRCHILD);
      break;
    case 1:
      font_load(&emu.system, FONT_CUTE);
      break;
    case 2:
      font_load(&emu.system, FONT_SKINNY);
      break;
    default:
      return;
    }
    break;
  default:  
    return;
  }
  entry->current_value = value;
}

static void pfu_menu_file(pfu_menu_entry_t *entry)
{
  if (entry)
  {
    pfu_load_rom(0x0800, entry->title, entry->current_value);
    pfu_state_set(PFU_STATE_EMU);
    pressf_reset(&emu.system);
  }
}

bool pfu_menu_init_settings(void)
{
  pfu_menu_ctx_t menu;
  pfu_menu_entry_t *entry;

  memset(&menu, 0, sizeof(menu));

  snprintf(menu.menu_title, sizeof(menu.menu_title), "%s", "Press F Ultra - Settings");
  snprintf(menu.menu_subtitle, sizeof(menu.menu_subtitle), "%s", "Select a setting to change.");

  entry = &menu.entries[0];
  entry->key = PFU_ENTRY_KEY_PIXEL_PERFECT;
  entry->type = PFU_ENTRY_TYPE_BOOL;
  snprintf(entry->title, sizeof(entry->title), "%s", "Pixel-perfect scaling");

  entry = &menu.entries[1];
  entry->key = PFU_ENTRY_KEY_SYSTEM_MODEL;
  entry->type = PFU_ENTRY_TYPE_CHOICE;
  snprintf(entry->title, sizeof(entry->title), "%s", "System CPU clock");
  snprintf(entry->choices[0], sizeof(entry->choices[0]), "%s", "NTSC (1.79 MHz)");
  snprintf(entry->choices[1], sizeof(entry->choices[1]), "%s", "PAL Gen I (2.00 MHz)");
  snprintf(entry->choices[2], sizeof(entry->choices[2]), "%s", "PAL Gen II (1.97 MHz)");

  entry = &menu.entries[2];
  entry->key = PFU_ENTRY_KEY_FONT;
  entry->type = PFU_ENTRY_TYPE_CHOICE;
  snprintf(entry->title, sizeof(entry->title), "%s", "System font");
  snprintf(entry->choices[0], sizeof(entry->choices[0]), "%s", "Fairchild");
  snprintf(entry->choices[1], sizeof(entry->choices[1]), "%s", "Cute");
  snprintf(entry->choices[2], sizeof(entry->choices[2]), "%s", "Skinny");

  menu.entry_count = 3;

  emu.menu = menu;
  emu.state = PFU_STATE_MENU;

  return true;
}

static void pfu_menu_init_roms_source(pfu_menu_ctx_t *menu, const char *src_path, int src)
{
  dir_t dir;
  int err = dir_findfirst(src_path, &dir); 

  while (!err)
  {
    if (dir.d_type == DT_REG)
    {
      /* Load BIOS if found on SD Card */
      if (!strncmp(dir.d_name, "sl31253.bin", 8))
      {
        if (!emu.bios_a_loaded)
          emu.bios_a_loaded = pfu_load_rom(0x0000, dir.d_name, src);
      }
      else if (!strncmp(dir.d_name, "sl31254.bin", 8))
      {
        if (!emu.bios_b_loaded)
          emu.bios_b_loaded = pfu_load_rom(0x0400, dir.d_name, src);
      }
      else if (strlen(dir.d_name) && dir.d_name[0] != '.')
      {
        /* List all other files */
        snprintf(menu->entries[menu->entry_count].title, sizeof(dir.d_name), "%s", dir.d_name);
        menu->entries[menu->entry_count].type = PFU_ENTRY_TYPE_FILE;
        menu->entries[menu->entry_count].current_value = src;
        menu->entry_count++;
      }
    }
    err = dir_findnext(src_path, &dir); 
  }
}

bool pfu_menu_init_roms(void)
{
  bool success = false;
  pfu_menu_ctx_t menu;

  /* Set up dummy file entry to not load a ROM */
  snprintf(menu.entries[0].title, sizeof(menu.entries[0].title), "%s", "Boot to BIOS");
  menu.entries[0].type = PFU_ENTRY_TYPE_BACK;
  menu.entries[0].key = PFU_ENTRY_KEY_NONE;
  menu.entry_count = 1;

  pfu_menu_init_roms_source(&menu, "rom:/roms", PFU_SOURCE_ROMFS);
  pfu_menu_init_roms_source(&menu, "sd:/press-f", PFU_SOURCE_SD_CARD);

  /* Fail if BIOS are not located */
  if (emu.bios_a_loaded && emu.bios_b_loaded)
  {
    snprintf(menu.menu_title, sizeof(menu.menu_title), "%s", "Press F Ultra - ROMs");
    snprintf(menu.menu_subtitle, sizeof(menu.menu_subtitle), "%s", "Select a ROM to load.");
    menu.cursor = 0;
    emu.menu = menu;
    emu.state = PFU_STATE_MENU;

    success = true;
  }

  return success;
}

static uint8_t sine_color;

#define PFU_DROP 3
#define PFU_ROWS 12

static void pfu_menu_input(void)
{
  joypad_buttons_t buttons;
  pfu_menu_entry_t *entry = &emu.menu.entries[emu.menu.cursor];

  joypad_poll();
  buttons = joypad_get_buttons_pressed(JOYPAD_PORT_1);
  if (buttons.d_up)
    emu.menu.cursor--;
  else if (buttons.d_down)
    emu.menu.cursor++;
  else if (buttons.d_left)
    switch (entry->type)
    {
    case PFU_ENTRY_TYPE_BOOL:
      pfu_menu_option_bool(entry, false);
      break;
    case PFU_ENTRY_TYPE_CHOICE:
      pfu_menu_option_choice(entry, entry->current_value - 1);
      break;
    case PFU_ENTRY_TYPE_FILE:
      emu.menu.cursor -= PFU_ROWS;
    default:
      return;
    }
  else if (buttons.d_right)
    switch (entry->type)
    {
    case PFU_ENTRY_TYPE_BOOL:
      pfu_menu_option_bool(entry, true);
      break;
    case PFU_ENTRY_TYPE_CHOICE:
      pfu_menu_option_choice(entry, entry->current_value + 1);
      break;
    default:
      emu.menu.cursor += PFU_ROWS;
    }
  else if (buttons.a)
  {
    switch (entry->type)
    {
    case PFU_ENTRY_TYPE_BOOL:
      pfu_menu_option_bool(entry, !entry->current_value);
      break;
    case PFU_ENTRY_TYPE_CHOICE:
      pfu_menu_option_choice(entry, entry->current_value + 1);
      break;
    case PFU_ENTRY_TYPE_FILE:
      pfu_menu_file(entry);
      break;
    default:
      return;
    }
  }
  else if (buttons.b)
    pfu_state_set(PFU_STATE_EMU);
  
  if (emu.menu.cursor < 0)
    emu.menu.cursor = 0;
  else if (emu.menu.cursor >= emu.menu.entry_count)
    emu.menu.cursor = emu.menu.entry_count - 1;
}

void pfu_menu_run(void)
{
  surface_t *disp = display_get();
  int i;

  rdpq_attach_clear(disp, NULL);
  rdpq_set_mode_fill(RGBA32(0x22, 0x22, 0x22, 1));
  rdpq_fill_rectangle(0, 0, display_get_width(), display_get_height());

  rdpq_set_mode_fill(RGBA32(sine_color, sine_color, 0x00, 1));
  rdpq_fill_rectangle(48 + 4, 32 + 64 + (emu.menu.cursor % PFU_ROWS) * 24 + 6, display_get_width() - (48 + 4), 32 + 64 + (emu.menu.cursor % PFU_ROWS) * 24 + 24 + 6);

  rdpq_set_mode_copy(true);

  rdpq_sprite_blit(emu.icon, 48, 32, NULL);

  rdpq_text_printf(NULL, 1, 64 + 48 + 8, 32 + 24, emu.menu.menu_title);
  rdpq_text_printf(NULL, 1, 64 + 48 + 8, 32 + 24 * 2, emu.menu.menu_subtitle);
  for (i = (emu.menu.cursor / PFU_ROWS) * PFU_ROWS; i < (emu.menu.cursor / PFU_ROWS) * PFU_ROWS + PFU_ROWS && i < emu.menu.entry_count; i++)
  {
    int j = i % PFU_ROWS;

    if (i == emu.menu.cursor)
      rdpq_text_printf(NULL, 2, 48 + 8 + PFU_DROP, 32 + 64 + 24 + j * 24 + PFU_DROP, emu.menu.entries[i].title);
    rdpq_text_printf(NULL, 1, 48 + 8, 32 + 64 + 24 + j * 24, emu.menu.entries[i].title);
    if (emu.menu.entries[i].type == PFU_ENTRY_TYPE_BOOL)
      rdpq_text_printf(NULL, 1, 386, 32 + 64 + 24 + j * 24, emu.menu.entries[i].current_value ? "Enabled" : "Disabled");
    else if (emu.menu.entries[i].type == PFU_ENTRY_TYPE_CHOICE)
      rdpq_text_printf(NULL, 1, 386, 32 + 64 + 24 + j * 24, emu.menu.entries[i].choices[emu.menu.entries[i].current_value]);
  }
  rdpq_detach_show();

  sine_color = (int)(sin(emu.frames * 0.1) * 127.0) + 128;
  pfu_menu_input();
}
