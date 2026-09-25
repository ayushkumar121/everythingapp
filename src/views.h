#pragma once

#include "env.h"
#include "drawing.h"
#include "basic.h"

typedef struct View	View;
typedef ARRAY(View*) Views;

typedef void (*DrawFn)(View* view, Vec4 rect, Vec4 clip, Env *env);

typedef struct View
{
	Vec4 rect;
	Vec2 offset;
	Vec2 content_offset;
	Vec4* padding;
	Views children;
	DrawFn draw;
} View;

typedef struct
{
	Vec4 rect;
} ViewArgs;

void draw_view(View* view, Env *env);
void destroy_view(View* view);

typedef enum
{
	DIRECTION_HORIZONTAL,
	DIRECTION_VERTICAL,
} Axis;

typedef struct
{
	View base;
	float scroll; // pixels along axis
	Axis axis;
	bool is_dragging;
	float bar_thickness;
	float border_radius;
	Color background_color;
	Color track_color;
	Color thumb_color;
} ScrollView;

typedef struct
{
	ViewArgs base;
	Axis axis;
	float bar_thickness;
	float border_radius;
	Color background_color;
	Color track_color;
	Color thumb_color;
} ScrollViewArgs;

ScrollView* new_scroll_view(ScrollViewArgs* args);

typedef struct
{
	View base;
	Color color;
} RectView;

typedef struct
{
	ViewArgs base;
	Color color;
} RectViewArgs;

RectView* new_rect_view(RectViewArgs* args);

typedef struct
{
	View base;
	Font font;
	const char *text;
	Color text_color;
	int text_size;
} TextView;

typedef struct
{
	ViewArgs base;
	Color color;
	Font font;
	const char *text;
	int text_size;
	Color text_color;
} TextViewArgs;

TextView* new_text_view(TextViewArgs* args);

typedef struct
{
	View base;
	Color background_color;
	Color active_color;
	float border_radius;
} PanelView;

typedef struct
{
	ViewArgs base;
	Color background_color;
	Color active_color;
	float border_radius;
} PanelViewArgs;

PanelView* new_panel_view(PanelViewArgs* args);

// Runs while the tree is being drawn, so it must not destroy views
typedef void (*ClickFn)(View* view, void* user_data);

typedef struct
{
	View base;
	Font font;
	const char *text;
	int text_size;
	Color text_color;
	Color background_color;
	Color hover_color;
	Color pressed_color;
	float border_radius;
	ClickFn on_click;
	void* user_data;
	bool is_pressed;
	bool mouse_was_down;
} ButtonView;

typedef struct
{
	ViewArgs base;
	Font font;
	const char *text;
	int text_size; // 0 uses the font's native size
	Color text_color;
	Color background_color;
	Color hover_color;
	Color pressed_color;
	float border_radius;
	ClickFn on_click;
	void* user_data;
} ButtonViewArgs;

ButtonView* new_button_view(ButtonViewArgs* args);

// A button that shows an image instead of a label. The image is borrowed, not freed by the view.
typedef struct
{
	ButtonView base;
	Image image;
	float image_padding;
} ImageButtonView;

typedef struct
{
	ViewArgs base;
	Image image;
	float image_padding;
	Color background_color;
	Color hover_color;
	Color pressed_color;
	float border_radius;
	ClickFn on_click;
	void* user_data;
} ImageButtonViewArgs;

ImageButtonView* new_image_button_view(ImageButtonViewArgs* args);
