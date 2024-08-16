#include "env.h"
#include "drawing.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "hotreload.h"

typedef struct
{
	Image background_image;
	Font font;
	Color foreground_color;
} AppState;

AppState *state = NULL;

void app_init(void)
{
	state = malloc(sizeof(AppState));
	state->foreground_color = MAGENTA;

	load_image(&state->background_image, "assets/sample3.bmp");
	load_font(&state->font, "assets/spleen-16x32.bdf");
}

void app_update(Env *env)
{
	Image window = image_from_env(env);

	clear_image(window, WHITE);

	// Drawing background image
	{
		ImageArgs image_args =
		{
			.image = &state->background_image,
			.rect = (Rect)
			{
				.x = 0,
				.y = 0,
				.w = window.width,
				.h = window.height,
			},
			.crop = NULL,
		};
		draw_image(window, &image_args);
	}

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
			.background_color = (Color){.rgba=0xAAEFEFEF},
			.border_radius = 8.0f,
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
				.text = button_text[i],
				.font = &state->font,
				.font_size = 24,
			};
			if (button(env, &args))
			{
				state->foreground_color = button_colors[i];
			}
		}
	}

	// Drawing text
	{
		char* text = "Hello, World!";
		Point text_size = measure_text(&state->font, text, 52);
		TextArgs args =
		{
			.font = &state->font,
			.text = text,
			.size = 52,
			.position = (Point){
				.x=env->width/3.0 + env->width/3.0 - text_size.x/2, 
				.y=env->height/2.0- text_size.y/2
			},
			.color = state->foreground_color,
		};
		draw_text(window, &args);
	}

	{
		float fps = 1/(env->delta_time);
		char text[100];
		sprintf(text, "FPS: %.2f", roundf(fps));
		Point text_size = measure_text(&state->font, text, 52);

		TextArgs args =
		{
			.font = &state->font,
			.text = text,
			.size = 28,
			.position = (Point){
				.x=window.width-text_size.x/2.0f, 
				.y=50.0f,
			},
			.color = state->foreground_color,
		};
		draw_text(window, &args);

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
