#include "drawing.h"
#include "basic.h"

#include <stdbool.h>
#include <math.h>

#define EPSILON 1e-3f;

inline static float lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}

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

Color layer_color(Color color1, Color color2)
{
	float f = (float) color2.a / 255.0f;
	float r = (float) color2.r * f + (float) color1.r * (1.0f - f);
	float g = (float) color2.g * f + (float) color1.g * (1.0f - f);
	float b = (float) color2.b * f + (float) color1.b * (1.0f - f);

	return (Color)
	{
		.r=(uint8_t) r, .g=(uint8_t) g, .b=(uint8_t) b, .a=(uint8_t) color1.a
	};
}

inline static bool inside_rect(Point p, Rect r)
{
	return p.x >= r.x && p.x <= (r.x + r.w) && p.y >= r.y && p.y <= (r.y + r.h);
}

Point lerp_points(Point a, Point b, float t)
{
	Point p = {0};
	p.x = lerp(a.x, b.x, t);
	p.y = lerp(a.y, b.y, t);
	return p;
}

typedef enum
{
	OUTSIDE_BORDER,
	ON_BORDER,
	INSIDE_BORDER,
} BorderCheckResult;

#define BORDER_RADIUS_THRESHOLD 10.0f;

BorderCheckResult border_radius_check(Rect rect, int cx, int cy, float r)
{
	float r_squared = r * r;

	float c;
	bool inside_border = false;
	bool on_border = false;

	// Top Left corner
	c = powf(cx - (rect.x + r), 2.0f) + powf(cy - (rect.y + r), 2.0f);
	if (cx <= (rect.x + r) && cy <= (rect.y + r))
	{
		inside_border = c < r_squared;
		on_border = fabsf(c - r_squared) <= BORDER_RADIUS_THRESHOLD;
	}

	// Top Right corner
	c = powf(cx - (rect.x + rect.w - r), 2.0f) + powf(cy - (rect.y + r), 2.0f);
	if (cx >= (rect.x + rect.w - r) && cy <= (rect.y + r))
	{
		inside_border = inside_border || c < r_squared;
		on_border = on_border || fabsf(c - r_squared) <= BORDER_RADIUS_THRESHOLD;
	}

	// Bottom Left corner
	c = powf(cx - (rect.x + r), 2.0f) + powf(cy - (rect.y + rect.h - r), 2.0f);
	if (cx <= (rect.x + r) && cy >= (rect.y + rect.h - r))
	{
		inside_border = inside_border || c < r_squared;
		on_border = on_border || fabsf(c - r_squared) <= BORDER_RADIUS_THRESHOLD;
	}

	// Bottom Right corner
	c = powf(cx - (rect.x + rect.w - r), 2.0f) + powf(cy - (rect.y + rect.h - r), 2.0f);
	if (cx >= (rect.x + rect.w - r) && cy >= (rect.y + rect.h - r))
	{
		inside_border = inside_border || c < r_squared;
		on_border = on_border || fabsf(c - r_squared) <= BORDER_RADIUS_THRESHOLD;
	}

	inside_border = inside_border
	                || (cx > rect.x + r && cx < rect.x + rect.w - r)
	                || (cy > rect.y + r && cy < rect.y + rect.h - r);

	on_border = on_border
	            || (cx == rect.x && cy >= rect.y + r && cy <= rect.y + rect.h - r)
	            || (cx == rect.x + rect.w && cy >= rect.y + r && cy <= rect.y + rect.h - r)
	            || (cy == rect.y && cx >= rect.x + r && cx <= rect.x + rect.w - r)
	            || (cy == rect.y + rect.h && cx >= rect.x + r && cx <= rect.x + rect.w - r);

	if (on_border) return ON_BORDER;
	if (inside_border) return INSIDE_BORDER;

	return OUTSIDE_BORDER;
}

