#ifndef ENV_H
#define ENV_H

#include <stdint.h>

typedef struct {
  double delta_time;
  int window_width;
  int window_height;
  uint8_t *buffer;
} Env;

#endif
