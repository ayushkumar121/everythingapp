#include "drawing.h"
#include "basic.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EPSILON 1e-3f
#define BORDER_RADIUS_THRESHOLD 10.0f
#define THREAD_COUNT 4

Vec4 v4_add_v4(Vec4 a, Vec4 b)
{
	a.x += b.x;
	a.y += b.y;
	a.w += b.w;
	a.h += b.h;
	return a;
}

Vec4 v4_add_v2(Vec4 a, Vec2 b)
{
	a.x += b.x;
	a.y += b.y;
	return a;
}

Vec2 v2_add_v2(Vec2 a, Vec2 b)
{
	a.x += b.x;
	a.y += b.y;
	return a;
}

inline float lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}

inline float clamp(float x, float min, float max)
{
	if (x < min) return min;
	if (x > max) return max;
	return x;
}

bool inside_rect(Vec2 p, Vec4 r)
{
    return (p.x >= r.x) && (p.x <= (r.x + r.w)) && (p.y >= r.y) && (p.y <= (r.y + r.h));
}

Image image_from_env(Env* env)
{
	assert(env != NULL);
	assert(env->buffer != NULL);

	Image image = {0};
	image.width = env->width;
	image.height = env->height;
	image.pixels = (Color *)env->buffer;
	return image;
}

Env env_from_image(Image image)
{
	return (Env)
	{
		.width = image.width,
		.height = image.height,
		.buffer = (uint8_t*)image.pixels,
	};
}

Color layer_color(Color bottom, Color top)
{
	float top_alpha = (float) top.a / 255.0f;
    float bottom_alpha = (float) bottom.a / 255.0f;
    float out_alpha = top_alpha + bottom_alpha * (1.0f - top_alpha);
    if (out_alpha == 0)
    {
        return (Color) { .r = 0, .g = 0, .b = 0, .a = 0 };
    }

    float r = ((float) top.r * top_alpha + (float) bottom.r * bottom_alpha * (1.0f - top_alpha)) / out_alpha;
    float g = ((float) top.g * top_alpha + (float) bottom.g * bottom_alpha * (1.0f - top_alpha)) / out_alpha;
    float b = ((float) top.b * top_alpha + (float) bottom.b * bottom_alpha * (1.0f - top_alpha)) / out_alpha;

    return (Color)
    {
        .r = (uint8_t) r,
        .g = (uint8_t) g,
        .b = (uint8_t) b,
        .a = (uint8_t) (out_alpha * 255.0f)
    };
}

inline Color get_pixel(Image image, int x, int y)
{
	assert(image.pixels != NULL);

	if (x < 0 || x >= image.width) return COLOR_TRANSPARENT;
	if (y < 0 || y >= image.height) return COLOR_TRANSPARENT;

	return image.pixels[y * image.width + x];
}

inline void put_pixel(Image image, int x, int y, Color color)
{
	assert(image.pixels != NULL);

	if (x < 0 || x >= image.width) return;
	if (y < 0 || y >= image.height) return;
	if (color.a == 0) return;

	if (color.a == 255)
	{
		image.pixels[y * image.width + x] = color;
	}
	else
	{
		image.pixels[y * image.width + x] = layer_color(get_pixel(image, x, y), color);
	}
}

Color mix_color(Color a, Color b, float t)
{
	Color c = {0};
	c.r = lerp(a.r, b.r, t);
	c.g = lerp(a.g, b.g, t);
	c.b = lerp(a.b, b.b, t);
	c.a = lerp(a.a, b.a, t);
	return c;
}

