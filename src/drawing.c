#include "drawing.h"

#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

inline static Color get_pixel(Env *env, int x, int y)
{
    if (x < 0 || x >= env->window_width) return TRANSPARENT;
    if (y < 0 || y >= env->window_height) return TRANSPARENT;

    return ((Color *)env->buffer)[y * env->window_width + x];
}

inline static void put_pixel(Env *env, int x, int y, Color color)
{
    if (x < 0 || x >= env->window_width) return;
    if (y < 0 || y >= env->window_height) return;

    ((Color *)env->buffer)[y * env->window_width + x] = color;
}

Color layer_color(Color color1, Color color2) {
    float f = (float)color2.a/255.0f;
    float r = color2.r * f + color1.r * (1.0f - f);
    float g = color2.g * f + color1.g * (1.0f - f);
    float b = color2.b * f + color1.b * (1.0f - f);

    return (Color){.r=(uint8_t)r, .g=(uint8_t)g, .b=(uint8_t)b, .a=(uint8_t)color1.a};
}

inline bool is_inside_rect(int x, int y, Rect r) {
    return x >= r.x && x <= (r.x + r.w) && y >= r.y && y <= (r.y + r.h);
}

typedef enum {
    OUTSIDE_BORDER,
    ON_BORDER,
    INSIDE_BORDER,
} BorderCheckResult;

#define EPS 10.0f;

BorderCheckResult border_radius_check(Rect rect, int cx, int cy, float r) {
    float r_squared = r*r;

    float c;
    bool inside_border = false;
    bool on_border = false;

    // Top Left corner
    c = powf(cx - (rect.x+r), 2.0f) + powf(cy - (rect.y+r), 2.0f);
    if (cx <= (rect.x+r) && cy <= (rect.y+r)) {
        inside_border = c < r_squared;
        on_border = fabsf(c - r_squared) <= EPS;
    }

    // Top Right corner
    c = powf(cx - (rect.x+rect.w - r), 2.0f) + powf(cy - (rect.y+r), 2.0f);
    if (cx >= (rect.x+rect.w - r) && cy <= (rect.y+r)) {
        inside_border = inside_border || c < r_squared;
        on_border = on_border ||  fabsf(c - r_squared) <= EPS;
    }

    // Bottom Left corner
    c = powf(cx - (rect.x+r), 2.0f) + powf(cy - (rect.y+rect.h - r), 2.0f);
    if (cx <= (rect.x+r) && cy >= (rect.y+rect.h - r)) {
        inside_border = inside_border || c < r_squared;
        on_border = on_border ||  fabsf(c - r_squared) <= EPS;
    }

    // Bottom Right corner
    c = powf(cx - (rect.x+rect.w - r), 2.0f) + powf(cy - (rect.y+rect.h - r), 2.0f);
    if (cx >= (rect.x+rect.w - r) && cy >= (rect.y+rect.h - r)) {
        inside_border = inside_border || c < r_squared;
        on_border = on_border ||  fabsf(c - r_squared) <= EPS;
    }

    inside_border = inside_border
            || (cx > rect.x+r && cx < rect.x+rect.w-r)
            ||  (cy > rect.y+r && cy < rect.y+rect.h-r) ;

    on_border = on_border
                || (cx == rect.x && cy >= rect.y+r && cy <= rect.y+rect.h-r)
                || (cx == rect.x+rect.w && cy >= rect.y+r && cy <= rect.y+rect.h-r)
                || (cy == rect.y && cx >= rect.x+r && cx <= rect.x+rect.w-r)
                || (cy == rect.y+rect.h && cx >= rect.x+r && cx <= rect.x+rect.w-r);

    if (on_border) return ON_BORDER;
    if (inside_border) return INSIDE_BORDER;

    return OUTSIDE_BORDER;
}

void draw_rect(Env *env, Rect rect, Color color, float border_radius, Color border_color)
{
    for (int cy = rect.y; cy <= rect.y+rect.h; ++cy) {
        for (int cx = rect.x; cx <= rect.x+rect.w; ++cx) {
            BorderCheckResult result = border_radius_check(rect, cx, cy, border_radius);

            if (result != OUTSIDE_BORDER) {
                Color rect_color = color;
                if (result == ON_BORDER) {
                    rect_color = border_color;
                }

                Color base = get_pixel(env, cx, cy);
                Color final = layer_color(base, rect_color);
                put_pixel(env, cx, cy, final);
            }
        }
    }
}

/* Public functions */
void clear_screen(Env *env, Color color)
{
    for (int y = 0; y < env->window_height; ++y) {
        for (int x = 0; x < env->window_width; ++x) {
            put_pixel(env, x, y, color);
        }
    }
}

bool button(Env *env, ButtonArgs *args) {
    bool mouse_over = is_inside_rect(env->mouse_x, env->mouse_y, args->rect);
    bool hover = !env->mouse_left_down && mouse_over;
    bool clicked = env->mouse_left_down && mouse_over;

    Color bg_color;

    if (hover) {
        bg_color = args->hover_color;
    }  else if (clicked) {
        bg_color = args->active_color;
    } else {
        bg_color = args->background_color;
    }

    draw_rect(env, args->rect, bg_color, args->border_radius, args->border_color);

    return clicked;
}

bool panel(Env *env, PanelArgs *args) {
    bool mouse_over = is_inside_rect(env->mouse_x, env->mouse_y, args->rect);
    bool clicked = env->mouse_left_down && mouse_over;

    draw_rect(env, args->rect, args->background_color, args->border_radius, args->border_color);

    return clicked;
}
