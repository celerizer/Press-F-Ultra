#include <libdragon.h>

#include "libpressf/src/emu.h"
#include "libpressf/src/font.h"

#include "main.h"
#include "menu.h"

int pfu_load_rom(unsigned address, const char *path)
{
  FILE *file;
  char buffer[0x0400];
  char fullpath[256];

  snprintf(fullpath, sizeof(fullpath), "sd:/press-f/%s", path);
  file = fopen(fullpath, "r");
  if (file)
  {
    int file_size;
    int i;

    printf("Loading %s...\n", fullpath);

    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);

    printf("Size: %04X\n", file_size);

    for (i = 0; i < file_size; i += 0x0400)
    {
      int bytes_read;

      bytes_read = fread(buffer, sizeof(char), 0x0400, file);
      f8_write(&emu.system, address + i, buffer, bytes_read);
      printf("Loaded %04X bytes to %04X\n", bytes_read, address + i);
    }
    fclose(file);

    return 1;
  }

  return 0;
}

static void pfu_menu_option_bool(pfu_entry_key key, bool value)
{
  switch (key)
  {
  case PFU_ENTRY_KEY_PIXEL_PERFECT:
    emu.video_scaling = value ? PFU_SCALING_1_1 : PFU_SCALING_4_3;
    break;
  default:
    break;
  }
}

static void pfu_menu_option_choice(pfu_entry_key key, unsigned value)
{
  switch (key)
  {
  case PFU_ENTRY_KEY_SYSTEM_MODEL:
    switch (value)
    {
    case 0:
      emu.system.cycles = F8_CLOCK_CHANNEL_F_NTSC;
      break;
    case 1:
      emu.system.cycles = F8_CLOCK_CHANNEL_F_PAL_GEN_1;
      break;
    case 2:
      emu.system.cycles = F8_CLOCK_CHANNEL_F_PAL_GEN_2;
      break;
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
    }
    break;
  default:  
    break;
  }
}

