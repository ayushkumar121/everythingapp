#include "env.h"
#define BASIC_IMPLEMENTATION
#include "basic.h"
#include "drawing.h"
#include "hotreload.h"
#include "views.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define THEME_BACKGROUND     0xFF2E2E2Eu
#define THEME_SIDEBAR        0xFF1C1C1Cu
#define THEME_ITEM           0xFF262626u
#define THEME_ITEM_HOVER     0xFF333333u
#define THEME_ITEM_SELECTED  0xFF4A4A4Au
#define THEME_CARD           0xFF242424u
#define THEME_TEXT           0xFFE6E6E6u
#define THEME_TEXT_MUTED     0xFFB0B0B0u
#define THEME_TEXT_FAINT     0xFF6E6E6Eu
#define THEME_SCROLL_TRACK   0xFF242424u
#define THEME_SCROLL_THUMB   0xFF555555u

// Layout sizes are in points, multiplied by the display scale
#define PADDING 10
#define ITEM_HEIGHT 50
#define SCROLL_BAR_THICKNESS 6
#define SCROLL_BAR_SPACE 20

typedef struct
{
	const char *title;
	const char *body;
} Section;

static const Section sections[] =
{
	{"Today", "Nothing scheduled yet.\n\nPlan the day by adding tasks,\nevents and habits to check off."},
	{"Tasks", "Your to-do list.\n\nCapture anything you need to do\nand tick it off when it's done."},
	{"Calendar", "Upcoming events and deadlines.\n\nSee your week at a glance."},
	{"Notes", "Quick notes and ideas.\n\nWrite things down before\nyou forget them."},
	{"Habits", "Daily habits and streaks.\n\nTrack what you want to do\nevery day."},
	{"Goals", "Long term goals.\n\nBreak big goals into small steps\nand follow your progress."},
	{"Journal", "A private daily journal.\n\nReflect on how the day went."},
	{"Reading", "Books to read and books read.\n\nKeep a list and short reviews."},
	{"Fitness", "Workouts and activity.\n\nLog exercise and see trends\nover time."},
	{"Finance", "Budget and spending.\n\nTrack where your money goes\neach month."},
	{"Contacts", "People you keep in touch with.\n\nBirthdays, notes and\nwhen you last spoke."},
	{"Settings", "Preferences for the app.\n\nTheme, data and shortcuts."},
};

#define SECTION_COUNT ((int)countof(sections))

typedef struct
{
	Font font_regular;
	Font font_small;
	Font font_bold;
	View* view;
	int width;
	int height;
	float scale;
	ScrollView* sidebar;
	PanelView* items[countof(sections)];
	TextView* detail_title;
	TextView* detail_body;
	int selected;
	bool mouse_was_down;
} AppState;

AppState *state = NULL;

static void select_section(int index);

export void app_load(void)
{
	state = malloc(sizeof(AppState));
	memset(state, 0, sizeof(AppState));

	// Bitmap fonts are only drawn at their native pixel size to stay sharp
	load_font(&state->font_regular, "assets/spleen-16x32.bdf");
	load_font(&state->font_small, "assets/ter-u18n.bdf");
	load_font(&state->font_bold, "assets/ter-u32b.bdf");
}

