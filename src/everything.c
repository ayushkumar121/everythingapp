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
	ScrollView* scroll = new_scroll_view((Rect){
		.x = 10,
		.y = 50,
		.w = env->width,
		.h = env->height-50
	}, DIRECTION_VERTICAL);

	for (int i = 0; i < 20; i++)
	{
		RectView* rect = new_rect_view((Rect){
			.x = 0,
			.y = i*60,
			.w = 300,
			.h = 50
		}, i%2==0? MAGENTA : GREEN);

		TextView* text = new_text_view((Point){
			.x = 10, .y = 10,
		}, state->font, i%2==0? "MAGENTA":"GREEN", WHITE, 32);

		array_append(&rect->base.children, (View*)text);
		array_append(&scroll->base.children, (View*)rect);
	}
	state->view = (View*)scroll;
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
	return (AppStateHandle){
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
