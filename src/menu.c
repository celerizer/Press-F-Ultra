#include <libdragon.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "libpressf/src/emu.h"
#include "libpressf/src/font.h"

#include "emu.h"
#include "error.h"
#include "main.h"
#include "menu.h"
#include "FastLZ/fastlz.h"

enum
{
  PFU_SOURCE_INVALID = 0,

  PFU_SOURCE_ROMFS,
  PFU_SOURCE_SD_CARD,
  PFU_SOURCE_CONTROLLER_PAK,

  PFU_SOURCE_SIZE
};

#define PFU_PATH_CONTROLLER_PAK "cpak1:/HF8E.01"
#define PFU_PATH_ROMFS "rom:/roms"
#define PFU_PATH_SD_CARD "sd:/press-f"

static const u16 pfu_compression_magic = 0xF8CF;
static bool pfu_pak_connected = false;

typedef struct
{
  u16 magic;
  u16 original_size;
  u16 compressed_size;
} pfu_compression_header_t;

static int pfu_load_file(void *dst, unsigned size, const char *path, unsigned source)
{
  if (source == PFU_SOURCE_INVALID || source >= PFU_SOURCE_SIZE)
    return 0;
  else
  {
    FILE *file;
    const char *prefix = NULL;
    char fullpath[1024];

    switch (source)
    {
    case PFU_SOURCE_CONTROLLER_PAK:
      prefix = PFU_PATH_CONTROLLER_PAK;
      break;
    case PFU_SOURCE_ROMFS:
      prefix = PFU_PATH_ROMFS;
      break;
    case PFU_SOURCE_SD_CARD:
      prefix = PFU_PATH_SD_CARD;
      break;
    default:
      pfu_message_switch(PFU_STATE_MENU,
        "Invalid source for loading file: %u", source);
    }
    snprintf(fullpath, sizeof(fullpath), "%s/%s", prefix, path);

    if (source == PFU_SOURCE_CONTROLLER_PAK)
      cpakfs_mount(JOYPAD_PORT_1, "cpak1:/");
    file = fopen(fullpath, "rb");
    if (file)
    {
      size_t bytes_read = fread(dst, sizeof(char), size, file);
      fclose(file);

      if (source == PFU_SOURCE_CONTROLLER_PAK)
      {
        pfu_compression_header_t header;
        u8 *output;
        u8 *compressed_ptr;
        int decompressed_size;

        /* Read compression header */
        memcpy(&header, dst, sizeof(header));
        if (header.magic != pfu_compression_magic)
        {
          pfu_message_switch(PFU_STATE_MENU,
            "Invalid Controller Pak file format:\n%s\n\n"
            "Expected magic: 0x%08X, got: 0x%08X",
            fullpath, pfu_compression_magic, header.magic);
          bytes_read = 0;
        }

        /* Decompress file */
        output = (u8*)malloc(header.original_size);
        compressed_ptr = ((u8*)dst) + sizeof(header);
        decompressed_size = fastlz_decompress(compressed_ptr,
          header.compressed_size, output, header.original_size);
        if (decompressed_size <= 0)
        {
          pfu_message_switch(PFU_STATE_MENU,
            "Failed to decompress Controller Pak data:\n%s\n\n"
            "Header original size: %i\n"
            "Header compressed size: %i\n"
            "Decompressed size: %i\n", strerror(errno),
            header.original_size, header.compressed_size, decompressed_size);
          bytes_read = 0;
        }
        else
        {
          memcpy(dst, output, decompressed_size);
          bytes_read = decompressed_size;
        }
        cpakfs_unmount(JOYPAD_PORT_1);
        free(output);
      }

      return bytes_read;
    }
    else
      pfu_message_switch(PFU_STATE_MENU, 
        "Failed to open file for reading:\n%s\n", fullpath);
  }

  return 0;
}

