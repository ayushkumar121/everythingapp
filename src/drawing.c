#include "drawing.h"
#include "basic.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EPSILON 1e-3f
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

Vec4 rect_intersect(Vec4 a, Vec4 b)
{
	float left = fmaxf(a.x, b.x);
	float top = fmaxf(a.y, b.y);
	float right = fminf(a.x + a.w, b.x + b.w);
	float bottom = fminf(a.y + a.h, b.y + b.h);

	return (Vec4)
	{
		.x = left, .y = top, .w = fmaxf(right - left, 0.0f), .h = fmaxf(bottom - top, 0.0f)
	};
}

// Half-open pixel range [x0, x1) x [y0, y1) of rect, limited to clip and the image
typedef struct
{
	int x0;
	int y0;
	int x1;
	int y1;
} PixelBounds;

static PixelBounds pixel_bounds(Image image, Vec4 rect, Vec4 *clip)
{
	if (clip != NULL)
	{
		rect = rect_intersect(rect, *clip);
	}

	return (PixelBounds)
	{
		.x0 = (int)fmaxf(floorf(rect.x), 0.0f),
		.y0 = (int)fmaxf(floorf(rect.y), 0.0f),
		.x1 = (int)fminf(ceilf(rect.x + rect.w), (float)image.width),
		.y1 = (int)fminf(ceilf(rect.y + rect.h), (float)image.height),
	};
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
		.scale = 1.0f,
	};
}

// Blend onto an opaque pixel: c = (top * a + bottom * (255 - a)) / 255, rounded.
// Red and blue share one multiply since each fits in 16 bits.
static inline Color blend_opaque(Color bottom, Color top)
{
	uint32_t a = COLOR_A(top);
	uint32_t ia = 255 - a;

	uint32_t rb = (top & 0x00FF00FF) * a + (bottom & 0x00FF00FF) * ia + 0x00800080;
	rb = ((rb + ((rb >> 8) & 0x00FF00FF)) >> 8) & 0x00FF00FF;

	uint32_t g = (top & 0x0000FF00) * a + (bottom & 0x0000FF00) * ia + 0x00008000;
	g = ((g + ((g >> 8) & 0x0000FF00)) >> 8) & 0x0000FF00;

	return 0xFF000000u | rb | g;
}

// Kept out of line: inlined into put_pixel, the compiler blends every pixel even when it's opaque
static NOINLINE Color layer_color(Color bottom, Color top)
{
	uint32_t top_alpha = COLOR_A(top);
	uint32_t bottom_alpha = COLOR_A(bottom);

	if (bottom_alpha == 255)
	{
		return blend_opaque(bottom, top);
	}

	// Channels are weighted by 255 * alpha so everything stays in integers
	uint32_t top_weight = top_alpha * 255;
	uint32_t bottom_weight = bottom_alpha * (255 - top_alpha);
	uint32_t out_weight = top_weight + bottom_weight;
	if (out_weight == 0)
	{
		return COLOR_TRANSPARENT;
	}

	uint32_t half = out_weight / 2;
	uint32_t r = (COLOR_R(top) * top_weight + COLOR_R(bottom) * bottom_weight + half) / out_weight;
	uint32_t g = (COLOR_G(top) * top_weight + COLOR_G(bottom) * bottom_weight + half) / out_weight;
	uint32_t b = (COLOR_B(top) * top_weight + COLOR_B(bottom) * bottom_weight + half) / out_weight;

	return COLOR_ARGB((out_weight + 127) / 255, r, g, b);
}

static inline Color get_pixel(Image image, int x, int y)
{
	assert(image.pixels != NULL);

	if (x < 0 || x >= image.width) return COLOR_TRANSPARENT;
	if (y < 0 || y >= image.height) return COLOR_TRANSPARENT;

	return image.pixels[y * image.width + x];
}

static inline void put_pixel(Image image, int x, int y, Color color)
{
	assert(image.pixels != NULL);

	if (x < 0 || x >= image.width) return;
	if (y < 0 || y >= image.height) return;
	if (COLOR_A(color) == 0) return;

	if (COLOR_A(color) == 255)
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
	return COLOR_ARGB(
		(uint8_t)lerp(COLOR_A(a), COLOR_A(b), t),
		(uint8_t)lerp(COLOR_R(a), COLOR_R(b), t),
		(uint8_t)lerp(COLOR_G(a), COLOR_G(b), t),
		(uint8_t)lerp(COLOR_B(a), COLOR_B(b), t));
}

