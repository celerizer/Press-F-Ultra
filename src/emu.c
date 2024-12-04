#include <libdragon.h>

#include "libpressf/src/emu.h"
#include "libpressf/src/input.h"
#include "libpressf/src/screen.h"
#include "libpressf/src/hw/beeper.h"
#include "libpressf/src/hw/vram.h"

#include "main.h"
#include "emu.h"

#define PFU_EMU_X_MARGIN_240P 24
#define PFU_EMU_X_MARGIN_480P 48
#define PFU_EMU_Y_MARGIN_240P 16
#define PFU_EMU_Y_MARGIN_480P 32

static const rdpq_blitparms_t pfu_1_1_480p_params = {
  .scale_x = 6.0f,
  .scale_y = 6.0f };
static void pfu_video_render_1_1(void)
{
  surface_t *disp = display_get();

  rdpq_attach_clear(disp, NULL);
  rdpq_set_mode_standard();
  rdpq_tex_blit(&emu.video_frame,
                14,
                66,
                &pfu_1_1_480p_params);
  rdpq_detach_show();
}

static const rdpq_blitparms_t pfu_4_3_480p_params = {
  .scale_x = (640.0f - PFU_EMU_X_MARGIN_480P * 2) / SCREEN_WIDTH,
  .scale_y = (480.0f - PFU_EMU_Y_MARGIN_480P * 2) / SCREEN_HEIGHT };
static void pfu_video_render_4_3(void)
{
  surface_t *disp = display_get();

  rdpq_attach_clear(disp, NULL);
  rdpq_set_mode_standard();
  rdpq_tex_blit(&emu.video_frame,
                PFU_EMU_X_MARGIN_480P,
                PFU_EMU_Y_MARGIN_480P,
                &pfu_4_3_480p_params);
  rdpq_detach_show();
}

static void pfu_emu_input(void)
{
  joypad_buttons_t buttons;

  joypad_poll();
  buttons = joypad_get_buttons(JOYPAD_PORT_1);

  /* Handle hotkeys */
  if (buttons.l)
  {
    pfu_menu_switch_roms();
    return;
  }
  else if (buttons.r)
  {
    pfu_menu_switch_settings();
    return;
  }

  /* Handle console input */
  set_input_button(0, INPUT_TIME, buttons.a);
  set_input_button(0, INPUT_MODE, buttons.b);
  set_input_button(0, INPUT_HOLD, buttons.z);
  set_input_button(0, INPUT_START, buttons.start);

  /* Handle player 1 input */
  set_input_button(4, INPUT_RIGHT, buttons.d_right);
  set_input_button(4, INPUT_LEFT, buttons.d_left);
  set_input_button(4, INPUT_BACK, buttons.d_down);
  set_input_button(4, INPUT_FORWARD, buttons.d_up);
  set_input_button(4, INPUT_ROTATE_CCW, buttons.c_left);
  set_input_button(4, INPUT_ROTATE_CW, buttons.c_right);
  set_input_button(4, INPUT_PULL, buttons.c_up);
  set_input_button(4, INPUT_PUSH, buttons.c_down);

  buttons = joypad_get_buttons(JOYPAD_PORT_2);

  /* Handle player 2 input */
  set_input_button(1, INPUT_RIGHT, buttons.d_right);
  set_input_button(1, INPUT_LEFT, buttons.d_left);
  set_input_button(1, INPUT_BACK, buttons.d_down);
  set_input_button(1, INPUT_FORWARD, buttons.d_up);
  set_input_button(1, INPUT_ROTATE_CCW, buttons.c_left);
  set_input_button(1, INPUT_ROTATE_CW, buttons.c_right);
  set_input_button(1, INPUT_PULL, buttons.c_up);
  set_input_button(1, INPUT_PUSH, buttons.c_down);
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
  audio_push(((f8_beeper_t*)emu.system.f8devices[7].device)->samples, PF_SOUND_SAMPLES, true);

  /* Blit the frame */
  if (emu.video_scaling == PFU_SCALING_1_1)
    pfu_video_render_1_1();
  else
    pfu_video_render_4_3();
}

void pfu_emu_switch(void)
{
  emu.state = PFU_STATE_EMU;
}