static int pfu_controller_pak_write(const char *path, unsigned source)
{
  if (!cpakfs_mount(JOYPAD_PORT_1, "cpak1:/"))
  {
    FILE *output_file;
    u8 rom_data[0x4000];
    u8 compressed_rom_data[0x4000];
    unsigned size;
    pfu_compression_header_t header;
    char formatted_path[64];
    char temp_path[32] = { 0 };
    size_t bytes_written = 0;
    cpakfs_stats_t stats;
    int pages_needed;
    unsigned i, j, last_char_was_space = 0;

    /* Format Controller Pak if needed */
    if (cpakfs_fsck(JOYPAD_PORT_1, false, NULL))
      pfu_message_switch(PFU_STATE_MENU,
        "The Controller Pak needs to be formatted.\n\n"
        "Please format it in a compatible game and try again.");

    /* Load the file to be copied to Controller Pak */
    header.original_size = pfu_load_file(rom_data, sizeof(rom_data), path, source);
    if (!header.original_size)
    {
      pfu_message_switch(PFU_STATE_MENU,
        "Failed to load ROM data.\n\n"
        "Please check the file path and try again.");
      goto error;
    }

    /* Compress the file */
    header.compressed_size = fastlz_compress_level(2, rom_data, header.original_size, compressed_rom_data);
    if (!header.compressed_size)
    {
      pfu_message_switch(PFU_STATE_MENU,
        "Failed to compress ROM data.");
      goto error;
    }

    /* Add space for header */
    size = header.compressed_size + sizeof(pfu_compression_header_t);

    /* Check if the Controller Pak has space to hold it */
    cpakfs_get_stats(JOYPAD_PORT_1, &stats);
    pages_needed = size % 256 == 0 ? size / 256 : size / 256 + 1;
    if (stats.pages.used + pages_needed > stats.pages.total ||
        stats.notes.used + 1 > stats.notes.total)
    {
      signed pages_free, notes_free;

      pages_free = stats.pages.total - stats.pages.used;
      notes_free = stats.notes.total - stats.notes.used;
      pfu_message_switch(PFU_STATE_MENU, "Not enough space on Controller Pak.\n\n"
        "Required: %i pages, 1 note\n"
        "Available: %i pages, %i notes\n\n"
        "Please free some space and try again.",
        pages_needed, pages_free, notes_free);

      goto error;
    }

    /**
     * Format ROM name to Controller Pak format: 16 characters, uppercase
     * letters and spaces only. Trim any tags like (USA).
     */
    for (i = 0, j = 0; i < 256 && path[i] != '\0' && j < 16 && path[i] != '(' && path[i] != '.'; i++)
    {
      char c = path[i];

      /* Convert lowercase letters to uppercase */
      if (c >= 'a' && c <= 'z')
        c -= 0x20;

      /* Acceptable characters: A-Z, 0-9, space, hyphen */
      if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == ' ')
      {
        /* Collapse multiple spaces */
        if (c == ' ')
        {
          if (last_char_was_space)
            continue;
          last_char_was_space = 1;
        }
        else
          last_char_was_space = 0;

        temp_path[j++] = c;
      }
      else
      {
        /* Replace other characters with space, but avoid multiple spaces */
        if (!last_char_was_space)
        {
          temp_path[j++] = ' ';
          last_char_was_space = 1;
        }
      }
    }
    temp_path[j] = '\0';

    /* Remove trailing space if present */
    if (j > 0 && temp_path[j - 1] == ' ')
      temp_path[j - 1] = '\0';
    snprintf(formatted_path, sizeof(formatted_path), "%s/%s.CHF", PFU_PATH_CONTROLLER_PAK, temp_path);

    /* Create new file on Controller Pak */
    output_file = fopen(formatted_path, "wb");
    if (!output_file || ferror(output_file))
    {
      pfu_message_switch(PFU_STATE_MENU,
        "Failed to open file for writing:\n%s", strerror(errno));
      goto error;
    }

    /* Write ROM to the file */
    header.magic = pfu_compression_magic;
    bytes_written += fwrite(&header, 1, sizeof(header), output_file);
    bytes_written += fwrite(compressed_rom_data, sizeof(char), header.compressed_size, output_file);
    fclose(output_file);
    if (bytes_written != size)
    {
      pfu_message_switch(PFU_STATE_MENU,
        "Failed to write data to file:\n%s\n\n"
        "Expected: %u\nWritten: %u", strerror(errno), size, bytes_written);
      goto error;
    }
    else
    {
      unsigned pages_free, notes_free;

      cpakfs_get_stats(JOYPAD_PORT_1, &stats);
      pages_free = stats.pages.total - stats.pages.used;
      notes_free = stats.notes.total - stats.notes.used;
      pfu_message_switch(PFU_STATE_MENU,
        "ROM successfully saved to Controller Pak.\n"
        "You can now load it from the ROMs menu.\n\n"
        "Name: %s\n"
        "Size: 1 note, %i pages\n\n"
        "Remaining space on Controller Pak:\n"
        "Pages free: %i / %i\n"
        "Notes free: %i / %i",
        formatted_path, pages_needed, pages_free,
        stats.pages.total, notes_free, stats.notes.total);
      cpakfs_unmount(JOYPAD_PORT_1);

      return 1;
    }
  }
  pfu_message_switch(PFU_STATE_MENU,
    "Press F Ultra requires a Controller Pak to be inserted in\n"
    "the first joypad port to save ROMs to it.\n\n"
    "Please insert a Controller Pak and try again.");
