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

	Vec2 child_offset = { .x = rect.x - view->content_offset.x, .y = rect.y - view->content_offset.y };
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
#define SCROLL_THUMB_MIN_SIZE 20

static float scroll_content_size(ScrollView* scroll_view)
{
	float size = 0.0f;
	for (size_t i = 0; i < scroll_view->base.children.length; i++)
	{
		Vec4 child = scroll_view->base.children.items[i]->rect;
		if (scroll_view->axis == DIRECTION_HORIZONTAL)
		{
			size = fmaxf(size, child.x + child.w);
		}
		else
		{
			size = fmaxf(size, child.y + child.h);
		}
	}
	return size;
}

void draw_scroll_view(View* view, Vec4 rect, Vec4 clip, Env *env)
{
	ScrollView* scroll_view = (ScrollView*) view;
	Image image = image_from_env(env);
	const bool horizontal = scroll_view->axis == DIRECTION_HORIZONTAL;

	const float viewport = horizontal ? rect.w : rect.h;
	const float content = scroll_content_size(scroll_view);
	const float max_scroll = fmaxf(content - viewport, 0.0f);

	Vec4 scroll_bar;
	if (horizontal)
	{
		scroll_bar = (Vec4){ .x = rect.x, .y = rect.y + rect.h - SCROLL_BAR_THICKNESS, .w = rect.w, .h = SCROLL_BAR_THICKNESS };
	}
	else
	{
		scroll_bar = (Vec4){ .x = rect.x + rect.w - SCROLL_BAR_THICKNESS, .y = rect.y, .w = SCROLL_BAR_THICKNESS, .h = rect.h };
	}

	const float track = horizontal ? scroll_bar.w : scroll_bar.h;
	float thumb_size = track;
	if (content > viewport)
	{
		thumb_size = fmaxf(track * viewport / content, fminf(SCROLL_THUMB_MIN_SIZE, track));
	}

	Vec2 mouse_pos = mouse_position(env);
	if (inside_rect(mouse_pos, clip))
	{
		scroll_view->scroll -= horizontal ? env->scroll_x : env->scroll_y;
	}

	if (!env->mouse_left_down)
	{
		scroll_view->is_dragging = false;
	}
	else if (inside_rect(mouse_pos, scroll_bar) && inside_rect(mouse_pos, clip))
	{
		scroll_view->is_dragging = true;
	}

	if (scroll_view->is_dragging && track > thumb_size)
	{
		// Centre the thumb on the mouse
		float mouse = horizontal ? mouse_pos.x - scroll_bar.x : mouse_pos.y - scroll_bar.y;
		scroll_view->scroll = (mouse - thumb_size / 2) / (track - thumb_size) * max_scroll;
	}

	scroll_view->scroll = clamp(scroll_view->scroll, 0.0f, max_scroll);
	if (horizontal)
	{
		view->content_offset = (Vec2){ .x = scroll_view->scroll };
	}
	else
	{
		view->content_offset = (Vec2){ .y = scroll_view->scroll };
	}

	draw_rect(image, rect, 0x50AAAAAAu, &clip);
	if (max_scroll <= 0.0f) return;

	draw_rect(image, scroll_bar, 0x60EEEEEEu, &clip);

	float thumb_pos = scroll_view->scroll / max_scroll * (track - thumb_size);
	Vec4 thumb = scroll_bar;
	if (horizontal)
	{
		thumb.x += thumb_pos;
		thumb.w = thumb_size;
	}
	else
	{
		thumb.y += thumb_pos;
		thumb.h = thumb_size;
	}
	draw_rect(image, thumb, COLOR_RED, &clip);
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
