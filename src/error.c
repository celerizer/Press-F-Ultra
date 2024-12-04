#include <libdragon.h>

#include "main.h"

void pfu_error_switch(const char *error, ...)
{
  surface_t *disp = display_get();
  char message[1024];

  va_list args;
  va_start(args, error);
  vsnprintf(message, sizeof(message), error, args);
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
	  }, 1, 64, 128, message);

  rdpq_detach_show();

  exit(1);
}