Vec2 lerp_points(Vec2 a, Vec2 b, float t)
{
	Vec2 p = {0};
	p.x = lerp(a.x, b.x, t);
	p.y = lerp(a.y, b.y, t);
	return p;
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

// Fills [x0, x1) of row y, the caller keeps the span inside the image
static void fill_span(Image image, int y, int x0, int x1, Color color)
{
	Color *row = image.pixels + y * image.width;

	if (COLOR_A(color) == 255)
	{
		for (int x = x0; x < x1; ++x) row[x] = color;
	}
	else if (COLOR_A(color) > 0)
	{
		// Framebuffer rows are opaque, and then the blend is a loop the compiler vectorizes
		Color alpha = 0xFF000000u;
		for (int x = x0; x < x1; ++x) alpha &= row[x];

		if (alpha == 0xFF000000u)
		{
			for (int x = x0; x < x1; ++x) row[x] = blend_opaque(row[x], color);
		}
		else
		{
			for (int x = x0; x < x1; ++x) row[x] = layer_color(row[x], color);
		}
	}
}

void draw_rect(Image image, Vec4 rect, Color color, Vec4 *clip)
{
	PixelBounds b = pixel_bounds(image, rect, clip);

	for (int y = b.y0; y < b.y1; ++y)
	{
		fill_span(image, y, b.x0, b.x1, color);
	}
}

void draw_rounded_rect(Image image, Vec4 rect, Color color, float border_radius, Vec4 *clip)
{
	float r = fminf(border_radius, fminf(rect.w, rect.h) / 2);

	// The outline includes the right and bottom edges
	Vec4 outline = rect;
	outline.w += 1;
	outline.h += 1;
	PixelBounds b = pixel_bounds(image, outline, clip);

	float top_centre = rect.y + r;
	float bottom_centre = rect.y + rect.h - r;

	for (int y = b.y0; y < b.y1; ++y)
	{
		// Only rows beside a corner are inset, by where they cross the corner's circle
		float dy = 0.0f;
		if (y < top_centre) dy = top_centre - y;
		else if (y > bottom_centre) dy = y - bottom_centre;

		float inset = r - sqrtf(fmaxf(r * r - dy * dy, 0.0f));
		int x0 = (int)ceilf(rect.x + inset);
		int x1 = (int)floorf(rect.x + rect.w - inset) + 1;

		fill_span(image, y, x0 > b.x0 ? x0 : b.x0, x1 < b.x1 ? x1 : b.x1, color);
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

// Area-averaging resize for good looking downscales; colours are weighted by alpha
// so transparent pixels don't darken the edges
Image resize_image(Image image, int width, int height)
{
	assert(image.pixels != NULL);

	Image resized = new_image(width, height);

	for (int y = 0; y < height; ++y)
	{
		int sy0 = y * image.height / height;
		int sy1 = (y + 1) * image.height / height;
		if (sy1 <= sy0) sy1 = sy0 + 1;

		for (int x = 0; x < width; ++x)
		{
			int sx0 = x * image.width / width;
			int sx1 = (x + 1) * image.width / width;
			if (sx1 <= sx0) sx1 = sx0 + 1;

			uint64_t a = 0, r = 0, g = 0, b = 0;
			for (int sy = sy0; sy < sy1; ++sy)
			{
				for (int sx = sx0; sx < sx1; ++sx)
				{
					Color c = image.pixels[sy * image.width + sx];
					uint32_t ca = COLOR_A(c);
					a += ca;
					r += COLOR_R(c) * ca;
					g += COLOR_G(c) * ca;
					b += COLOR_B(c) * ca;
				}
			}

			uint64_t count = (uint64_t)(sx1 - sx0) * (sy1 - sy0);
			if (a > 0)
			{
				resized.pixels[y * width + x] = COLOR_ARGB((a + count / 2) / count, (r + a / 2) / a, (g + a / 2) / a, (b + a / 2) / a);
			}
		}
	}

	return resized;
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

// Scales the bits under mask to 0..255
static uint8_t bmp_channel(uint32_t pixel, uint32_t mask)
{
	if (mask == 0) return 0;

	int shift = 0;
	while (((mask >> shift) & 1) == 0) shift++;

	uint32_t max = mask >> shift;
	return (uint8_t)(((pixel & mask) >> shift) * 255 / max);
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

	// BI_BITFIELDS stores each channel's bit mask, which is how most editors save alpha
	const uint32_t BMP_RGB = 0;
	const uint32_t BMP_BITFIELDS = 3;
	bool has_masks = info_header.compression == BMP_BITFIELDS;
	if (info_header.compression != BMP_RGB && !(has_masks && (info_header.bits_per_pixel == 16 || info_header.bits_per_pixel == 32)))
	{
		fprintf(stderr, "ERROR: Compressed BMP files are not supported\n");
		fclose(file);
		return;
	}

	// Masks follow the 40 byte header; V4/V5 headers include an alpha mask
	uint32_t masks[4] = {0};
	if (has_masks)
	{
		fseek(file, sizeof(BMPHeader) + sizeof(BMPInfoHeader), SEEK_SET);
		fread(masks, sizeof(uint32_t), info_header.size >= 56 ? 4 : 3, file);
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
			Color color = COLOR_TRANSPARENT;
			uint8_t bytes[4] = {0};
			for (int i = 0; i < bytes_per_pixel; ++i)
			{
				fread(&bytes[i], sizeof(uint8_t), 1, file);
			}

			if (has_masks)
			{
				uint32_t pixel = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
				uint8_t alpha = masks[3] ? bmp_channel(pixel, masks[3]) : 255;
				color = COLOR_ARGB(alpha, bmp_channel(pixel, masks[0]), bmp_channel(pixel, masks[1]), bmp_channel(pixel, masks[2]));
			}
			else if (bytes_per_pixel == 1)
			{
				color = COLOR_ARGB(255, bytes[0], bytes[0], bytes[0]);
			}
			else if (bytes_per_pixel == 3)
			{
				color = COLOR_ARGB(255, bytes[2], bytes[1], bytes[0]);
			}
			else if (bytes_per_pixel == 4)
			{
				color = COLOR_ARGB(bytes[3], bytes[2], bytes[1], bytes[0]);
			}

			image->pixels[y * image->width + x] = color;
		}
		fseek(file, padding, SEEK_CUR);
	}

	fclose(file);
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

void draw_image(Image background, Image image, Vec4 rect, Vec4 *crop, Vec4 *clip)
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

	PixelBounds b = pixel_bounds(background, rect, clip);

	for (int y = b.y0; y < b.y1; ++y)
	{
		const int iy = (int)((y - rect.y) * sy + crop_rect.y);
		if (iy < 0 || iy >= image.height) continue;

		for (int x = b.x0; x < b.x1; ++x)
		{
			const int ix = (int)((x - rect.x) * sx + crop_rect.x);

			if (ix >= 0 && ix < image.width)
			{
				put_pixel(background, x, y, image.pixels[iy * image.width + ix]);
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
	int ascent;
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
		else if (strncmp(line, "FONT_ASCENT", 11) == 0)
		{
			sscanf(line, "FONT_ASCENT %d", &font_bdf->ascent);
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

	if (font_bdf->ascent == 0)
	{
		font_bdf->ascent = font_bdf->size;
	}

	fclose(file);
}

int font_size(Font font)
{
	switch (font.format)
	{
		case FONT_BDF:
			return ((FontBDF *)font.data)->size;
		default:
			return 0;
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
	int line_width = 0;
	int line_top = 0;

	for (int i = 0; i < n; ++i)
	{
		char ch = text[i];
		if (ch == '\n')
		{
			line_width = 0;
			line_top += size;
			continue;
		}

		FontBDFGlyph glyph = font_bdf->glyphs[(int)ch];

		line_width += glyph.advance * scaling;
		width = fmaxf(width, line_width);
		height = fmaxf(height, line_top + glyph.height * scaling);
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

void draw_text_bdf(Image image, Font font, const char *text, int size, Vec2 position, Color text_color, Vec4 *clip)
{
	assert(font.data != NULL);

	FontBDF *font_bdf = (FontBDF *)font.data;
	int x = position.x;
	int y = position.y;

	int n = strlen(text);

	float scaling = (float)size / (float)font_bdf->size;
	static int samples = 3;

	PixelBounds b = pixel_bounds(image, (Vec4){.x = 0, .y = 0, .w = image.width, .h = image.height}, clip);

	for (int i = 0; i < n; ++i)
	{
		char ch = text[i];
		if (ch == '\n')
		{
			x = position.x;
			y += size;
			continue;
		}

		FontBDFGlyph glyph = font_bdf->glyphs[(int)ch];

		int width = glyph.width * scaling;
		int height = glyph.height * scaling;

		// position is the top-left of the line, glyph offsets are relative to the baseline
		int x_offset = glyph.x_offset * scaling;
		int y_offset = (font_bdf->ascent - glyph.height - glyph.y_offset) * scaling;

		int left = x + x_offset;
		int top = y + y_offset;
		int gx0 = b.x0 > left ? b.x0 - left : 0;
		int gy0 = b.y0 > top ? b.y0 - top : 0;
		int gx1 = b.x1 - left < width ? b.x1 - left : width;
		int gy1 = b.y1 - top < height ? b.y1 - top : height;

		for (int gy = gy0; gy < gy1; ++gy)
		{
			for (int gx = gx0; gx < gx1; ++gx)
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
				Color color = (text_color & 0x00FFFFFF) | ((uint32_t)(COLOR_A(text_color) * coverage_ratio) << 24);
				put_pixel(image, left + gx, top + gy, color);
			}
		}

		x += glyph.advance * scaling;
	}
}

void draw_text(Image image, Font font, const char *text, int size, Vec2 position, Color text_color, Vec4 *clip)
{
	switch (font.format)
	{
		case FONT_BDF:
			draw_text_bdf(image, font, text, size, position, text_color, clip);
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