error:
  cpakfs_unmount(JOYPAD_PORT_1);
  return 0;
}

static int pfu_load_rom(unsigned address, const char *path, unsigned source)
{
  if (source == PFU_SOURCE_INVALID || source >= PFU_SOURCE_SIZE)
    return 0;
  else
    return pfu_load_file(&emu.system.memory[address], 0x4000, path, source);
}

static void pfu_menu_entry_back(void)
{
  unsigned dummy = 0;

  f8_write(&emu.system, 0x0800, &dummy, sizeof(dummy));
  pfu_emu_switch();
  pressf_reset(&emu.system);
}

static void pfu_menu_entry_bool(pfu_menu_entry_t *entry, bool value)
{
  if (!entry)
    return;
  else switch (entry->key)
  {
  case PFU_ENTRY_KEY_SWAP_CONTROLLERS:
    emu.swap_controllers = value;
    break;
  case PFU_ENTRY_KEY_PIXEL_PERFECT:
    emu.video_scaling = value ? PFU_SCALING_1_1 : PFU_SCALING_4_3;
    break;
  default:
    return;
  }
  entry->current_value = value;
}

static void pfu_menu_entry_choice(pfu_menu_entry_t *entry, signed value)
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

static void pfu_menu_entry_file(pfu_menu_entry_t *entry)
{
  if (entry)
  {
    pfu_load_rom(0x0800, entry->title, entry->current_value);
    pfu_emu_switch();
    pressf_reset(&emu.system);
  }
}

