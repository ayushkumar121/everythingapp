#ifndef EVERYTHINGAPP_DRAWING_H
#define EVERYTHINGAPP_DRAWING_H

#include "env.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef union __attribute__((packed))
{
	struct __attribute__((packed))
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	};
	uint32_t rgba;
} Color;

const static Color TRANSPARENT = {.rgba = 0X0};
const static Color RED = {.rgba = 0XFF0000FF};
const static Color GREEN = {.rgba = 0XFF00FF00};
const static Color MAGENTA = {.rgba = 0XFFFF00FF};
const static Color WHITE = {.rgba = 0XFFFFFFFF};
const static Color BLACK = {.rgba = 0XFF000000};
const static Color GRAY = {.rgba = 0XFF666666};

typedef union __attribute__((packed))
{
	struct __attribute__((packed))
	{
		float x;
		float y;
	};
	float xy[2];
} Point;

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

typedef struct
{
	int width;
	int height;
	Color *pixels;
} Image;

Image image_from_env(Env* env);

void blur_image(Image image);
void fade_image(Image image, float opacity);
Image scale_image(Image image, float sx, float sy);
Image duplicate_image(Image image);

void load_image(Image *image, const char *filename);

typedef struct
{
	Image *image;
	Rect rect;
	Rect *crop;
} ImageArgs;

void draw_image(Image background, ImageArgs *args); // Draws an image on another image
void free_image(Image* image);

void clear_image(Image image, Color color);

typedef union __attribute__((packed))
{
	struct __attribute__((packed))
	{
		Point p1;
		Point p2;
		Point p3;
		Point p4;
	};
	Point points[4];
} BezierCurve;

void draw_curve(Image image, BezierCurve curve, Color color);

void draw_rect(Image image, Rect rect, Color color);
void draw_rounded_rect(Image image, Rect rect, Color color, float border_radius, Color border_color);

typedef enum
{
	FONT_TTF,
	FONT_BDF,
} FontFormat;

typedef struct
{
	int width;
	int height;
	int x_offset;
	int y_offset;
	int advance;
	uint64_t *bitmap;
} FontBDFGlyph;

typedef struct
{
	int size;
	int x_dpi;
	int y_dpi;
	FontBDFGlyph glyphs[128];
} FontBDF;

typedef struct
{
	FontFormat format;
	void *data;
} Font;

void load_font(Font *font, const char *filename);

typedef struct
{
	Font *font;
	const char *text;
	int size;
	Point position;
	Color color;
} TextArgs;

Point measure_text(Font *font, const char* text, int size);
void draw_text(Image image, TextArgs *args);
void free_font(Font *font);

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
	const char *text;
	Font *font;
	int font_size;
} ButtonArgs;

bool button(Env *env, ButtonArgs *args);

#endif // EVERYTHINGAPP_DRAWING_H
