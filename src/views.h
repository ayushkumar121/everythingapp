#pragma once

#include "env.h"
#include "drawing.h"
#include "basic.h"

typedef struct View	View;
typedef ARRAY(View*) Views;

typedef void (*DrawFn)(View* view, Vec4 rect, Env *env);

typedef struct View
{
	Vec4 rect;
	Vec2 offset;
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
	float scroll;
	Axis axis;
	bool is_dragging;
} ScrollView;

typedef struct
{
	ViewArgs base;
	Axis axis;
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
