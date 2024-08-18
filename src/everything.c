#include "env.h"
#include "basic.h"
#include "drawing.h"
#include "hotreload.h"
#include "views.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

typedef struct
{
	Image background_image;
	Font font;
	View* view;
} AppState;

AppState *state = NULL;

void app_load(void)
{
	state = malloc(sizeof(AppState));

	load_image(&state->background_image, "assets/sample3.bmp");
	load_font(&state->font, "assets/spleen-16x32.bdf");
}

void app_init(Env* env)
{
	ScrollView* scroll_view = new_scroll_view(
		(Rect){
			.x = 200,
			.y = 200,
			.w = env->width,
			.h = env->height/2
		},
	DIRECTION_VERTICAL);

	for (int i = 0; i < 20; i++)
	{
		PanelView* panel_view = new_panel_view(
			(Rect){
			.x = 10,
			.y = i*210+10,
			.w = 300,
			.h = 50
		},
		(Color){.rgba=0XFFEEEEEE},
		(Color) {.rgba=0xFF6482AD},
		8.0f
		);

		TextView* text = new_text_view((Point){
			.x = 10, .y = 10,
		}, state->font, i%2==0? "EVEN":"ODD", GREEN, 32);

		array_append(&panel_view->base.children, (View*)text);
		array_append(&scroll_view->base.children, (View*)panel_view);
	}
	state->view = (View*)scroll_view;
}

void app_update(Env *env)
{
	Image image = image_from_env(env);
	clear_image(image, WHITE);
	draw_view(state->view, env);
}

// For hot reloading
AppStateHandle app_pre_reload(void)
{
	return (AppStateHandle) {
		.state = state,
		.size = sizeof(AppState)
	};
}

void app_post_reload(AppStateHandle handle)
{
	state = malloc(sizeof(AppState));
	memcpy(state, handle.state, handle.size);
	free(handle.state);
}
