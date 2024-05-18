#include "drawing.h"
#include "basic.h"

#include <stdbool.h>
#include <math.h>

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

void clear_screen(Env *env, uint32_t color)
{
    for (int y = 0; y < env->window_height; ++y) {
        for (int x = 0; x < env->window_width; ++x) {
            put_pixel(env, x, y, color);
        }
    }
}

void draw_rect(Env *env, Rect rect, Color color, float r)
{
    float r_squared = r*r;
    float c;
    bool cutoff;

    for (int cy = rect.y; cy < rect.y+rect.h; ++cy) {
        for (int cx = rect.x; cx < rect.x+rect.w; ++cx) {
            // Top Left
            c = powf(cx - (rect.x+r), 2.0f) + powf(cy - (rect.y+r), 2.0f);
            cutoff = (cx < (rect.x+r) && cy < (rect.y+r) && c > r_squared);

            // Top Right
            c = powf(cx - (rect.x+rect.w - r), 2.0f) + powf(cy - (rect.y+r), 2.0f);
            cutoff = cutoff || (cx > (rect.x+rect.w - r) && cy < (rect.y+r) && c > r_squared);

            // Bottom Left
            c = powf(cx - (rect.x+r), 2.0f) + powf(cy - (rect.y+rect.h - r), 2.0f);
            cutoff = cutoff || (cx < (rect.x+r) && cy > (rect.y+rect.h - r) && c > r_squared);

            // Bottom Right
            c = powf(cx - (rect.x+rect.w - r), 2.0f) + powf(cy - (rect.y+rect.h - r), 2.0f);
            cutoff = cutoff || (cx > (rect.x+rect.w - r) && cy > (rect.y+rect.h - r) && c > r_squared);

            if (!cutoff) {
               put_pixel(env, cx, cy, color);
            }
        }
    }
}

bool button(Env *env, ButtonArgs *args) {
    draw_rect(env, args->rect, args->background_color, args->border_radius);

    return false;
}

bool panel(Env *env, PanelArgs *args) {
    draw_rect(env, args->rect, args->background_color, args->border_radius);

    return false;
}