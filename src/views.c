#include "basic.h"
#include "views.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void dump_image(Image image, const char *filename)
{
	FILE *file = fopen(filename, "wb");
	if (!file) return;
	fprintf(file, "P6\n%d %d\n255\n", image.width, image.height);
	for (int y = 0; y < image.height; y++)
	{
		for (int x = 0; x < image.width; x++)
		{
			Color color = image.pixels[y * image.width + x];
			char* channels = (char*)&color;
			fwrite(channels, 1, 3, file);
		}
	}
	fclose(file);
}

Point mouse_position(Env* env)
{
	return (Point)
	{
		.x = env->mouse_x,
		.y = env->mouse_y
	};
}

Env new_env(Env* env, int width, int height)
{
	assert (env != NULL);
	Env new_env = *env;
	new_env.width = width;
	new_env.height = height;
	size_t size = width * height * sizeof(Color);
	new_env.buffer = malloc(size);
	assert (new_env.buffer != NULL);
	memset(new_env.buffer, 0, size);
	return new_env;
}

void draw_view(View* view, Env *env)
{
	assert(view != NULL);
	assert(env != NULL);

	Rect rect = rect_add_point(view->rect, view->offset);
	if (view->draw != NULL)
	{
		view->draw(view, rect, env);
	}

	for (size_t i=0; i < view->children.length; i++)
	{
		View* child = view->children.items[i];
		child->offset = (Point) {.x=rect.x,.y = rect.y};
		draw_view(child, env);
	}
}

void destroy_view(View* view)
{
	for (size_t i = 0; i < view->children.length; i++)
	{
		destroy_view(view->children.items[i]);
	}

	if (view->children.items)
	{
		array_free(&view->children);
	}

	free(view);
}

#define SCROLL_BAR_THICKNESS 10

void draw_scroll_view(View* view, Rect rect, Env *env)
{
	ScrollView* scroll_view = (ScrollView*) view;

	float content_size = 0.0f;
	float total_scroll = 0.0f;
	float item_size = 0.0f;
	for (size_t i = 0; i < scroll_view->base.children.length; i++)
	{
		View* child = scroll_view->base.children.items[i];
		if (scroll_view->axis == DIRECTION_HORIZONTAL)
		{
			content_size = fmaxf(content_size, child->rect.h);
			total_scroll += child->rect.w;
			item_size = fmaxf(item_size, child->rect.w);
		}
		else
		{
			content_size = fmaxf(content_size, child->rect.w);
			total_scroll += child->rect.h;
			item_size = fmaxf(item_size, child->rect.h);
		}
	}
	content_size+=10; // Padding

	Rect scroll_bar;
	int scroll_bar_button_size;
	if (scroll_view->axis == DIRECTION_HORIZONTAL)
	{
		scroll_bar.x = rect.x;
		scroll_bar.y = rect.y + content_size;
		scroll_bar.w = rect.w;
		scroll_bar.h = SCROLL_BAR_THICKNESS;

		scroll_bar_button_size = scroll_bar.w / view->children.length;
	}
	else
	{
		scroll_bar.x = rect.x + content_size;
		scroll_bar.y = rect.y;
		scroll_bar.w = SCROLL_BAR_THICKNESS;
		scroll_bar.h = rect.h;

		scroll_bar_button_size = scroll_bar.h / view->children.length;
	}

	Point mouse_pos = mouse_position(env);
	bool is_mouse_over = inside_rect(mouse_pos, scroll_bar);

	if (is_mouse_over && env->mouse_left_down)
	{
		if (scroll_view->axis == DIRECTION_HORIZONTAL)
		{
			float scroll = (env->mouse_x - rect.x) / scroll_bar.w;
			for (size_t i = 0; i < scroll_view->base.children.length; i++)
			{
				View* child = scroll_view->base.children.items[i];
				child->rect.x += (scroll_view->scroll - scroll) * (total_scroll - rect.w + item_size);
			}
			scroll_view->scroll = scroll;
		}
		else
		{
			float scroll = (env->mouse_y - rect.y) / scroll_bar.h;
			for (size_t i = 0; i < scroll_view->base.children.length; i++)
			{
				View* child = scroll_view->base.children.items[i];
				child->rect.y += (scroll_view->scroll - scroll) * (total_scroll - rect.h + item_size);
			}
			scroll_view->scroll = scroll;
		}
	}

	Image image = image_from_env(env);
	draw_rect(image, scroll_bar, (Color)
	{
		.rgba=0XFFEEEEEE
	});

	Rect scroll_bar_button;
	if (scroll_view->axis == DIRECTION_HORIZONTAL)
	{
		scroll_bar_button.x = rect.x + fminf(scroll_view->scroll*scroll_bar.w, rect.w - scroll_bar_button_size);
		scroll_bar_button.y = rect.y + content_size;
		scroll_bar_button.w = scroll_bar_button_size;
		scroll_bar_button.h = SCROLL_BAR_THICKNESS;
	}
	else
	{
		scroll_bar_button.x = rect.x + content_size;
		scroll_bar_button.y = rect.y + fminf(scroll_view->scroll*scroll_bar.h, rect.h - scroll_bar_button_size);
		scroll_bar_button.w = SCROLL_BAR_THICKNESS;
		scroll_bar_button.h = scroll_bar_button_size;
	}

	draw_rect(image, scroll_bar_button, (Color)
	{
		.rgba=0XFF686D76
	});
}

