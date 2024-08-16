#include "env.h"
#include "drawing.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "hotreload.h"

typedef struct
{
	Image lena;
	Font font;
	Color clear_color;
} AppState;

AppState *state = NULL;

void app_init(void)
{
	state = malloc(sizeof(AppState));
	state->clear_color = WHITE;

	load_image(&state->lena, "assets/lena.bmp");
	load_font(&state->font, "assets/ter-u18n.bdf");
}

void app_update(Env *env)
{
	Image window = image_from_env(env);

	clear_image(window, state->clear_color);

	// Drawing side panel
	{
		PanelArgs args =
		{
			.rect = {
				.x = 0,
				.y = 0,
				.w = env->width/3.0,
				.h = env->height,
			},
			.background_color = (Color){.rgba=0xEEEFEFEF},
			.border_radius = 8.0f,
			.border_color = (Color){.rgba=0x66222222},
		};
		panel(env, &args);
	}

	// Drawing boxes
	{
		int box_height = 50;
		int gap = 10;
		int margin_top = 50;
		int margin_sides = 20;

		Color button_colors[] =
		{
			MAGENTA,
			RED,
			GREEN,
		};

		char* button_text[] =
		{
			"MAGENTA",
			"RED",
			"GREEN",
		};

		for (int i = 0; i < 3; ++i)
		{
			ButtonArgs args =
			{
				.rect = {
					.x = 10,
					.y = margin_top+(box_height+gap)*i,
					.w = env->width/3.0-margin_sides,
					.h = box_height,
				},
				.background_color = (Color){.rgba=0x22222222},
				.active_color = (Color){.rgba=0xEE222222},
				.hover_color = (Color){.rgba=0x66222222},
				.foreground_color = (Color){.rgba=0x66222222},
				.border_radius = 8.0f,
				.border_color = (Color){.rgba=0x66222222},
				.text = button_text[i],
				.font = &state->font,
				.font_size = 24,
			};
			if (button(env, &args))
			{
				state->clear_color = button_colors[i];
			}

		}
	}
}

// For hot reloading
AppStateHandle app_pre_reload(void)
{
	return (AppStateHandle)
	{
		.state=state, .size=sizeof(AppState)
	};
}

void app_post_reload(AppStateHandle handle)
{
	state = malloc(sizeof(AppState));
	memcpy(state, handle.state, handle.size);
	free(handle.state);
}