static void pfu_menu_init_settings(void)
{
  pfu_menu_ctx_t menu;
  pfu_menu_entry_t *entry;
  const unsigned entry_count = 4;
  unsigned i = 0;

  memset(&menu, 0, sizeof(menu));
  menu.entries = calloc(entry_count, sizeof(pfu_menu_entry_t));
  menu.entry_count = entry_count;

  snprintf(menu.menu_title, sizeof(menu.menu_title), "%s", "Press F Ultra - Settings");
  snprintf(menu.menu_subtitle, sizeof(menu.menu_subtitle), "%s", "Select a setting to change.");

  entry = &menu.entries[i];
  entry->key = PFU_ENTRY_KEY_SWAP_CONTROLLERS;
  entry->type = PFU_ENTRY_TYPE_BOOL;
  snprintf(entry->title, sizeof(entry->title), "%s", "Swap player 1 / player 2 controller");
  i++;

  entry = &menu.entries[i];
  entry->key = PFU_ENTRY_KEY_PIXEL_PERFECT;
  entry->type = PFU_ENTRY_TYPE_BOOL;
  snprintf(entry->title, sizeof(entry->title), "%s", "Pixel-perfect scaling");
  i++;

  entry = &menu.entries[i];
  entry->key = PFU_ENTRY_KEY_SYSTEM_MODEL;
  entry->type = PFU_ENTRY_TYPE_CHOICE;
  snprintf(entry->title, sizeof(entry->title), "%s", "System CPU clock");
  snprintf(entry->choices[0], sizeof(entry->choices[0]), "%s", "NTSC (1.79 MHz)");
  snprintf(entry->choices[1], sizeof(entry->choices[1]), "%s", "PAL Gen I (2.00 MHz)");
  snprintf(entry->choices[2], sizeof(entry->choices[2]), "%s", "PAL Gen II (1.97 MHz)");
  i++;

  entry = &menu.entries[i];
  entry->key = PFU_ENTRY_KEY_FONT;
  entry->type = PFU_ENTRY_TYPE_CHOICE;
  snprintf(entry->title, sizeof(entry->title), "%s", "System font");
  snprintf(entry->choices[0], sizeof(entry->choices[0]), "%s", "Fairchild");
  snprintf(entry->choices[1], sizeof(entry->choices[1]), "%s", "Cute");
  snprintf(entry->choices[2], sizeof(entry->choices[2]), "%s", "Skinny");
  i++;

  if (i != entry_count)
  {
    pfu_error_switch(
      "Error initializing settings menu: expected %u entries, got %u.",
      entry_count, i);
    return;
  }

  emu.menu_settings = menu;
}

static void pfu_menu_init_roms_source(pfu_menu_ctx_t *menu, const char *src_path, int src)
{
  dir_t dir;
  int err = dir_findfirst(src_path, &dir); 

  while (!err)
  {
    if (dir.d_type == DT_REG)
    {
      /* Load BIOS if found */
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
      else if (strlen(dir.d_name) && dir.d_name[0] != '.' &&
               (src != PFU_SOURCE_CONTROLLER_PAK || strstr(dir.d_name, ".CHF")))
      {
        /* List all other files */
        const char *basename = strrchr(dir.d_name, '/');

        if (basename)
          basename++;
        else
          basename = dir.d_name;

        snprintf(menu->entries[menu->entry_count].title,
                 sizeof(menu->entries[menu->entry_count].title),
                 "%s", basename);
        menu->entries[menu->entry_count].title[sizeof(menu->entries[menu->entry_count].title) - 1] = '\0';
        menu->entries[menu->entry_count].type = PFU_ENTRY_TYPE_FILE;
        menu->entries[menu->entry_count].current_value = src;
        menu->entry_count++;
      }
    }
    err = dir_findnext(src_path, &dir); 
  }
}

