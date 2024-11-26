#ifndef PRESS_F_ULTRA_MENU_H
#define PRESS_F_ULTRA_MENU_H

#define PFU_MENU_MAX_ENTRIES 256
#define PFU_MENU_MAX_CHOICES 8

typedef enum
{
  PFU_ENTRY_KEY_NONE = 0,

  PFU_ENTRY_KEY_PIXEL_PERFECT,
  PFU_ENTRY_KEY_SYSTEM_MODEL,
  PFU_ENTRY_KEY_FONT,

  PFU_ENTRY_KEY_SIZE
} pfu_entry_key;

typedef enum
{
  PFU_ENTRY_TYPE_NONE = 0,

  PFU_ENTRY_TYPE_BACK,
  PFU_ENTRY_TYPE_FILE,
  PFU_ENTRY_TYPE_BOOL,
  PFU_ENTRY_TYPE_CHOICE,

  PFU_ENTRY_TYPE_SIZE
} pfu_entry_type;

typedef struct
{
  char title[128];
  char choices[PFU_MENU_MAX_CHOICES][32];
  pfu_entry_key key;
  pfu_entry_type type;
  signed current_value;
} pfu_menu_entry_t;

typedef struct
{
  pfu_menu_entry_t *entries;
  char menu_title[256];
  char menu_subtitle[256];
  int entry_count;
  int cursor;
  int offset;
} pfu_menu_ctx_t;

void pfu_menu_run(void);

void pfu_menu_init(void);

void pfu_menu_switch_roms(void);

void pfu_menu_switch_settings(void);

#endif
