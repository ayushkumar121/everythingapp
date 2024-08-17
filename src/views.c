#include "basic.h"
#include "views.h"

void draw_view(View* view, Env *env)
{
	assert(view != NULL);
	assert(env != NULL);

	Rect rect = rect_add_point(view->rect, view->offset);
	if (view->draw != NULL)
	{
		view->draw(view, rect, env);
	}

	Env canvas_env = new_env(env, rect.w, rect.h);
	Image canvas = image_from_env(&canvas_env);

	for (size_t i=0; i < view->children.length; i++)
	{
		View* child = view->children.items[i];
		draw_view(child, &canvas_env);
	}

	draw_image(image_from_env(env), canvas, rect, NULL);
	free(canvas_env.buffer);
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

	float k = 0;
	for (size_t i = 0; i < scroll_view->base.children.length; i++)
	{
		View* child = scroll_view->base.children.items[i];

		if (scroll_view->axis == DIRECTION_HORIZONTAL)
		{
			k = fmaxf(k, child->rect.h);
		}
		else
		{
			k = fmaxf(k, child->rect.w);
		}
	}

	Image image = image_from_env(env);

	Rect scroll_bar;
	if (scroll_view->axis == DIRECTION_HORIZONTAL)
	{
		scroll_bar.x = rect.x;
		scroll_bar.y = rect.y + k;
		scroll_bar.w = rect.w;
		scroll_bar.h = SCROLL_BAR_THICKNESS;
	}
	else
	{
		scroll_bar.x = rect.x + k;
		scroll_bar.y = rect.y;
		scroll_bar.w = SCROLL_BAR_THICKNESS;
		scroll_bar.h = rect.h;
	}

	draw_rect(image, scroll_bar, (Color){.rgba=0XFFEEEEEE});

	Point mouse_pos = (Point) {
		.x = env->mouse_x,
		.y = env->mouse_y,
	};

	bool is_mouse_over = inside_rect(mouse_pos, scroll_bar);
	if (is_mouse_over && env->mouse_left_down)
	{
		if (scroll_view->axis == DIRECTION_HORIZONTAL)
		{
			scroll_view->scroll = env->mouse_x - rect.x;
			for (size_t i = 0; i < scroll_view->base.children.length; i++) {
				View* child = scroll_view->base.children.items[i];
				child->offset.x = -scroll_view->scroll;
			}
		}
		else
		{
			scroll_view->scroll = env->mouse_y - rect.y;
			for (size_t i = 0; i < scroll_view->base.children.length; i++) {
				View* child = scroll_view->base.children.items[i];
				child->offset.y = -scroll_view->scroll;
			}
		}
	}

	Rect scroll_bar_button;
	if (scroll_view->axis == DIRECTION_HORIZONTAL)
	{
		int scroll_bar_button_width = rect.w / view->children.length;
		scroll_bar_button.x = rect.x + fminf(scroll_view->scroll, rect.w - scroll_bar_button_width),
		scroll_bar_button.y = rect.y + k;
		scroll_bar_button.w = scroll_bar_button_width;
		scroll_bar_button.h = SCROLL_BAR_THICKNESS;
	}
	else
	{
		int scroll_bar_button_height = rect.h / view->children.length;
		scroll_bar_button.x = rect.x + k;
		scroll_bar_button.y = rect.y + fminf(scroll_view->scroll, rect.w - scroll_bar_button_height);
		scroll_bar_button.w = SCROLL_BAR_THICKNESS;
		scroll_bar_button.h = scroll_bar_button_height;
	}

	draw_rect(image, scroll_bar_button, (Color){.rgba=0XFF686D76});
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
		(Point) {.x = rect.x, .y = rect.y},
		text_view->text_color
	);
}

TextView* new_text_view(Point pos, Font font, const char *text, Color text_color, int size)
{
	TextView* text_view = malloc(sizeof(TextView));
	memset(text_view, 0, sizeof(TextView));
	Point text_size = measure_text(font, text, size);
	text_view->base.rect = (Rect) {
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