static void pfu_menu_init_roms(void)
{
  pfu_menu_ctx_t menu;

  if (emu.menu_roms.entries)
    free(emu.menu_roms.entries);
  memset(&menu, 0, sizeof(menu));
  menu.entries = calloc(256, sizeof(pfu_menu_entry_t));

  /* Set up dummy file entry to not load a ROM */
  snprintf(menu.entries[0].title, sizeof(menu.entries[0].title), "%s", "Boot to BIOS...");
  menu.entries[0].type = PFU_ENTRY_TYPE_BACK;
  menu.entries[0].key = PFU_ENTRY_KEY_NONE;
  menu.entry_count = 1;

  if (!cpakfs_mount(JOYPAD_PORT_1, "cpak1:/"))
    pfu_menu_init_roms_source(&menu, "cpak1:/", PFU_SOURCE_CONTROLLER_PAK);
  cpakfs_unmount(JOYPAD_PORT_1);
  pfu_menu_init_roms_source(&menu, PFU_PATH_ROMFS, PFU_SOURCE_ROMFS);
  pfu_menu_init_roms_source(&menu, PFU_PATH_SD_CARD, PFU_SOURCE_SD_CARD);

  /* Fail if BIOS are not located */
  if (emu.bios_a_loaded && emu.bios_b_loaded)
  {
    snprintf(menu.menu_title, sizeof(menu.menu_title), "%s", "Press F Ultra - ROMs");
    if (joypad_get_accessory_type(JOYPAD_PORT_1) == JOYPAD_ACCESSORY_TYPE_CONTROLLER_PAK)
      snprintf(menu.menu_subtitle, sizeof(menu.menu_subtitle), "%s", "Press A to load, or Z to copy to Controller Pak.");
    else
      snprintf(menu.menu_subtitle, sizeof(menu.menu_subtitle), "%s", "Select a ROM. Press A to load.");
    emu.menu_roms = menu;
  }
  else
    pfu_error_switch(
      "Press F Ultra requires Channel F BIOS data to be stored on\n"
      "the SD Card in the \"press-f\" directory.\n\n"
      "Please include both the $03sl31253.bin$01 and "
      "$03sl31254.bin$01 BIOS images.\n\n"
      "Alternatively, this data can be compiled in statically.\n\n"
      "See https://github.com/celerizer/Press-F-Ultra for details.");
}

static uint8_t sine_color;

#define PFU_DROP 3
#define PFU_ROWS 12

static void pfu_menu_input(void)
{
  joypad_buttons_t buttons;
  pfu_menu_ctx_t *menu = emu.current_menu;
  pfu_menu_entry_t *entry;
  
  if (!menu)
    return;

  entry = &emu.current_menu->entries[emu.current_menu->cursor];

  joypad_poll();
  buttons = joypad_get_buttons_pressed(JOYPAD_PORT_1);
  if (buttons.d_up)
    menu->cursor--;
  else if (buttons.d_down)
    menu->cursor++;
  else if (buttons.d_left)
    switch (entry->type)
    {
    case PFU_ENTRY_TYPE_BOOL:
      pfu_menu_entry_bool(entry, false);
      break;
    case PFU_ENTRY_TYPE_CHOICE:
      pfu_menu_entry_choice(entry, entry->current_value - 1);
      break;
    case PFU_ENTRY_TYPE_FILE:
      menu->cursor -= PFU_ROWS;
    default:
      return;
    }
  else if (buttons.d_right)
    switch (entry->type)
    {
    case PFU_ENTRY_TYPE_BOOL:
      pfu_menu_entry_bool(entry, true);
      break;
    case PFU_ENTRY_TYPE_CHOICE:
      pfu_menu_entry_choice(entry, entry->current_value + 1);
      break;
    default:
      menu->cursor += PFU_ROWS;
    }
  else if (buttons.a)
  {
    switch (entry->type)
    {
    case PFU_ENTRY_TYPE_BACK:
      pfu_menu_entry_back();
      break;
    case PFU_ENTRY_TYPE_BOOL:
      pfu_menu_entry_bool(entry, !entry->current_value);
      break;
    case PFU_ENTRY_TYPE_CHOICE:
      pfu_menu_entry_choice(entry, entry->current_value + 1);
      break;
    case PFU_ENTRY_TYPE_FILE:
      pfu_menu_entry_file(entry);
      break;
    default:
      return;
    }
  }
  else if (buttons.b)
    pfu_emu_switch();
  else if (buttons.l)
    pfu_menu_init_roms();
  else if (buttons.z)
  {
    if (entry->type == PFU_ENTRY_TYPE_FILE &&
        entry->current_value != PFU_SOURCE_CONTROLLER_PAK &&
        joypad_get_accessory_type(JOYPAD_PORT_1) == JOYPAD_ACCESSORY_TYPE_CONTROLLER_PAK)
      pfu_controller_pak_write(entry->title, entry->current_value);
    pfu_menu_init_roms();
  }
  
  if (menu->cursor < 0)
    menu->cursor = 0;
  else if (menu->cursor >= menu->entry_count)
    menu->cursor = menu->entry_count - 1;
}

