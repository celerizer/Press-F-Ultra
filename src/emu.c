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

static joypad_inputs_t pfu_analog_to_digital(joypad_inputs_t inputs, joypad_style_t style)
{
  if (style == JOYPAD_STYLE_N64)
  {
    static const int stick_threshold = JOYPAD_RANGE_N64_STICK_MAX / 2;

    inputs.btn.d_up = inputs.stick_y > +stick_threshold;
    inputs.btn.d_down = inputs.stick_y < -stick_threshold;
    inputs.btn.d_left = inputs.stick_x < -stick_threshold;
    inputs.btn.d_right = inputs.stick_x > +stick_threshold;
  }
  else if (style == JOYPAD_STYLE_GCN)
  {
    static const int stick_threshold = JOYPAD_RANGE_GCN_STICK_MAX / 2;

    inputs.btn.d_up = inputs.stick_y > +stick_threshold;
    inputs.btn.d_down = inputs.stick_y < -stick_threshold;
    inputs.btn.d_left = inputs.stick_x < -stick_threshold;
    inputs.btn.d_right = inputs.stick_x > +stick_threshold;
  }

  return inputs;
}

static void pfu_emu_input(void)
{
  joypad_inputs_t inputs;
  joypad_style_t style;
  unsigned port;

  joypad_poll();
  inputs = joypad_get_inputs(JOYPAD_PORT_1);
  style = joypad_get_style(JOYPAD_PORT_1);
  inputs = pfu_analog_to_digital(inputs, style);

  /* Handle hotkeys */
  if (inputs.btn.l)
  {
    pfu_menu_switch_roms();
    return;
  }
  else if (inputs.btn.r)
  {
    pfu_menu_switch_settings();
    return;
  }

  /* Handle console input */
  set_input_button(0, INPUT_TIME, inputs.btn.a);
  set_input_button(0, INPUT_MODE, inputs.btn.b);
  set_input_button(0, INPUT_HOLD, inputs.btn.z);
  set_input_button(0, INPUT_START, inputs.btn.start);

  /* Handle player 1 input */
  port = emu.swap_controllers ? 1 : 4;
  set_input_button(port, INPUT_RIGHT, inputs.btn.d_right);
  set_input_button(port, INPUT_LEFT, inputs.btn.d_left);
  set_input_button(port, INPUT_BACK, inputs.btn.d_down);
  set_input_button(port, INPUT_FORWARD, inputs.btn.d_up);
  set_input_button(port, INPUT_ROTATE_CCW, inputs.btn.c_left);
  set_input_button(port, INPUT_ROTATE_CW, inputs.btn.c_right);
  set_input_button(port, INPUT_PULL, inputs.btn.c_up);
  set_input_button(port, INPUT_PUSH, inputs.btn.c_down);

  inputs = joypad_get_inputs(JOYPAD_PORT_2);
  style = joypad_get_style(JOYPAD_PORT_2);
  inputs = pfu_analog_to_digital(inputs, style);

  /* Handle player 2 input */
  port = emu.swap_controllers ? 4 : 1;
  set_input_button(port, INPUT_RIGHT, inputs.btn.d_right);
  set_input_button(port, INPUT_LEFT, inputs.btn.d_left);
  set_input_button(port, INPUT_BACK, inputs.btn.d_down);
  set_input_button(port, INPUT_FORWARD, inputs.btn.d_up);
  set_input_button(port, INPUT_ROTATE_CCW, inputs.btn.c_left);
  set_input_button(port, INPUT_ROTATE_CW, inputs.btn.c_right);
  set_input_button(port, INPUT_PULL, inputs.btn.c_up);
  set_input_button(port, INPUT_PUSH, inputs.btn.c_down);
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