void draw_rect_ex(Env *env, Rect rect, Color color, float border_radius, Color border_color)
{
	for (size_t cy = rect.y; cy <= rect.y + rect.h; ++cy)
	{
		for (size_t cx = rect.x; cx <= rect.x + rect.w; ++cx)
		{
			BorderCheckResult result = border_radius_check(rect, cx, cy, border_radius);

			if (result != OUTSIDE_BORDER)
			{
				Color rect_color = color;
				if (result == ON_BORDER)
				{
					rect_color = border_color;
				}

				Color base = get_pixel(env, cx, cy);
				Color final = layer_color(base, rect_color);
				put_pixel(env, cx, cy, final);
			}
		}
	}
}

void draw_curve(Env *env, BezierCurve curve, Color color)
{
	float t = 0.0f;
	while (t <= 1.0f)
	{
		Point p5 = lerp_points(curve.p1, curve.p2, t);
		Point p6 = lerp_points(curve.p2, curve.p3, t);
		Point p7 = lerp_points(curve.p3, curve.p4, t);

		Point p8 = lerp_points(p5, p6, t);
		Point p9 = lerp_points(p6, p7, t);

		Point p10 = lerp_points(p8, p9, t);
		put_pixel(env, (int) p10.x, (int) p10.y, color);

		t += EPSILON;
	}
}

typedef struct
{
	size_t length;
	size_t capacity;
	Point *items;
} Points;

// Subdivide the Bezier curve into line segments
void subdivide_curve(BezierCurve curve, Points *points)
{
	float t = 0.0f;
	while (t <= 1.0f)
	{
		Point p5 = lerp_points(curve.p1, curve.p2, t);
		Point p6 = lerp_points(curve.p2, curve.p3, t);
		Point p7 = lerp_points(curve.p3, curve.p4, t);

		Point p8 = lerp_points(p5, p6, t);
		Point p9 = lerp_points(p6, p7, t);

		array_append(points, lerp_points(p8, p9, t));
		t += EPSILON;
	}
}

/* Public functions */
void clear_screen(Env *env, Color color)
{
	for (int y = 0; y < env->window_height; ++y)
	{
		for (int x = 0; x < env->window_width; ++x)
		{
			put_pixel(env, x, y, color);
		}
	}
}

void test(Env *env)
{
	BezierCurve curve = (BezierCurve)
	{
		.p1 = {.x=400.0f, .y=300.0f}, // Start
		.p2 = {.x=400.0f, .y=200.0f},
		.p3 = {.x=1000.0f, .y=200.0f},
		.p4 = {.x=1000.0f, .y=300.0f}, // End
	};
	draw_curve(env, curve, BLACK);

	curve = (BezierCurve)
	{
		.p1 = {.x=400.0f, .y=300.0f}, // Start
		.p2 = {.x=400.0f, .y=400.0f},
		.p3 = {.x=1000.0f, .y=400.0f},
		.p4 = {.x=1000.0f, .y=300.0f}, // End
	};
	draw_curve(env, curve, BLACK);
}

bool button(Env *env, ButtonArgs *args)
{
	Point mouse_pos = (Point)
	{
		.x=(float) env->mouse_x, .y=(float) env->mouse_y
	};

	bool mouse_over = inside_rect(mouse_pos, args->rect);
	bool hover = !env->mouse_left_down && mouse_over;
	bool clicked = env->mouse_left_down && mouse_over;

	Color bg_color;
	if (hover)
	{
		bg_color = args->hover_color;
	}
	else if (clicked)
	{
		bg_color = args->active_color;
	}
	else
	{
		bg_color = args->background_color;
	}

	draw_rect_ex(env, args->rect, bg_color, args->border_radius, args->border_color);

	return clicked;
}

bool panel(Env *env, PanelArgs *args)
{
	Point mouse_pos = (Point)
	{
		.x = (float) env->mouse_x, .y = (float) env->mouse_y
	};

	bool mouse_over = inside_rect(mouse_pos, args->rect);
	bool clicked = env->mouse_left_down && mouse_over;

	draw_rect_ex(env, args->rect, args->background_color, args->border_radius, args->border_color);

	return clicked;
}
