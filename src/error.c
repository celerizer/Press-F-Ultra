#include <libdragon.h>

#include "emu.h"
#include "error.h"
#include "main.h"

void pfu_error_switch(const char *error, ...)
{
  char message[1024];

  va_list args;
  va_start(args, error);
  vsnprintf(message, sizeof(message), error, args);
  va_end(args);

  pfu_message_switch(PFU_STATE_INVALID, message);
}

void pfu_message_switch(unsigned state, const char *message, ...)
{
  surface_t *disp = display_get();
  char text[1024];

  va_list args;
  va_start(args, message);
  vsnprintf(text, sizeof(text), message, args);
  va_end(args);

  rdpq_attach_clear(disp, NULL);
  rdpq_set_mode_fill(RGBA32(0x22, 0x22, 0x22, 1));
  rdpq_fill_rectangle(0, 0, display_get_width(), display_get_height());

  rdpq_set_mode_copy(false);
  rdpq_sprite_blit(emu.icon, 48, 32, NULL);
  rdpq_text_printf(
    &(rdpq_textparms_t){
      .width = 640 - 64*2,
      .height = 480 - 64*2,
		  .align = ALIGN_CENTER
	  }, 1, 64, 128, text);

  rdpq_detach_show();

  while (1)
  {
    joypad_buttons_t buttons;

    joypad_poll();
    buttons = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    if (buttons.a || buttons.b)
    {
      switch (state)
      {
      case PFU_STATE_EMU:
        pfu_emu_switch();
        return;
      case PFU_STATE_MENU:
        pfu_menu_switch_roms();
        return;
      default:
        exit(0);
      }
    }
  }
}