ScrollView* new_scroll_view(Rect rect, Axis axis)
{
	ScrollView* scroll_view = malloc(sizeof(ScrollView));
	memset(scroll_view, 0, sizeof(ScrollView));
	scroll_view->base.rect = rect;
	scroll_view->base.draw = draw_scroll_view;
	scroll_view->axis = axis;
	return scroll_view;
}

void draw_rectangle_view(View* view, Rect rect, Env *env)
{
	RectView* rect_view = (RectView*) view;
	Image image = image_from_env(env);
	draw_rect(image, rect, rect_view->color);
}

RectView* new_rect_view(Rect rect, Color color)
{
	RectView* rect_view = malloc(sizeof(RectView));
	memset(rect_view, 0, sizeof(RectView));
	rect_view->base.rect = rect;
	rect_view->color = color;
	rect_view->base.draw = draw_rectangle_view;
	return rect_view;
}

void draw_text_view(View* view, Rect rect, Env *env)
{
	TextView* text_view = (TextView*) view;
	Image image = image_from_env(env);

	draw_text(
	    image,
	    text_view->font,
	    text_view->text,
	    text_view->text_size,
	    (Point)
		{
			.x = rect.x, .y = rect.y
		},
		text_view->text_color
	);

}

TextView* new_text_view(Point pos, Font font, const char *text, Color text_color, int size)
{
	TextView* text_view = malloc(sizeof(TextView));
	memset(text_view, 0, sizeof(TextView));
	Point text_size = measure_text(font, text, size);
	text_view->base.rect = (Rect)
	{
		.x = pos.x,
		.y = pos.y,
		.w = text_size.x,
		.h = text_size.y,
	};
	text_view->font = font;
	text_view->text = text;
	text_view->text_size = size;
	text_view->text_color = text_color;
	text_view->base.draw = draw_text_view;

	return text_view;
}

void draw_panel_view(View* view, Rect rect, Env *env)
{
	PanelView* panel_view = (PanelView*) view;
	Image image = image_from_env(env);

	Color color;
	const bool is_mouse_over = inside_rect(mouse_position(env), rect);
	if (is_mouse_over)
	{
		color = panel_view->active_color;
	}
	else
	{
		color = panel_view->background_color;
	}

	draw_rounded_rect(image, rect, color, panel_view->border_radius);
}

PanelView* new_panel_view(Rect rect, Color background_color, Color active_color, float border_radius)
{
	PanelView* panel_view = malloc(sizeof(PanelView));
	memset(panel_view, 0, sizeof(PanelView));
	panel_view->base.rect = rect;
	panel_view->background_color = background_color;
	panel_view->active_color = active_color;
	panel_view->border_radius = border_radius;
	panel_view->base.draw = draw_panel_view;
	return panel_view;
}