static void pfu_menu_file(const char *path)
{
  if (path)
  {
    pfu_load_rom(0x0800, path);
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

bool pfu_menu_init_roms(void)
{
  if (debug_init_sdfs("sd:/", -1))
  {
    dir_t dir;
    pfu_menu_ctx_t menu;
    int err = dir_findfirst("sd:/press-f", &dir);
    int count = 1;

    /* Set up dummy file entry to not load a ROM */
    snprintf(menu.entries[0].title, sizeof(menu.entries[0].title), "%s", "Boot to BIOS");
    menu.entries[0].type = PFU_ENTRY_TYPE_BACK;
    menu.entries[0].key = PFU_ENTRY_KEY_NONE;

    while (!err)
    {
      if (dir.d_type == DT_REG)
      {
        /* Load BIOS if found on SD Card */
        if (!strncmp(dir.d_name, "sl31253.bin", 8))
        {
          if (!emu.bios_a_loaded)
            emu.bios_a_loaded = pfu_load_rom(0x0000, dir.d_name);
        }
        else if (!strncmp(dir.d_name, "sl31254.bin", 8))
        {
          if (!emu.bios_b_loaded)
            emu.bios_b_loaded = pfu_load_rom(0x0400, dir.d_name);
        }
        else if (strlen(dir.d_name) && dir.d_name[0] != '.')
        {
          /* List all other files */
          snprintf(menu.entries[count].title, sizeof(dir.d_name), "%s", dir.d_name);
          menu.entries[count].type = PFU_ENTRY_TYPE_FILE;
          menu.entries[count].key = PFU_ENTRY_KEY_NONE;
          count++;
        }
      }
      err = dir_findnext("sd:/press-f", &dir); 
    }
    debug_close_sdfs();

    /* Fail if BIOS are not located */
    if (emu.bios_a_loaded && emu.bios_b_loaded)
    {
      menu.entry_count = count;
      menu.cursor = 0;
      snprintf(menu.menu_title, sizeof(menu.menu_title), "%s", "Press F Ultra - ROMs");
      snprintf(menu.menu_subtitle, sizeof(menu.menu_subtitle), "%s", "Select a ROM to load.");
      emu.menu = menu;
      emu.state = PFU_STATE_MENU;

      return true;
    }
  }
  debug_close_sdfs();

  return false;
}

static uint8_t sine_color;

static void pfu_menu_input(void)
{
  joypad_buttons_t buttons;

  joypad_poll();
  buttons = joypad_get_buttons_pressed(JOYPAD_PORT_1);
  if (buttons.d_up && emu.menu.cursor > 0)
    emu.menu.cursor--;
  else if (buttons.d_down && emu.menu.cursor < emu.menu.entry_count - 1)
    emu.menu.cursor++;
  else if (buttons.d_left && emu.menu.entries[emu.menu.cursor].type == PFU_ENTRY_TYPE_CHOICE)
  {
    emu.menu.entries[emu.menu.cursor].current_value--;
    if (emu.menu.entries[emu.menu.cursor].current_value < 0)
      emu.menu.entries[emu.menu.cursor].current_value = PFU_MENU_MAX_CHOICES - 1;
    pfu_menu_option_choice(emu.menu.entries[emu.menu.cursor].key, emu.menu.entries[emu.menu.cursor].current_value);
  }
  else if (buttons.d_right && emu.menu.entries[emu.menu.cursor].type == PFU_ENTRY_TYPE_CHOICE)
  {
    emu.menu.entries[emu.menu.cursor].current_value++;
    if (emu.menu.entries[emu.menu.cursor].current_value >= PFU_MENU_MAX_CHOICES)
      emu.menu.entries[emu.menu.cursor].current_value = 0;
    pfu_menu_option_choice(emu.menu.entries[emu.menu.cursor].key, emu.menu.entries[emu.menu.cursor].current_value);
  }
  else if (buttons.a)
  {
    switch (emu.menu.entries[emu.menu.cursor].type)
    {
    case PFU_ENTRY_TYPE_BOOL:
      pfu_menu_option_bool(emu.menu.entries[emu.menu.cursor].key, true);
      break;
    case PFU_ENTRY_TYPE_CHOICE:
      emu.menu.entries[emu.menu.cursor].current_value++;
      if (emu.menu.entries[emu.menu.cursor].current_value >= PFU_MENU_MAX_CHOICES)
        emu.menu.entries[emu.menu.cursor].current_value = 0;
      pfu_menu_option_choice(emu.menu.entries[emu.menu.cursor].key, emu.menu.entries[emu.menu.cursor].current_value);
      break;
    case PFU_ENTRY_TYPE_FILE:
      pfu_menu_file(emu.menu.entries[emu.menu.cursor].title);
      break;
    default:
      break;
    }
  }
  else if (buttons.b)
    pfu_state_set(PFU_STATE_EMU);
}

void pfu_menu_run(void)
{
#if PRESS_F_ULTRA_PREVIEW
  surface_t *disp = display_get();
  int i;

  rdpq_attach_clear(disp, NULL);
  rdpq_set_mode_fill(RGBA32(0x22, 0x22, 0x22, 1));
  rdpq_fill_rectangle(0, 0, display_get_width(), display_get_height());

  rdpq_set_mode_fill(RGBA32(sine_color, sine_color, 0x00, 1));
  rdpq_fill_rectangle(48 + 4, 32 + 64 + (emu.menu.cursor % 8) * 24 + 3, display_get_width() - (48 + 4), 32 + 64 + (emu.menu.cursor % 8) * 24 + 24 + 3);

  rdpq_set_mode_copy(true);
  rdpq_text_printf(NULL, 1, 48, 32 + 24, emu.menu.menu_title);
  rdpq_text_printf(NULL, 1, 48, 32 + 24 * 2, emu.menu.menu_subtitle);
  for (i = (emu.menu.cursor / 8) * 8; i < (emu.menu.cursor / 8) * 8 + 8; i++)
  {
    rdpq_text_printf(NULL, 1, 48 + 8, 32 + 64 + 24 + i * 24, emu.menu.entries[i].title);
    if (emu.menu.entries[i].type == PFU_ENTRY_TYPE_CHOICE)
      rdpq_text_printf(NULL, 1, 386, 32 + 64 + 24 + i * 24, emu.menu.entries[i].choices[emu.menu.entries[i].current_value]);
  }
  rdpq_detach_show();

  sine_color = (int)(sin(emu.frames * 0.1) * 127.0) + 128;
  pfu_menu_input();
#else
  /** 
   * This is the codepath for the old console-based menu. Only the ROM
   * selection menu is implemented here, and will be removed in the future.
   */
  bool finished = false;

  console_init();
  console_set_render_mode(RENDER_MANUAL);
  pfu_menu_init_roms();

  do
  {
    /* Process menu controller logic */
    struct controller_data keys;
    int i;

    controller_scan();
    keys = get_keys_down();

    if (keys.c[0].up && emu.menu.cursor > 0)
      emu.menu.cursor--;
    else if (keys.c[0].down && emu.menu.cursor < emu.menu.entry_count - 1)
      emu.menu.cursor++;
    else if (keys.c[0].A)
    {
      if (emu.menu.entries[emu.menu.cursor].type == PFU_ENTRY_TYPE_FILE)
        pfu_load_rom(0x0800, emu.menu.entries[emu.menu.cursor].title);
      else if (emu.menu.entries[emu.menu.cursor].type == PFU_ENTRY_TYPE_BACK)
      {
        /* Zero some data so it doesn't persist between boots */
        int dummy = 0;
        f8_write(&emu.system, 0x0800, &dummy, sizeof(dummy));
      }
      finished = true;
    }

    /* Render menu entries */
    console_clear();
    printf("\n\n");
    for (i = 0; i < emu.menu.entry_count; i++)
      printf("%c %s\n", i == emu.menu.cursor ? '>' : '-', emu.menu.entries[i].title);
    console_render();
  } while (!finished);
  pfu_state_set(PFU_STATE_EMU);
#endif
}
