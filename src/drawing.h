#ifndef EVERYTHINGAPP_DRAWING_H
#define EVERYTHINGAPP_DRAWING_H

#include "env.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define TRANSPARENT 0X0
#define RED 0XFF0000FF
#define MAGENTA 0XFFFF00FF
#define WHITE 0XFFFFFFFF
#define BLACK 0XFF000000

typedef struct {
    int x;
    int y;
    int w;
    int h;
} Rect;

typedef uint32_t Color;

// Basic drawing functions
void clear_screen(Env *env, Color color);

// UI elements
typedef struct {
    Rect rect;
    Color background_color;
    Color foreground_color;
    float border_radius;
    float box_shadow;
} ButtonArgs;

bool button(Env *env, ButtonArgs *args);

typedef struct {
    Rect rect;
    Color background_color;
    float border_radius;
} PanelArgs;

bool panel(Env *env, PanelArgs *args);

#endif //EVERYTHINGAPP_DRAWING_H
