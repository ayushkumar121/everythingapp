#include "env.h"

#define BLOCK_SIZE 100
#define MAGENTA 0XFFFF00FF // 0xAARRGGBB
#define WHITE 0XFFFFFFFF

void app_init(void) {
  
}

void app_update(Env *env) {
    for (int y = 0; y < env->window_height; y++) {
      for (int x = 0; x < env->window_width; x++) {
        int square_x = x/BLOCK_SIZE;
        int square_y = y/BLOCK_SIZE;

        uint32_t color;
          if ((square_x+square_y)%2 == 0) {
          color = MAGENTA;
        } else {
          color = WHITE;
        }
      
        ((uint32_t *)env->buffer)[y * env->window_width + x] = color;
      }
    }
}
