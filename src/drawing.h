#pragma once

#include "env.h"
#include <stdint.h>

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
const static Color BLUE = {.rgba = 0XFF0000FF};
const static Color CYAN = {.rgba = 0XFF00FFFF};
const static Color ORANGE = {.rgba = 0xFFA500FF};
const static Color PURPLE = {.rgba = 0xFF00FF00};
const static Color YELLOW = {.rgba = 0xFFFF00FF};

typedef union
{
	struct __attribute__((packed))
	{
		float x;
		float y;
	};
	float xy[2];
} Vec2;

typedef union
{
	struct __attribute__((packed))
	{
		float x;
		float y;
		float w;
		float h;
	};
	struct __attribute__((packed))
	{
		float left;
		float top;
		float right;
		float bottom;
	};
	float xyzw[4];
} Vec4;

Vec4 v4_add_v4(Vec4 a, Vec4 b);
Vec4 v4_add_v2(Vec4 a, Vec2 b);
Vec2 v2_add_v2(Vec2 a, Vec2 b);

typedef struct
{
	int width;
	int height;
	Color *pixels;
} Image;

float lerp(float a, float b, float t);
float clamp(float x, float min, float max);
bool inside_rect(Vec2 p, Vec4 r);

Image image_from_env(Env* env);
Env env_from_image(Image image);

void blur_image(Image image);
void fade_image(Image image, float opacity);
Image scale_image(Image image, float sx, float sy);
Image duplicate_image(Image image);

Image new_image(int width, int height);
void load_image(Image *image, const char *filename);

void draw_image(Image background, Image image, Vec4 rect, Vec4 *crop);
void free_image(Image* image);
void clear_image(Image image, Color color);

typedef union
{
	struct __attribute__((packed))
	{
		Vec2 p1;
		Vec2 p2;
		Vec2 p3;
		Vec2 p4;
	};
	Vec2 points[4];
} BezierCurve;

void draw_rect(Image image, Vec4 rect, Color color);
void draw_rounded_rect(Image image, Vec4 rect, Color color, float border_radius);
void draw_curve(Image image, BezierCurve curve, Color color);

typedef enum
{
	FONT_TTF,
	FONT_BDF,
} FontFormat;

typedef struct
{
	FontFormat format;
	void *data;
} Font;

void load_font(Font *font, const char *filename);
Vec2 measure_text(Font font, const char* text, int size);
void draw_text(Image image,  Font font, const char *text, int size, Vec2 position, Color text_color);
void free_font(Font *font);
