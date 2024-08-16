#include "drawing.h"
#include "basic.h"

#include <stdbool.h>
#include <math.h>

#define EPSILON 1e-3f;
#define BORDER_RADIUS_THRESHOLD 10.0f;

inline static float lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}

inline static float clamp(float x, float min, float max)
{
	if (x < min) return min;
	if (x > max) return max;
	return x;
}

inline static bool inside_rect(Point p, Rect r)
{
	return p.x >= r.x && p.x <= (r.x + r.w) && p.y >= r.y && p.y <= (r.y + r.h);
}

inline static Color get_pixel(Image image, int x, int y)
{
	assert(image.pixels != NULL);

	if (x < 0 || x >= image.width) return TRANSPARENT;
	if (y < 0 || y >= image.height) return TRANSPARENT;

	return image.pixels[y * image.width + x];
}

inline static void put_pixel(Image image, int x, int y, Color color)
{
	assert(image.pixels != NULL);
	
	if (color.a == 0) return;
	if (x < 0 || x >= image.width) return;
	if (y < 0 || y >= image.height) return;

	image.pixels[y * image.width + x] = color;
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

void clear_image(Image image, Color color)
{
	for (int y = 0; y < image.height; ++y)
	{
		for (int x = 0; x < image.width; ++x)
		{
			put_pixel(image, x, y, color);
		}
	}
}

void draw_rect(Image image, Rect rect, Color color)
{
	for (size_t y = rect.y; y < rect.y + rect.h; ++y)
	{
		for (size_t x = rect.x; x < rect.x + rect.w; ++x)
		{
			put_pixel(image, x, y, color);
		}
	}
}

void draw_rounded_rect(Image image, Rect rect, Color color, float border_radius, Color border_color)
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

				Color base = get_pixel(image, cx, cy);
				Color final = layer_color(base, rect_color);
				put_pixel(image, cx, cy, final);
			}
		}
	}
}

void draw_curve(Image image, BezierCurve curve, Color color)
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
		put_pixel(image, (int)p10.x, (int)p10.y, color);

		t += EPSILON;
	}
}

typedef struct __attribute__((packed))
{
	uint16_t type;
	uint32_t size;
	uint16_t reserved1;
	uint16_t reserved2;
	uint32_t offset;
} BMPHeader;

typedef struct __attribute__((packed))
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
} BMPInfoHeader;