Vec2 lerp_points(Vec2 a, Vec2 b, float t)
{
	Vec2 p = {0};
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

BorderCheckResult border_radius_check(Vec4 rect, int cx, int cy, float r, float r_squared)
{
	bool inside_border = false;
	bool on_border = false;

	// Top Left corner
	float c = powf(cx - (rect.x + r), 2.0f) + powf(cy - (rect.y + r), 2.0f);
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

void clear_image(Image image, Color color)
{
	for (int y = 0; y < image.height; ++y)
	{
		for (int x = 0; x < image.width; ++x)
		{
			image.pixels[y * image.width + x] = color;
		}
	}
}

void draw_rect(Image image, Vec4 rect, Color color)
{
	for (size_t y = rect.y; y < rect.y + rect.h; ++y)
	{
		for (size_t x = rect.x; x < rect.x + rect.w; ++x)
		{
			put_pixel(image, x, y, color);
		}
	}
}

void draw_rounded_rect(Image image, Vec4 rect, Color color, float border_radius)
{
	float r_squared = border_radius * border_radius;

	for (size_t cy = rect.y; cy <= rect.y + rect.h; ++cy)
	{
		for (size_t cx = rect.x; cx <= rect.x + rect.w; ++cx)
		{
			BorderCheckResult result = border_radius_check(rect, cx, cy, border_radius, r_squared);
			if (result != OUTSIDE_BORDER)
			{
				put_pixel(image, cx, cy, color);
			}
		}
	}
}

void draw_curve(Image image, BezierCurve curve, Color color)
{
	float t = 0.0f;
	while (t <= 1.0f)
	{
		Vec2 p5 = lerp_points(curve.p1, curve.p2, t);
		Vec2 p6 = lerp_points(curve.p2, curve.p3, t);
		Vec2 p7 = lerp_points(curve.p3, curve.p4, t);

		Vec2 p8 = lerp_points(p5, p6, t);
		Vec2 p9 = lerp_points(p6, p7, t);

		Vec2 p10 = lerp_points(p8, p9, t);
		put_pixel(image, (int)p10.x, (int)p10.y, color);

		t += EPSILON;
	}
}

PACK(typedef struct
{
	uint16_t type;
	uint32_t size;
	uint16_t reserved1;
	uint16_t reserved2;
	uint32_t offset;
})
BMPHeader;

PACK(typedef struct
{
	uint32_t size;
	int32_t width;
	int32_t height;
	uint16_t planes;
	uint16_t bits_per_pixel;
	uint32_t compression;
	uint32_t image_size;
	int32_t x_pixels_per_meter;
	int32_t y_pixels_per_meter;
	uint32_t colors_used;
	uint32_t colors_important;
})
BMPInfoHeader;

void blur_image(Image image)
{
	assert(image.pixels != NULL);

	static int kernel_size = 3;
	static float kernel[3][3] =
	{
		{1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f},
		{2.0f/16.0f, 4.0f/16.0f, 2.0f/16.0f},
		{1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f},
	};

	for (int y=0; y<image.height; ++y)
	{
		for (int x=0; x<image.width; ++x)
		{
			float r = 0.0f;
			float g = 0.0f;
			float b = 0.0f;
			float a = 0.0f;

			for (int ky=0; ky<kernel_size; ++ky)
			{
				for (int kx=0; kx<kernel_size; ++kx)
				{
					int px = x + kx - kernel_size / 2;
					int py = y + ky - kernel_size / 2;

					Color color = get_pixel(image, px, py);

					r += color.r * kernel[ky][kx];
					g += color.g * kernel[ky][kx];
					b += color.b * kernel[ky][kx];
					a += color.a * kernel[ky][kx];
				}
			}

			r = clamp(r, 0.0f, 255.0f);
			g = clamp(g, 0.0f, 255.0f);
			b = clamp(b, 0.0f, 255.0f);

			put_pixel(image, x, y, (Color)
			{
				.r=(uint8_t)r,
				.g=(uint8_t)g,
				.b=(uint8_t)b,
				.a=(uint8_t)a,
			});
		}
	}
}

void fade_image(Image image, float opacity)
{
	assert(image.pixels != NULL);

	for (int y=0; y<image.height; ++y)
	{
		for (int x=0; x<image.width; ++x)
		{
			Color color = get_pixel(image, x, y);
			color.a = (uint8_t)((float)color.a * opacity);
			put_pixel(image, x, y, color);
		}
	}
}

Image scale_image(Image image, float sx, float sy)
{
	assert(image.pixels != NULL);

	int scaled_w = (int)((float)image.width * sx);
	int scaled_h = (int)((float)image.height * sy);

	Image scaled_image = new_image(scaled_w, scaled_h);

	for (int y = 0; y < scaled_h; ++y)
	{
		for (int x = 0; x < scaled_w; ++x)
		{
			int src_x = (int)(((float)x / sx));
			int src_y = (int)(((float)y / sy));

			Color c = get_pixel(image, src_x, src_y);
			put_pixel(scaled_image, x, y, c);
		}
	}

	return scaled_image;
}

Image duplicate_image(Image image)
{
	assert(image.pixels != NULL);

	Image duplicate = new_image(image.width, image.height);
	memcpy(duplicate.pixels, image.pixels, image.width * image.height * sizeof(Color));

	return duplicate;
}

Image new_image(int width, int height)
{
	Image image = {0};
	image.width = width;
	image.height = height;
	size_t size = width * height * sizeof(Color);
	image.pixels = malloc(size);
	assert(image.pixels != NULL);
	memset(image.pixels, 0, size);

	return image;
}

void load_image_bmp(Image *image, const char *filename)
{
	FILE *file = fopen(filename, "rb");
	if (file == NULL)
	{
		fprintf(stderr, "ERROR: Failed to open file\n");
		return;
	}

	BMPHeader header;
	fread(&header, sizeof(BMPHeader), 1, file);
	if (header.type != 0x4D42)
	{
		fprintf(stderr, "ERROR: Invalid BMP file\n");
		fclose(file);
		return;
	}

	BMPInfoHeader info_header;
	fread(&info_header, sizeof(BMPInfoHeader), 1, file);

	if (info_header.compression != 0)
	{
		fprintf(stderr, "ERROR: Compressed BMP files are not supported\n");
		fclose(file);
		return;
	}

	image->width = info_header.width;
	image->height = info_header.height;
	image->pixels = malloc(image->width * image->height * sizeof(Color));

	fprintf(stderr, "INFO: Image size: %dx%d\n", image->width, image->height);
	fprintf(stderr, "INFO: Bits per pixel: %d\n", info_header.bits_per_pixel);

	fseek(file, header.offset, SEEK_SET);
	int bytes_per_pixel = info_header.bits_per_pixel / 8;
	int bytes_per_scanline = image->width * bytes_per_pixel;
	int padding = (4 - (bytes_per_scanline % 4)) % 4;

	for (int y = image->height - 1; y >= 0; --y)
	{
		for (int x = 0; x < image->width; ++x)
		{
			Color color = {0};
			uint8_t bytes[4] = {0};
			for (int i = 0; i < bytes_per_pixel; ++i)
			{
				fread(&bytes[i], sizeof(uint8_t), 1, file);
			}

			if (bytes_per_pixel == 1)
			{
				color.b = bytes[0];
				color.g = bytes[0];
				color.r = bytes[0];
				color.a = 255;
			}
			else if (bytes_per_pixel == 3)
			{
				color.b = bytes[0];
				color.g = bytes[1];
				color.r = bytes[2];
				color.a = 255;
			}
			else if (bytes_per_pixel == 4)
			{
				color.b = bytes[0];
				color.g = bytes[1];
				color.r = bytes[2];
				color.a = bytes[3];
			}

			image->pixels[y * image->width + x] = color;
		}
		fseek(file, padding, SEEK_CUR);
	}
}

void load_image(Image *image, const char *filename)
{
	fprintf(stderr, "INFO: Loading image: %s\n", filename);
	const char *ext = filename + strlen(filename) - 4;
	if (strcmp(ext, ".bmp") == 0)
	{
		load_image_bmp(image, filename);
	}
	else
	{
		fprintf(stderr, "Unsupported image format\n");
	}
}

typedef struct
{
	Image background;
	Image image;
	Vec4 rect;
	Vec4 crop;
} DrawImageThreadArgs;

void draw_image(Image background, Image image, Vec4 rect, Vec4 *crop)
{
	assert(image.pixels != NULL);
	assert(background.pixels != NULL);

	Vec4 crop_rect;
	if (crop != NULL)
	{
		crop_rect = *crop;
	}
	else
	{
		crop_rect = (Vec4)
		{
			.x = 0, .y = 0, .w = image.width, .h = image.height
		};
	}

	const float sx = crop_rect.w / rect.w;
	const float sy = crop_rect.h / rect.h;

	for (int y = 0; y < rect.h; ++y)
	{
		for (int x = 0; x < rect.w; ++x)
		{
			const int ix = (int)(x * sx + crop_rect.x);
			const int iy = (int)(y * sy + crop_rect.y);

			if (ix >= 0 && ix < image.width && iy >= 0 && iy < image.height)
			{
				int k = iy * image.width + ix;
				put_pixel(background, x + rect.x, y + rect.y, image.pixels[k]);
			}
		}
	}
}

void free_image(Image *image)
{
	free(image->pixels);
	image->pixels = NULL;
}

typedef struct
{
	int width;
	int height;
	int x_offset;
	int y_offset;
	int advance;
	uint64_t *bitmap;
} FontBDFGlyph;

#define FONT_BDF_GLYPH_COUNT 128
typedef struct
{
	int size;
	int x_dpi;
	int y_dpi;
	FontBDFGlyph glyphs[FONT_BDF_GLYPH_COUNT];
} FontBDF;

void load_font_bdf(Font *font, const char *filename)
{
	FILE *file = fopen(filename, "r");
	if (file == NULL)
	{
		fprintf(stderr, "ERROR: Failed to open file\n");
		return;
	}

	font->format = FONT_BDF;
	font->data = malloc(sizeof(FontBDF));
	memset(font->data, 0, sizeof(FontBDF));

	FontBDF *font_bdf = (FontBDF *)font->data;

	char line[256];
	int code = 0;
	int i = 0;
	bool bitmap = false;

	while (fgets(line, sizeof(line), file))
	{
		if (strncmp(line, "ENCODING", 8) == 0)
		{
			code = 0;
			sscanf(line, "ENCODING %d", &code);
		}
		else if (strncmp(line, "SIZE", 4) == 0)
		{
			int size, x_dpi, y_dpi;
			sscanf(line, "SIZE %d %d %d", &size, &x_dpi, &y_dpi);
			font_bdf->size = size;
			font_bdf->x_dpi = x_dpi;
			font_bdf->y_dpi = y_dpi;
		}
		else
		{
			if (code < 0 || code >= FONT_BDF_GLYPH_COUNT)
				continue;

			if (strncmp(line, "BBX", 3) == 0)
			{
				int width, height, x_offset, y_offset;
				sscanf(line, "BBX %d %d %d %d", &width, &height, &x_offset, &y_offset);
				font_bdf->glyphs[code].width = width;
				font_bdf->glyphs[code].height = height;
				font_bdf->glyphs[code].x_offset = x_offset;
				font_bdf->glyphs[code].y_offset = y_offset;
			}
			else if (strncmp(line, "BITMAP", 6) == 0)
			{
				bitmap = true;
				i = 0;

				size_t bitmap_size = font_bdf->glyphs[code].height * sizeof(uint64_t);
				font_bdf->glyphs[code].bitmap = malloc(bitmap_size);
				memset(font_bdf->glyphs[code].bitmap, 0, bitmap_size);
			}
			else if (strncmp(line, "DWIDTH", 6) == 0)
			{
				int advance;
				sscanf(line, "DWIDTH %d", &advance);
				font_bdf->glyphs[code].advance = advance;
			}
			else if (strncmp(line, "ENDCHAR", 7) == 0)
			{
				bitmap = false;
			}
			else if (bitmap)
			{
				uint64_t row;
				sscanf(line, "%llx", &row);

				font_bdf->glyphs[code].bitmap[i] = row;
				i++;
			}
		}
	}
}

void load_font(Font *font, const char *filename)
{
	fprintf(stderr, "INFO: Loading font: %s\n", filename);
	const char *ext = filename + strlen(filename) - 4;
	if (strcmp(ext, ".bdf") == 0)
	{
		load_font_bdf(font, filename);
	}
	else
	{
		fprintf(stderr, "Unsupported font format\n");
	}
}

Vec2 measure_text_bdf(Font font, const char* text, int size)
{
	assert(font.data != NULL);

	FontBDF *font_bdf = (FontBDF *)font.data;
	int n = strlen(text);

	float scaling = (float)size / (float)font_bdf->size;

	int width = 0;
	int height = 0;

	for (int i = 0; i < n; ++i)
	{
		char ch = text[i];
		FontBDFGlyph glyph = font_bdf->glyphs[(int)ch];

		width += glyph.advance * scaling + size/10.0f;
		height = fmaxf(height, glyph.height * scaling);
	}

	return (Vec2)
	{
		.x=width, .y=height
	};

}

Vec2 measure_text(Font font, const char* text, int size)
{
	switch (font.format)
	{
		case FONT_BDF:
			return measure_text_bdf(font, text, size);
		default:
			fprintf(stderr, "ERROR: Unsupported font format\n");
			break;
	}

	return (Vec2)
	{
		.x=0, .y=0
	};
}

void draw_text_bdf(Image image, Font font, const char *text, int size, Vec2 position, Color text_color)
{
	assert(font.data != NULL);

	FontBDF *font_bdf = (FontBDF *)font.data;
	int x = position.x;
	int y = position.y;

	int n = strlen(text);

	float scaling = (float)size / (float)font_bdf->size;
	static int samples = 3;

	for (int i = 0; i < n; ++i)
	{
		char ch = text[i];
		FontBDFGlyph glyph = font_bdf->glyphs[(int)ch];

		int width = glyph.width * scaling;
		int height = glyph.height * scaling;

		int x_offset = glyph.x_offset * scaling;
		int y_offset = glyph.y_offset * scaling;

		for (int gy = 0; gy < height; ++gy)
		{
			for (int gx = 0; gx < width; ++gx)
			{
				int coverage = 0;
				for (int sy = 0; sy < samples; ++sy)
				{
					for (int sx = 0; sx < samples; ++sx)
					{
						float sample_y = ((float)gy + (float)sy / samples) / scaling;
						float sample_x = ((float)gx + (float)sx / samples) / scaling;

						int row = (int)sample_y;
						int col = (int)sample_x;

						if (row >= 0 && row < glyph.height && col >= 0 && col < glyph.width)
						{
							uint64_t mask = 1 << (16 - col - 1);

							if (glyph.bitmap[row] & mask)
								coverage++;
						}
					}
				}

				float coverage_ratio = (float)coverage / (samples * samples);
				Color base = get_pixel(image, x + gx + x_offset, y + gy + y_offset);
				Color color = mix_color(base, text_color, coverage_ratio);
				put_pixel(image, x + gx + x_offset, y + gy + y_offset, color);
			}
		}

		x += glyph.advance * scaling;
	}
}

void draw_text(Image image, Font font, const char *text, int size, Vec2 position, Color text_color)
{
	switch (font.format)
	{
		case FONT_BDF:
			draw_text_bdf(image, font, text, size, position, text_color);
			break;
		default:
			fprintf(stderr, "ERROR: Unsupported font format\n");
			break;
	}
}

void free_font_bdf(Font *font)
{
	FontBDF *font_bdf = (FontBDF *)font->data;
	for (int i = 0; i < FONT_BDF_GLYPH_COUNT; ++i)
	{
		free(font_bdf->glyphs[i].bitmap);
	}
	free(font->data);
	font->data = NULL;
}

void free_font(Font *font)
{
	switch (font->format)
	{
		case FONT_BDF:
			free_font_bdf(font);
			break;
		default:
			fprintf(stderr, "ERROR: Unsupported font format\n");
			break;
	}
}