export void app_init(Env* env)
{
	state->width = env->width;
	state->height = env->height;
	state->scale = env->scale;

	float s = env->scale > 0 ? env->scale : 1.0f;
	Font body_font = s >= 1.5f ? state->font_regular : state->font_small;
	Font title_font = state->font_bold;
	int body_size = font_size(body_font);
	int title_size = font_size(title_font);

	if (state->view != NULL)
	{
		destroy_view(state->view);
	}

	RectView* root = new_rect_view(&(RectViewArgs){
		.base = (ViewArgs){
			.rect = (Vec4){ .x = 0, .y = 0, .w = env->width, .h = env->height },
		},
		.color = THEME_BACKGROUND,
	});

	float sidebar_width = env->width / 3;

	ScrollView* sidebar = new_scroll_view(&(ScrollViewArgs){
		.base = (ViewArgs){
			.rect = (Vec4){ .x = 0, .y = 0, .w = sidebar_width, .h = env->height },
		},
		.axis = DIRECTION_VERTICAL,
		.bar_thickness = SCROLL_BAR_THICKNESS * s,
		.background_color = THEME_SIDEBAR,
		.track_color = THEME_SCROLL_TRACK,
		.thumb_color = THEME_SCROLL_THUMB,
	});

	for (int i = 0; i < SECTION_COUNT; i++)
	{
		PanelView* item = new_panel_view(&(PanelViewArgs){
			.base = (ViewArgs){
				.rect = (Vec4){
					.x = PADDING * s,
					.y = (PADDING + i * (ITEM_HEIGHT + PADDING)) * s,
					.w = sidebar_width - (PADDING + SCROLL_BAR_SPACE) * s,
					.h = ITEM_HEIGHT * s,
				},
			},
			.border_radius = 8.0f * s,
		});

		TextView* label = new_text_view(&(TextViewArgs){
			.base = (ViewArgs){
				.rect = (Vec4){ .x = 16 * s, .y = (int)(ITEM_HEIGHT * s - body_size) / 2, .w = sidebar_width, .h = body_size },
			},
			.font = body_font,
			.text = sections[i].title,
			.text_color = THEME_TEXT,
			.text_size = body_size,
		});

		array_append(&item->base.children, (View*)label);
		array_append(&sidebar->base.children, (View*)item);
		state->items[i] = item;
	}

	PanelView* detail = new_panel_view(&(PanelViewArgs){
		.base = (ViewArgs){
			.rect = (Vec4){
				.x = sidebar_width + 2 * PADDING * s,
				.y = 2 * PADDING * s,
				.w = env->width - sidebar_width - 4 * PADDING * s,
				.h = env->height - 4 * PADDING * s,
			},
		},
		.background_color = THEME_CARD,
		.active_color = THEME_CARD,
		.border_radius = 12.0f * s,
	});

	TextView* detail_title = new_text_view(&(TextViewArgs){
		.base = (ViewArgs){
			.rect = (Vec4){ .x = 32 * s, .y = 32 * s, .w = detail->base.rect.w, .h = title_size },
		},
		.font = title_font,
		.text_color = THEME_TEXT,
		.text_size = title_size,
	});

	TextView* detail_body = new_text_view(&(TextViewArgs){
		.base = (ViewArgs){
			.rect = (Vec4){ .x = 32 * s, .y = 32 * s + title_size + 20 * s, .w = detail->base.rect.w, .h = detail->base.rect.h },
		},
		.font = body_font,
		.text_color = THEME_TEXT_MUTED,
		.text_size = body_size,
	});

	array_append(&detail->base.children, (View*)detail_title);
	array_append(&detail->base.children, (View*)detail_body);

	array_append(&root->base.children, (View*)sidebar);
	array_append(&root->base.children, (View*)detail);

	state->view = (View*)root;
	state->sidebar = sidebar;
	state->detail_title = detail_title;
	state->detail_body = detail_body;

	select_section(state->selected);
}

export void app_update(Env *env)
{
	if (state->width != env->width || state->height != env->height || state->scale != env->scale)
	{
		app_init(env);
	}

	bool clicked = env->mouse_left_down && !state->mouse_was_down;
	state->mouse_was_down = env->mouse_left_down;

	// Hit test against where the items were drawn last frame
	Vec2 mouse = { .x = env->mouse_x, .y = env->mouse_y };
	if (clicked && inside_rect(mouse, state->sidebar->base.rect))
	{
		for (int i = 0; i < SECTION_COUNT; i++)
		{
			View* item = &state->items[i]->base;
			if (inside_rect(mouse, v4_add_v2(item->rect, item->offset)))
			{
				select_section(i);
				break;
			}
		}
	}

	draw_view(state->view, env);

	// FPS counter in the bottom right margin
	char fps[32];
	snprintf(fps, 32, "FPS: %.0f", 1/env->delta_time);
	int fps_size = font_size(state->font_small);
	Vec2 fps_extent = measure_text(state->font_small, fps, fps_size);
	float s = env->scale > 0 ? env->scale : 1.0f;
	Vec2 fps_position = { .x = env->width - 2 * PADDING * s - fps_extent.x, .y = env->height - (2 * PADDING * s + fps_size) / 2 };
	draw_text(image_from_env(env), state->font_small, fps, fps_size, fps_position, THEME_TEXT_FAINT, NULL);
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
	// AppState may have grown between builds, so new fields go at the end
	state = malloc(sizeof(AppState));
	memset(state, 0, sizeof(AppState));
	memcpy(state, handle.state, handle.size < sizeof(AppState) ? handle.size : sizeof(AppState));
	free(handle.state);
}

// Private

static void select_section(int index)
{
	state->selected = index;

	for (int i = 0; i < SECTION_COUNT; i++)
	{
		bool selected = i == index;
		state->items[i]->background_color = selected ? THEME_ITEM_SELECTED : THEME_ITEM;
		state->items[i]->active_color = selected ? THEME_ITEM_SELECTED : THEME_ITEM_HOVER;
	}

	state->detail_title->text = sections[index].title;
	state->detail_body->text = sections[index].body;
}

