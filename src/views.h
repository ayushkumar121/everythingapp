#pragma once

#include "env.h"
#include "drawing.h"
#include "basic.h"

typedef struct View	View;
typedef ARRAY(View*) Views;

typedef void (*DrawFn)(View* view, Rect rect, Env *env);

typedef struct View
{
	Rect rect;
	Point offset;
	Views children;
	DrawFn draw;
} View;

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
} ScrollView;

ScrollView* new_scroll_view(Rect rect, Axis axis);

typedef struct
{
	View base;
	Color color;
} RectView;

RectView* new_rect_view(Rect rect, Color color);

typedef struct
{
	View base;
	Font font;
	const char *text;
	Color text_color;
	int text_size;
} TextView;

TextView* new_text_view(Point pos, Font font, const char *text, Color text_color, int size);

typedef struct
{
	View base;
	Color background_color;
	Color active_color;
	float border_radius;
} PanelView;

PanelView* new_panel_view(Rect rect, Color background_color, Color active_color, float border_radius);
