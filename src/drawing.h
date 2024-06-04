#ifndef EVERYTHINGAPP_DRAWING_H
#define EVERYTHINGAPP_DRAWING_H

#include "env.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef union
{
	struct
	{
		uint8_t  r;
		uint8_t  g;
		uint8_t  b;
		uint8_t  a;
	};
	uint32_t rgba;
} Color;

const static Color TRANSPARENT = {.rgba = 0X0};
const static Color RED = {.rgba = 0XFF0000FF};
const static Color GREEN = {.rgba = 0XFF00FF00};
const static Color MAGENTA = {.rgba = 0XFFFF00FF};
const static Color WHITE = {.rgba = 0XFFFFFFFF};
const static Color BLACK = {.rgba = 0XFF000000};

typedef union
{
	struct
	{
		float x;
		float y;
	};
	float xy[2];
} Point;

// Basic drawing functions
void test(Env *env);

void clear_screen(Env *env, Color color);

typedef union
{
	struct
	{
		;
		Point p1;
		Point p2;
		Point p3;
		Point p4;
	};
	Point points[4];
} BezierCurve;

void draw_curve(Env *env, BezierCurve curve, Color color);

void fill_curve(Env *env, BezierCurve curve, Color color);

typedef union
{
	struct
	{
		float x;
		float y;
		float w;
		float h;
	};
	float xyzw[4];
} Rect;

void draw_rect(Env *env, Rect rect, Color color);

// Immediate mode UI options

typedef struct
{
	Rect rect;
	Color background_color;
	float border_radius;
	float border_size;
	Color border_color;
} PanelArgs;

bool panel(Env *env, PanelArgs *args);

typedef struct
{
	Rect rect;
	Color background_color;
	Color foreground_color;
	Color hover_color;
	Color active_color;
	float border_radius;
	float border_size;
	Color border_color;
} ButtonArgs;

bool button(Env *env, ButtonArgs *args);


#endif //EVERYTHINGAPP_DRAWING_H
