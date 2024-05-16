#include "env.h"

#include <stdlib.h>

#define BLOCK_SIZE 100

#define RED 0XFF0000FF
#define MAGENTA 0XFFFF00FF
#define WHITE 0XFFFFFFFF

typedef struct {
    uint32_t color;
} AppState;

AppState *state = NULL;

void app_init() {
    state = malloc(sizeof(AppState));
    state->color = MAGENTA;
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

// For hot reloading
void* app_pre_reload(void) {
    return state;
}

void app_post_reload(void* old_state) {
    state = old_state;
}
