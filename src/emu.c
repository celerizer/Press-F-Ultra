#include <libdragon.h>

#include "libpressf/src/emu.h"
#include "libpressf/src/input.h"
#include "libpressf/src/screen.h"
#include "libpressf/src/hw/beeper.h"
#include "libpressf/src/hw/vram.h"

#include "main.h"
#include "emu.h"

static void pfu_video_render_1_1(void)
{
  surface_t *disp = display_get();

  rdpq_attach_clear(disp, NULL);
  rdpq_set_mode_standard();
  rdpq_tex_blit(&emu.video_frame, 14, 66, &(rdpq_blitparms_t){ .scale_x = 6.0f, .scale_y = 6.0f});
  rdpq_detach_show();
}

static void pfu_video_render_4_3(void)
{
  surface_t *disp = display_get();

  rdpq_attach_clear(disp, NULL);
  rdpq_set_mode_standard();
  rdpq_tex_blit(&emu.video_frame, 0, 0, &(rdpq_blitparms_t){ .scale_x = 640.0f / SCREEN_WIDTH, .scale_y = 480.0f / SCREEN_HEIGHT});
  rdpq_detach_show();
}

static void pfu_emu_input(void)
{
#if 0 /** @todo PRESS_F_ULTRA_PREVIEW */
  joypad_buttons_t keys;

  joypad_poll();
  keys = joypad_get_buttons_pressed(JOYPAD_PORT_1);
#else
  struct controller_data keys;

  controller_scan();
  keys = get_keys_pressed();

  /* Handle console input */
  set_input_button(0, INPUT_TIME, keys.c[0].A);
  set_input_button(0, INPUT_MODE, keys.c[0].B);
  set_input_button(0, INPUT_HOLD, keys.c[0].Z);
  set_input_button(0, INPUT_START, keys.c[0].start);

  /* Handle player 1 input */
  set_input_button(4, INPUT_RIGHT, keys.c[0].right);
  set_input_button(4, INPUT_LEFT, keys.c[0].left);
  set_input_button(4, INPUT_BACK, keys.c[0].down);
  set_input_button(4, INPUT_FORWARD, keys.c[0].up);
  set_input_button(4, INPUT_ROTATE_CCW, keys.c[0].C_left);
  set_input_button(4, INPUT_ROTATE_CW, keys.c[0].C_right);
  set_input_button(4, INPUT_PULL, keys.c[0].C_up);
  set_input_button(4, INPUT_PUSH, keys.c[0].C_down);

  /* Handle player 2 input */
  set_input_button(1, INPUT_RIGHT, keys.c[1].right);
  set_input_button(1, INPUT_LEFT, keys.c[1].left);
  set_input_button(1, INPUT_BACK, keys.c[1].down);
  set_input_button(1, INPUT_FORWARD, keys.c[1].up);
  set_input_button(1, INPUT_ROTATE_CCW, keys.c[1].C_left);
  set_input_button(1, INPUT_ROTATE_CW, keys.c[1].C_right);
  set_input_button(1, INPUT_PULL, keys.c[1].C_up);
  set_input_button(1, INPUT_PUSH, keys.c[1].C_down);

  /* Handle hotkeys */
  if (keys.c[0].L)
    emu.video_scaling = PFU_SCALING_1_1;
  else if (keys.c[0].R)
    emu.video_scaling = PFU_SCALING_4_3;
#endif
}

void pfu_emu_run(void)
{
  /* Input */
  pfu_emu_input();

  /* Emulation */
  pressf_run(&emu.system);

  /* Video */
  draw_frame_rgb5551(((vram_t*)emu.system.f8devices[3].device)->data, emu.video_buffer);

  /* Audio */
  audio_push(((f8_beeper_t*)emu.system.f8devices[7].device)->samples, PF_SOUND_SAMPLES, false);

  /* Blit the frame */
  if (emu.video_scaling == PFU_SCALING_1_1)
    pfu_video_render_1_1();
  else
    pfu_video_render_4_3();
}
