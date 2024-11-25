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

typedef struct
{
  bool p1_up;
  bool p1_down;
  bool p1_left;
  bool p1_right;
  bool p1_a;
  bool p1_b;
  bool p1_z;
  bool p1_start;
  bool p1_c_up;
  bool p1_c_down;
  bool p1_c_left;
  bool p1_c_right;
  bool p1_l;
  bool p1_r;

  bool p2_up;
  bool p2_down;
  bool p2_left;
  bool p2_right;
  bool p2_c_up;
  bool p2_c_down;
  bool p2_c_left;
  bool p2_c_right;
} pfu_buttons_t;

static void pfu_emu_input(void)
{
  pfu_buttons_t buttons;
#if PRESS_F_ULTRA_PREVIEW
  joypad_buttons_t keys_1, keys_2;

  joypad_poll();
  keys_1 = joypad_get_buttons_pressed(JOYPAD_PORT_1);
  keys_2 = joypad_get_buttons_pressed(JOYPAD_PORT_2);

  buttons.p1_up = keys_1.d_up;
  buttons.p1_down = keys_1.d_down;
  buttons.p1_left = keys_1.d_left;
  buttons.p1_right = keys_1.d_right;
  buttons.p1_c_up = keys_1.c_up;
  buttons.p1_c_down = keys_1.c_down;
  buttons.p1_c_left = keys_1.c_left;
  buttons.p1_c_right = keys_1.c_right;
  buttons.p1_a = keys_1.a;
  buttons.p1_b = keys_1.b;
  buttons.p1_z = keys_1.z;
  buttons.p1_start = keys_1.start;
  buttons.p1_l = keys_1.l;
  buttons.p1_r = keys_1.r;

  buttons.p2_up = keys_2.d_up;
  buttons.p2_down = keys_2.d_down;
  buttons.p2_left = keys_2.d_left;
  buttons.p2_right = keys_2.d_right;
  buttons.p2_c_up = keys_2.c_up;
  buttons.p2_c_down = keys_2.c_down;
  buttons.p2_c_left = keys_2.c_left;
  buttons.p2_c_right = keys_2.c_right;
#else
  struct controller_data keys;

  controller_scan();
  keys = get_keys_pressed();

  buttons.p1_up = keys.c[0].up;
  buttons.p1_down = keys.c[0].down;
  buttons.p1_left = keys.c[0].left;
  buttons.p1_right = keys.c[0].right;
  buttons.p1_c_up = keys.c[0].C_up;
  buttons.p1_c_down = keys.c[0].C_down;
  buttons.p1_c_left = keys.c[0].C_left;
  buttons.p1_c_right = keys.c[0].C_right;
  buttons.p1_a = keys.c[0].A;
  buttons.p1_b = keys.c[0].B;
  buttons.p1_z = keys.c[0].Z;
  buttons.p1_start = keys.c[0].start;
  buttons.p1_l = keys.c[0].L;
  buttons.p1_r = keys.c[0].R;

  buttons.p2_up = keys.c[1].up;
  buttons.p2_down = keys.c[1].down;
  buttons.p2_left = keys.c[1].left;
  buttons.p2_right = keys.c[1].right;
  buttons.p2_c_up = keys.c[1].C_up;
  buttons.p2_c_down = keys.c[1].C_down;
  buttons.p2_c_left = keys.c[1].C_left;
  buttons.p2_c_right = keys.c[1].C_right;
#endif

  /* Handle console input */
  set_input_button(0, INPUT_TIME, buttons.p1_a);
  set_input_button(0, INPUT_MODE, buttons.p1_b);
  set_input_button(0, INPUT_HOLD, buttons.p1_z);
  set_input_button(0, INPUT_START, buttons.p1_start);

  /* Handle player 1 input */
  set_input_button(4, INPUT_RIGHT, buttons.p1_right);
  set_input_button(4, INPUT_LEFT, buttons.p1_left);
  set_input_button(4, INPUT_BACK, buttons.p1_down);
  set_input_button(4, INPUT_FORWARD, buttons.p1_up);
  set_input_button(4, INPUT_ROTATE_CCW, buttons.p1_c_left);
  set_input_button(4, INPUT_ROTATE_CW, buttons.p1_c_right);
  set_input_button(4, INPUT_PULL, buttons.p1_c_up);
  set_input_button(4, INPUT_PUSH, buttons.p1_c_down);

  /* Handle player 2 input */
  set_input_button(1, INPUT_RIGHT, buttons.p2_right);
  set_input_button(1, INPUT_LEFT, buttons.p2_left);
  set_input_button(1, INPUT_BACK, buttons.p2_down);
  set_input_button(1, INPUT_FORWARD, buttons.p2_up);
  set_input_button(1, INPUT_ROTATE_CCW, buttons.p2_c_left);
  set_input_button(1, INPUT_ROTATE_CW, buttons.p2_c_right);
  set_input_button(1, INPUT_PULL, buttons.p2_c_up);
  set_input_button(1, INPUT_PUSH, buttons.p2_c_down);

  /* Handle hotkeys */
  if (buttons.p1_l)
    emu.video_scaling = PFU_SCALING_1_1;
  else if (buttons.p1_r)
    emu.video_scaling = PFU_SCALING_4_3;
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