void blur_image(Image image)
{
	assert(image.pixels != NULL);

	static int kernel_size = 3;
	static float kernel[3][3] = {
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

			r = clamp(r, 0, 255);
        	g = clamp(g, 0, 255);
        	b = clamp(b, 0, 255);

			put_pixel(image, x, y, (Color){
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

	Image scaled_image = {0};
	scaled_image.width = scaled_w;
	scaled_image.height = scaled_h;
	size_t size = scaled_w * scaled_h * sizeof(Color);
	scaled_image.pixels = malloc(size);
	assert(scaled_image.pixels != NULL);
	memset(scaled_image.pixels, 0, size);

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

	Image duplicate = {0};
	duplicate.width = image.width;
	duplicate.height = image.height;
	size_t size = image.width * image.height * sizeof(Color);
	duplicate.pixels = malloc(size);
	assert(duplicate.pixels != NULL);
	memcpy(duplicate.pixels, image.pixels, size);

	return duplicate;
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

void draw_image(Image background, ImageArgs *args)
{
	assert(args->image != NULL);
	assert(args->image->pixels != NULL);

	Image *image = args->image;
	Rect rect = args->rect;
	Rect *crop = args->crop;
	if (crop == NULL)
	{
		crop = &(Rect){.x = 0, .y = 0, .w = image->width, .h = image->height};
	}

	float sx = crop->w / rect.w;
	float sy = crop->h / rect.h;

	for (int y = 0; y < rect.h; ++y)
	{
		for (int x = 0; x < rect.w; ++x)
		{
			int ix = (int)(x * sx + crop->x);
			int iy = (int)(y * sy + crop->y);
			int k = iy * image->width + ix;

			Color base = get_pixel(background, x + rect.x, y + rect.y);
			Color final = layer_color(base,  image->pixels[k]);
			put_pixel(background, x + rect.x, y + rect.y, final);
		}
	}
}

void free_image(Image *image)
{
	free(image->pixels);
	image->pixels = NULL;
}

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
			if (code < 0 || code >= 128)
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

Point measure_text_bdf(Font *font, const char* text, int size)
{
	assert(font != NULL);
	assert(font->data != NULL);

	FontBDF *font_bdf = (FontBDF *)font->data;
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

	return (Point){.x=width, .y=height};

}

Point measure_text(Font *font, const char* text, int size)
{
	switch (font->format)
	{
	case FONT_BDF:
		return measure_text_bdf(font, text, size);
		break;
	default:
		fprintf(stderr, "ERROR: Unsupported font format\n");
		break;
	}

	return (Point){.x=0, .y=0};
}

void draw_text_bdf(Image image, TextArgs *args)
{
	assert(args->font != NULL);
	assert(args->font->data != NULL);

	FontBDF *font = (FontBDF *)args->font->data;
	int x = args->position.x;
	int y = args->position.y;

	int n = strlen(args->text);

	float scaling = (float)args->size / (float)font->size;

	for (int i = 0; i < n; ++i)
	{
		char ch = args->text[i];
		FontBDFGlyph glyph = font->glyphs[(int)ch];

		int width = glyph.width * scaling;
		int height = glyph.height * scaling;

		int x_offset = glyph.x_offset * scaling;
		int y_offset = glyph.y_offset * scaling;

		Image glyph_image =
		{
			.width = width,
			.height = height,
			.pixels = malloc(width * height * sizeof(Color)),
		};
		assert(glyph_image.pixels != NULL);
		memset(glyph_image.pixels, 0, width * height * sizeof(Color));

		for (int gy = 0; gy < height; ++gy)
		{
			for (int gx = 0; gx < width; ++gx)
			{
				int row = (float)gy / scaling;
				int col = (float)gx / scaling;

				uint64_t mask = 1 << (16 - col - 1);
				
				Color color = (glyph.bitmap[row] & mask) ? args->color : TRANSPARENT;
				put_pixel(glyph_image, gx, gy, color);
			}
		}

		Image blurred_image = duplicate_image(glyph_image);
		fade_image(blurred_image, 0.8f);
		blur_image(blurred_image);

		ImageArgs blurred_image_args =
		{
			.image = &blurred_image,
			.rect = (Rect)
			{
				.x = x+x_offset,
				.y = y+y_offset,
				.w = blurred_image.width,
				.h = blurred_image.height,
			},
			.crop = NULL,
		};
		draw_image(image, &blurred_image_args);
		free_image(&blurred_image);

		ImageArgs glyph_image_args =
		{
			.image = &glyph_image,
			.rect = (Rect)
			{
				.x = x+x_offset,
				.y = y+y_offset,
				.w = glyph_image.width,
				.h = glyph_image.height,
			},
			.crop = NULL,
		};

		draw_image(image, &glyph_image_args);
		free_image(&glyph_image);

		x += glyph.advance * scaling;
	}
}

void draw_text(Image image, TextArgs *args)
{
	switch (args->font->format)
	{
	case FONT_BDF:
		draw_text_bdf(image, args);
		break;
	default:
		fprintf(stderr, "ERROR: Unsupported font format\n");
		break;
	}
}

void free_font_bdf(Font *font)
{
	FontBDF *font_bdf = (FontBDF *)font->data;
	for (int i = 0; i < 256; ++i)
	{
		free(font_bdf->glyphs[i].bitmap);
	}
	free(font->data);
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

bool button(Env *env, ButtonArgs *args)
{
	assert(args != NULL);

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

	Image background = image_from_env(env);

	draw_rounded_rect(background, args->rect, bg_color, args->border_radius, args->border_color);
	Point text_size = measure_text(args->font, args->text, args->font_size);
	TextArgs text_args = 
	{
		.font = args->font,
		.text = args->text,
		.size = args->font_size,
		.color = args->foreground_color,
		.position = (Point)
		{
			.x = args->rect.x + args->rect.w/2.0 - text_size.x/2.0,
			.y = args->rect.y + args->rect.h/2.0 - text_size.y/2.0,
		},
	};
	draw_text(background, &text_args);
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

	Image background = image_from_env(env);
	draw_rounded_rect(background, args->rect, args->background_color, args->border_radius, args->border_color);

	return clicked;
}
