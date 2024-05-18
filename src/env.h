#ifndef ENV_H
#define ENV_H

#include <stdint.h>
#include <stdbool.h>

/* Platform Independent Keycode*/
typedef enum {
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,
    KEY_ENTER,
    KEY_CTRL,
    KEY_LEFT_SHIFT,
    KEY_RIGHT_SHIFT,
    KEY_META,
} AppKeyCode;

typedef struct {
  double delta_time;
  int window_width;
  int window_height;
  uint8_t *buffer;

  AppKeyCode key_code;
  bool key_down;
  bool mouse_left_down;
  bool mouse_right_down;
  bool mouse_moved;
} Env;

#endif
