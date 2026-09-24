#pragma once

#include "env.h"
#include "basic.h"
#include <stdint.h>

// Matches framebuffer byte order
#if defined(_WIN32) || defined(__linux__)
PACK(typedef struct
{
	uint8_t b;
	uint8_t g;
	uint8_t r;
	uint8_t a;
})
Color;
#else
PACK(typedef struct
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
})
Color;
#endif

#define COLOR_RGBA(red, green, blue, alpha) \
	((Color){.r = (red), .g = (green), .b = (blue), .a = (alpha)})

// 0xRRGGBBAA
#define COLOR_HEX(hex)                      \
	COLOR_RGBA(((uint32_t)(hex) >> 24) & 0xFF, \
	           ((uint32_t)(hex) >> 16) & 0xFF, \
	           ((uint32_t)(hex) >> 8) & 0xFF,  \
	           (uint32_t)(hex) & 0xFF)

#define COLOR_TRANSPARENT  COLOR_HEX(0x00000000)
#define COLOR_RED  COLOR_HEX(0xFF0000FF)
#define COLOR_GREEN  COLOR_HEX(0x00FF00FF)
#define COLOR_MAGENTA  COLOR_HEX(0xFF00FFFF)
#define COLOR_WHITE  COLOR_HEX(0xFFFFFFFF)
#define COLOR_BLACK  COLOR_HEX(0x000000FF)
#define COLOR_GRAY  COLOR_HEX(0x666666FF)
#define COLOR_BLUE  COLOR_HEX(0x0000FFFF)
#define COLOR_CYAN  COLOR_HEX(0x00FFFFFF)
#define COLOR_ORANGE  COLOR_HEX(0xFFA500FF)
#define COLOR_PURPLE  COLOR_HEX(0x800080FF)
#define COLOR_YELLOW  COLOR_HEX(0xFFFF00FF)

typedef union
{
	PACK(struct
	{
		float x;
		float y;
	});
	float xy[2];
} Vec2;

typedef union
{
	PACK(struct
	{
		float x;
		float y;
		float w;
		float h;
	});
	PACK(struct
	{
		float left;
		float top;
		float right;
		float bottom;
	});
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
Vec4 rect_intersect(Vec4 a, Vec4 b);

Image image_from_env(Env* env);
Env env_from_image(Image image);

void blur_image(Image image);
void fade_image(Image image, float opacity);
Image scale_image(Image image, float sx, float sy);
Image duplicate_image(Image image);

Image new_image(int width, int height);
void load_image(Image *image, const char *filename);

void draw_image(Image background, Image image, Vec4 rect, Vec4 *crop, Vec4 *clip);
void free_image(Image* image);
void clear_image(Image image, Color color);

typedef union
{
	PACK(struct
	{
		Vec2 p1;
		Vec2 p2;
		Vec2 p3;
		Vec2 p4;
	});
	Vec2 points[4];
} BezierCurve;

void draw_rect(Image image, Vec4 rect, Color color, Vec4 *clip);
void draw_rounded_rect(Image image, Vec4 rect, Color color, float border_radius, Vec4 *clip);
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
void draw_text(Image image, Font font, const char *text, int size, Vec2 position, Color text_color, Vec4 *clip);
void free_font(Font *font);