void pfu_menu_init(void)
{
  memset(&emu.menu_roms, 0, sizeof(emu.menu_roms));
  memset(&emu.menu_settings, 0, sizeof(emu.menu_settings));
  pfu_menu_init_roms();
  pfu_menu_init_settings();
}

void pfu_menu_run(void)
{
  surface_t *disp = display_get();
  const pfu_menu_ctx_t *menu = emu.current_menu;
  int i;

  if (!menu)
    return;

  /**
   * Every second, check if Controller Pak state has changed.
   * If so, reload the ROM list.
   */
  if (emu.frames % 60 == 0)
  {
    bool pak = joypad_get_accessory_type(JOYPAD_PORT_1) ==
                 JOYPAD_ACCESSORY_TYPE_CONTROLLER_PAK;

    if (pak != pfu_pak_connected)
    {
      pfu_pak_connected = pak;
      pfu_menu_init_roms();
    }
  }

  rdpq_attach_clear(disp, NULL);
  rdpq_set_mode_fill(RGBA32(0x22, 0x22, 0x22, 1));
  rdpq_fill_rectangle(0, 0, display_get_width(), display_get_height());

  rdpq_set_mode_fill(RGBA32(sine_color, sine_color, 0x00, 1));
  rdpq_fill_rectangle(48 + 4, 32 + 64 + (menu->cursor % PFU_ROWS) * 24 + 6, display_get_width() - (48 + 4), 32 + 64 + (menu->cursor % PFU_ROWS) * 24 + 24 + 6);

  rdpq_set_mode_copy(false);

  rdpq_sprite_blit(emu.icon, 48, 32, NULL);

  rdpq_text_printf(NULL, 1, 64 + 48 + 8, 32 + 24, menu->menu_title);
  rdpq_text_printf(NULL, 1, 64 + 48 + 8, 32 + 24 * 2, menu->menu_subtitle);
  for (i = (menu->cursor / PFU_ROWS) * PFU_ROWS; i < (menu->cursor / PFU_ROWS) * PFU_ROWS + PFU_ROWS && i < menu->entry_count; i++)
  {
    char print_string[sizeof(menu->entries[0].title)];
    int j = i % PFU_ROWS;
    int k;

    /* Prevent characters in files from being read as text control codes */
    snprintf(print_string, sizeof(print_string), "%s", menu->entries[i].title);
    for (k = 0; print_string[k] != '\0'; k++)
    {
      if (print_string[k] == '$' || print_string[k] == '^')
        print_string[k] = '-';
      else if (print_string[k] == '_')
        print_string[k] = ' ';
    }

    if (i == menu->cursor)
      rdpq_text_printf(NULL, 2, 48 + 8 + PFU_DROP, 32 + 64 + 24 + j * 24 + PFU_DROP, print_string);
    rdpq_text_printf(NULL, 1, 48 + 8, 32 + 64 + 24 + j * 24, print_string);
    if (menu->entries[i].type == PFU_ENTRY_TYPE_BOOL)
      rdpq_text_printf(NULL, 1, 386, 32 + 64 + 24 + j * 24, menu->entries[i].current_value ? "Enabled" : "Disabled");
    else if (menu->entries[i].type == PFU_ENTRY_TYPE_CHOICE)
      rdpq_text_printf(NULL, 1, 386, 32 + 64 + 24 + j * 24, menu->entries[i].choices[menu->entries[i].current_value]);
  }
  rdpq_detach_show();

  sine_color = (int)(sin(emu.frames * 0.1) * 127.0) + 128;
  pfu_menu_input();
}

void pfu_menu_switch_roms(void)
{
  emu.state = PFU_STATE_MENU;
  emu.current_menu = &emu.menu_roms;
}

void pfu_menu_switch_settings(void)
{
  emu.state = PFU_STATE_MENU;
  emu.current_menu = &emu.menu_settings;
}
