#include "env.h"
#include "basic.h"
#include "drawing.h"
#include "hotreload.h"
#include "views.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	Image background_image;
	Font font;
	View* view;
	int width;
	int height;
} AppState;

AppState *state = NULL;

export void app_load(void)
{
	state = malloc(sizeof(AppState));
	memset(state, 0, sizeof(AppState));

	load_image(&state->background_image, "assets/sample3.bmp");
	load_font(&state->font, "assets/spleen-16x32.bdf");
}

export void app_init(Env* env)
{
	state->width = env->width;
	state->height = env->height;

	if (state->view != NULL)
	{
		destroy_view(state->view);
	}
	
	ScrollView* scroll_view = new_scroll_view(&(ScrollViewArgs){
		.base = (ViewArgs){
			.rect = (Vec4) {
				.x = 0,
				.y = 30,
				.w = env->width/3,
				.h = env->height - 30,
			},
		},
		.axis = DIRECTION_VERTICAL,
	});

	for (int i = 0; i < 20; i++)
	{
		PanelView* panel_view = new_panel_view(&(PanelViewArgs){
			.base = (ViewArgs){
				.rect = (Vec4) {
					.x = 10,
					.y = 60*i + 20,
					.w = env->width/3-30,
					.h = 50,
				},
			},
			.background_color = (Color){.rgba = 0x60BB9AB1},
			.active_color = (Color){.rgba = 0x60BBBBBB},
			.border_radius = 8.0f,
		});

		TextView* text = new_text_view(&(TextViewArgs){
			.base = (ViewArgs){
				.rect = (Vec4) {
					.x = 10,
					.y = 10,
					.w = env->width/3,
					.h = 50,
				},
			},
			.font = state->font,
			.text = i%2 == 0? "ODD" : "EVEN",
			.text_color = COLOR_BLACK,
			.text_size = 32,
		});

		array_append(&panel_view->base.children, (View*)text);
		array_append(&scroll_view->base.children, (View*)panel_view);
	}
	state->view = (View*)scroll_view;
}

export void app_update(Env *env)
{
	if (state->width != env->width || state->height != env->height)
	{
		state->width = env->width;
		state->height = env->height;
		app_init(env);
	}

	Image image = image_from_env(env);
	clear_image(image, COLOR_WHITE);
	draw_image(image, state->background_image, (Vec4){
		.x = 0,
		.y = 0,
		.w = env->width,
		.h = env->height,
	}, NULL);
	draw_view(state->view, env);

	char fps[32];
	snprintf(fps, 32, "FPS: %.2f", 1/env->delta_time);
	draw_text(image, state->font, fps, 32, (Vec2){.x = env->width-200, .y = 50}, COLOR_GREEN);
}

export AppStateHandle app_pre_reload(void)
{
	return (AppStateHandle) {
		.state = state,
		.size = sizeof(AppState)
	};
}

export void app_post_reload(AppStateHandle handle)
{
	state = malloc(sizeof(AppState));
	memcpy(state, handle.state, handle.size);
	free(handle.state);
}
