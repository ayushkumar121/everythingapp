#include "basic.h"
#include "views.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

void new_view(View* view, const ViewArgs* args)
{
	view->rect = args->rect;
}

Vec2 mouse_position(Env* env)
{
	return (Vec2)
	{
		.x = env->mouse_x,
		.y = env->mouse_y
	};
}

static void draw_view_clipped(View* view, Vec4 clip, Env *env)
{
	Vec4 rect = v4_add_v2(view->rect, view->offset);
	clip = rect_intersect(clip, rect);
	if (clip.w <= 0 || clip.h <= 0) return;

	if (view->draw != NULL)
	{
		view->draw(view, rect, clip, env);
	}

	Vec2 child_offset = { .x = rect.x, .y = rect.y };
	for (size_t i=0; i < view->children.length; i++)
	{
		View* child = view->children.items[i];
		child->offset = child_offset;
		draw_view_clipped(child, clip, env);
	}
}

void draw_view(View* view, Env *env)
{
	assert(view != NULL);
	assert(env != NULL);

	Vec4 window = { .x = 0, .y = 0, .w = env->width, .h = env->height };
	draw_view_clipped(view, window, env);
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
	view = NULL;
}

#define SCROLL_BAR_THICKNESS 10

void draw_scroll_view(View* view, Vec4 rect, Vec4 clip, Env *env)
{
	ScrollView* scroll_view = (ScrollView*) view;
	Image image = image_from_env(env);

	float total_scroll = 0.0f;
	float item_size = 0.0f;
	for (size_t i = 0; i < scroll_view->base.children.length; i++)
	{
		View* child = scroll_view->base.children.items[i];
		if (scroll_view->axis == DIRECTION_HORIZONTAL)
		{
			total_scroll += child->rect.w;
			item_size = fmaxf(item_size, child->rect.w);
		}
		else
		{
			total_scroll += child->rect.h;
			item_size = fmaxf(item_size, child->rect.h);
		}
	}

	draw_rect(image, rect, 0x50AAAAAAu, &clip);

	Vec4 scroll_bar;
	int scroll_bar_button_size;
	if (scroll_view->axis == DIRECTION_HORIZONTAL)
	{
		scroll_bar.x = rect.x;
		scroll_bar.y = rect.y + rect.h - SCROLL_BAR_THICKNESS;
		scroll_bar.w = rect.w;
		scroll_bar.h = SCROLL_BAR_THICKNESS;

		scroll_bar_button_size = scroll_bar.w / view->children.length;
	}
	else
	{
		scroll_bar.x = rect.x + rect.w - SCROLL_BAR_THICKNESS;
		scroll_bar.y = rect.y;
		scroll_bar.w = SCROLL_BAR_THICKNESS;
		scroll_bar.h = rect.h;

		scroll_bar_button_size = scroll_bar.h / view->children.length;
	}

	draw_rect(image, scroll_bar, 0x60EEEEEEu, &clip);

	Vec4 scroll_bar_button;
	if (scroll_view->axis == DIRECTION_HORIZONTAL)
	{
		scroll_bar_button.x = rect.x + clamp(scroll_view->scroll*scroll_bar.w, 0, rect.w - scroll_bar_button_size);
		scroll_bar_button.y = rect.y + rect.h - SCROLL_BAR_THICKNESS;
		scroll_bar_button.w = scroll_bar_button_size;
		scroll_bar_button.h = SCROLL_BAR_THICKNESS;
	}
	else
	{
		scroll_bar_button.x = rect.x + rect.w - SCROLL_BAR_THICKNESS;
		scroll_bar_button.y = rect.y + clamp(scroll_view->scroll*scroll_bar.h, 0, rect.h - scroll_bar_button_size);
		scroll_bar_button.w = SCROLL_BAR_THICKNESS;
		scroll_bar_button.h = scroll_bar_button_size;
	}

	Color scroll_bar_color = COLOR_RED;
	Vec2 mouse_pos = mouse_position(env);

	if (inside_rect(mouse_pos, scroll_bar) && env->mouse_left_down)
	{
		if (scroll_view->axis == DIRECTION_HORIZONTAL)
		{
			float scroll = (env->mouse_x - rect.x - scroll_bar_button_size) / scroll_bar.w;
			for (size_t i = 0; i < scroll_view->base.children.length; i++)
			{
				View* child = scroll_view->base.children.items[i];
				child->rect.x += (scroll_view->scroll - scroll) * (total_scroll - rect.w + item_size);
			}
			scroll_view->scroll = scroll;
		}
		else
		{
			float scroll = (env->mouse_y - rect.y - scroll_bar_button_size) / scroll_bar.h;
			for (size_t i = 0; i < scroll_view->base.children.length; i++)
			{
				View* child = scroll_view->base.children.items[i];
				child->rect.y += (scroll_view->scroll - scroll) * (total_scroll - rect.h + item_size);
			}
			scroll_view->scroll = scroll;
		}
	}

	draw_rect(image, scroll_bar_button, scroll_bar_color, &clip);
}

ScrollView* new_scroll_view(ScrollViewArgs* args)
{
	assert(args != NULL);
	ScrollView* view = malloc(sizeof(ScrollView));
	memset(view, 0, sizeof(ScrollView));
	new_view((View*)view, (ViewArgs*)args);
	view->base.draw = draw_scroll_view;
	view->axis = args->axis;
	return view;
}

void draw_rectangle_view(View* view, Vec4 rect, Vec4 clip, Env *env)
{
	RectView* rect_view = (RectView*) view;
	Image image = image_from_env(env);
	draw_rect(image, rect, rect_view->color, &clip);
}

RectView* new_rect_view(RectViewArgs* args)
{
	assert(args != NULL);
	RectView* view = malloc(sizeof(RectView));
	memset(view, 0, sizeof(RectView));
	new_view((View*)view, (ViewArgs*)args);
	view->base.draw = draw_rectangle_view;
	view->color = args->color;
	return view;
}

void draw_text_view(View* view, Vec4 rect, Vec4 clip, Env *env)
{
	TextView* text_view = (TextView*) view;
	Image image = image_from_env(env);

	draw_text(
	    image,
	    text_view->font,
	    text_view->text,
	    text_view->text_size,
	    (Vec2)
		{
			.x = rect.x, .y = rect.y
		},
		text_view->text_color,
		&clip
	);

}

TextView* new_text_view(TextViewArgs* args)
{
	assert(args != NULL);
	TextView* view = malloc(sizeof(TextView));
	memset(view, 0, sizeof(TextView));
	new_view((View*)view, (ViewArgs*)args);
	view->base.draw = draw_text_view;
	view->font = args->font;
	view->text = args->text;
	view->text_size = args->text_size;
	view->text_color = args->text_color;
	return view;
}

void draw_panel_view(View* view, Vec4 rect, Vec4 clip, Env *env)
{
	PanelView* panel_view = (PanelView*) view;
	Image image = image_from_env(env);

	Color color;
	const bool is_mouse_over = inside_rect(mouse_position(env), clip);
	if (is_mouse_over)
	{
		color = panel_view->active_color;
	}
	else
	{
		color = panel_view->background_color;
	}

	draw_rounded_rect(image, rect, color, panel_view->border_radius, &clip);
}

PanelView* new_panel_view(PanelViewArgs* args)
{
	assert(args != NULL);
	PanelView* view = malloc(sizeof(PanelView));
	memset(view, 0, sizeof(PanelView));
	new_view((View*)view, (ViewArgs*)args);
	view->base.draw = draw_panel_view;
	view->background_color = args->background_color;
	view->active_color = args->active_color;
	view->border_radius = args->border_radius;
	return view;
}
