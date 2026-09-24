#pragma once

#include "env.h"
#include "basic.h"
#include <stdint.h>

// 0xAARRGGBB, stored as B,G,R,A in memory on little-endian
typedef uint32_t Color;

#define COLOR_ARGB(a, r, g, b) \
	((Color)(((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b)))
#define COLOR_A(c) (((c) >> 24) & 0xFF)
#define COLOR_R(c) (((c) >> 16) & 0xFF)
#define COLOR_G(c) (((c) >> 8) & 0xFF)
#define COLOR_B(c) ((c) & 0xFF)

#define COLOR_TRANSPARENT  0x00000000u
#define COLOR_RED  0xFFFF0000u
#define COLOR_GREEN  0xFF00FF00u
#define COLOR_MAGENTA  0xFFFF00FFu
#define COLOR_WHITE  0xFFFFFFFFu
#define COLOR_BLACK  0xFF000000u
#define COLOR_GRAY  0xFF666666u
#define COLOR_BLUE  0xFF0000FFu
#define COLOR_CYAN  0xFF00FFFFu
#define COLOR_ORANGE  0xFFFFA500u
#define COLOR_PURPLE  0xFF800080u
#define COLOR_YELLOW  0xFFFFFF00u

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
int font_size(Font font);
Vec2 measure_text(Font font, const char* text, int size);
void draw_text(Image image, Font font, const char *text, int size, Vec2 position, Color text_color, Vec4 *clip);
void free_font(Font *font);
