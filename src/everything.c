#include "env.h"
#include "drawing.h"

#include <stdlib.h>
#include <strings.h>
#include <stdio.h>

typedef struct {
    uint32_t color;
} AppState;

AppState *state = NULL;

void app_init() {
    state = malloc(sizeof(AppState));
    state->color = MAGENTA;
}


void app_update(Env *env) {
    // Input
    {
        if (env->key_down) {
            clear_screen(env, RED);
            return;
        }
    }


    // Rendering
    clear_screen(env, BLACK);

    // Drawing side panel
    {
        PanelArgs args = {
                .rect = {
                        .x = 0,
                        .y = 0,
                        .w = env->window_width/3,
                        .h = env->window_height,
                },
                .background_color = WHITE,
                .border_radius = 0.0f,
        };
        panel(env, &args);
        panel(env, &args);
    }

    // Drawing files
    {
        int box_height = 100;
        int gap = 10;
        int margin_top = 50;
        int margin_sides = 20;

        for (int i = 0; i < 3; ++i) {
            ButtonArgs args = {
                    .rect = {
                            .x = 10,
                            .y = margin_top+(box_height+gap)*i,
                            .w = env->window_width/3-margin_sides,
                            .h = box_height,
                    },
                    .background_color = 0xFF222222,
                    .foreground_color = WHITE,
                    .box_shadow = 8.0f,
                    .border_radius = 16.0f,
            };
            button(env, &args);
        }
    }

    // Drawing main panel
    {
    }
}

// For hot reloading
void* app_pre_reload(void) {
    return state;
}

void app_post_reload(void* old_state) {
    state = malloc(sizeof(AppState));
    memcpy(state, old_state, sizeof(*old_state));
    free(old_state);
}