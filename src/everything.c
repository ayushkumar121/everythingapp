#include "env.h"
#define BASIC_IMPLEMENTATION
#include "basic.h"
#include "drawing.h"
#include "hotreload.h"
#include "views.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define THEME_BACKGROUND       0xFF2E2E2Eu
#define THEME_SIDEBAR          0xFF1C1C1Cu
#define THEME_ITEM             0xFF262626u
#define THEME_ITEM_HOVER       0xFF333333u
#define THEME_ITEM_SELECTED    0xFF4A4A4Au
#define THEME_CARD             0xFF242424u
#define THEME_TEXT             0xFFE6E6E6u
#define THEME_TEXT_MUTED       0xFFB0B0B0u
#define THEME_TEXT_FAINT       0xFF6E6E6Eu
#define THEME_SCROLL_TRACK     0xFF242424u
#define THEME_SCROLL_THUMB     0xFF555555u
#define THEME_ACCENT           0xFFFFA500u
#define THEME_ACCENT_HOVER     0xFFD48900u
#define THEME_ACCENT_SELECTED  0xFF8A5900u

// Layout sizes are in points, multiplied by the display scale
#define PADDING 10
#define ITEM_HEIGHT 50
#define SCROLL_BAR_THICKNESS 6
#define SCROLL_BAR_SPACE 20
#define TOGGLE_WIDTH 30
#define TOGGLE_HEIGHT 30
#define TOGGLE_ICON_SIZE 20

#define SIDE_BAR_WIDTH(env) (env->width/3)

typedef struct
{
	const char *title;
	const char *body;
} Section;

static const Section sections[] =
{
	{"Todays", "Nothing scheduled yet.\n\nPlan the day by adding tasks,\nevents and habits to check off."},
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
	PanelView* detail;
	TextView* detail_title;
	TextView* detail_body;
	int selected;
	bool mouse_was_down;
	bool sidebar_opened;
	Image sidebar_icon_source;
	Image sidebar_icon;
} AppState;

AppState *state = NULL;

static void select_section(int index);
static void toggle_sidebar(View* view, void* user_data);

export void app_load(void)
{
	state = malloc(sizeof(AppState));
	memset(state, 0, sizeof(AppState));

	// Bitmap fonts are only drawn at their native pixel size to stay sharp
	load_font(&state->font_regular, "assets/spleen-16x32.bdf");
	load_font(&state->font_small, "assets/ter-u18n.bdf");
	load_font(&state->font_bold, "assets/ter-u32b.bdf");
	load_image(&state->sidebar_icon_source, "assets/sidebar.bmp");

	state->sidebar_opened = true;
}

export void app_init(Env* env)
{
	state->width = env->width;
	state->height = env->height;
	state->scale = env->scale;

	Font body_font = env->scale >= 1.5f ? state->font_regular : state->font_small;
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

	// Prepared at the size it's drawn so it stays sharp at every display scale
	free_image(&state->sidebar_icon);
	if (state->sidebar_icon_source.pixels != NULL)
	{
		int icon_size = TOGGLE_ICON_SIZE * env->scale;
		state->sidebar_icon = resize_image(state->sidebar_icon_source, icon_size, icon_size);
	}

	ImageButtonView* sidebar_toggle = new_image_button_view(&(ImageButtonViewArgs){
		.base = (ViewArgs){
			.rect = (Vec4){
				.x = 2 * PADDING * env->scale,
				.y = 5 * env->scale,
				.w = TOGGLE_WIDTH * env->scale,
				.h = TOGGLE_HEIGHT * env->scale,
			},
		},
		.image = state->sidebar_icon,
		.background_color = THEME_ACCENT,
		.hover_color = THEME_ACCENT_HOVER,
		.pressed_color = THEME_ACCENT_SELECTED,
		.border_radius = 8 * env->scale,
		.on_click = toggle_sidebar,
		.user_data = state,
	});
	array_append(&root->base.children, (View*)sidebar_toggle);

	float sidebar_width = SIDE_BAR_WIDTH(env);

	ScrollView* sidebar = new_scroll_view(&(ScrollViewArgs){
		.base = (ViewArgs){
			.rect = (Vec4){ 
				.x =  -8.0f * env->scale, 
				.y =  4 * PADDING * env->scale, 
				.w = sidebar_width, 
				.h = env->height - 6 * PADDING * env->scale 
			},
		},
		.axis = DIRECTION_VERTICAL,
		.border_radius = 8 * env->scale,
		.bar_thickness = SCROLL_BAR_THICKNESS * env->scale,
		.background_color = THEME_SIDEBAR,
		.track_color = THEME_SCROLL_TRACK,
		.thumb_color = THEME_SCROLL_THUMB,
	});

	for (int i = 0; i < SECTION_COUNT; i++)
	{
		PanelView* item = new_panel_view(&(PanelViewArgs){
			.base = (ViewArgs){
				.rect = (Vec4){
					.x = 2 * PADDING * env->scale,
					.y = (PADDING + i * (ITEM_HEIGHT + PADDING)) * env->scale,
					.w = sidebar_width - (PADDING + SCROLL_BAR_SPACE) * env->scale,
					.h = ITEM_HEIGHT * env->scale,
				},
			},
			.border_radius = 8.0f * env->scale,
		});

		TextView* label = new_text_view(&(TextViewArgs){
			.base = (ViewArgs){
				.rect = (Vec4){ .x = 16 * env->scale, .y = (int)(ITEM_HEIGHT * env->scale - body_size) / 2, .w = sidebar_width, .h = body_size },
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
				.x = sidebar_width + PADDING * env->scale,
				.y = 4 * PADDING * env->scale,
				.w = env->width - sidebar_width - 3 * PADDING * env->scale,
				.h = env->height - 6 * PADDING * env->scale,
			},
		},
		.background_color = THEME_CARD,
		.active_color = THEME_CARD,
		.border_radius = 12.0f * env->scale,
	});

	TextView* detail_title = new_text_view(&(TextViewArgs){
		.base = (ViewArgs){
			.rect = (Vec4){ .x = 32 * env->scale, .y = 32 * env->scale, .w = detail->base.rect.w, .h = title_size },
		},
		.font = title_font,
		.text_color = THEME_TEXT,
		.text_size = title_size,
	});

	TextView* detail_body = new_text_view(&(TextViewArgs){
		.base = (ViewArgs){
			.rect = (Vec4){ .x = 32 * env->scale, .y = 32 * env->scale + title_size + 20 * env->scale, .w = detail->base.rect.w, .h = detail->base.rect.h },
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
	state->detail = detail;
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

	float sidebar_width = SIDE_BAR_WIDTH(env);

	if (state->sidebar_opened)
	{
		state->sidebar->base.rect.x = -8.0f * env->scale;
		state->detail->base.rect.x = sidebar_width + PADDING * env->scale;
		state->detail->base.rect.w = env->width - sidebar_width - 3 * PADDING * env->scale;

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
	}
	else 
	{
		state->sidebar->base.rect.x = -1.0f * sidebar_width;
		state->detail->base.rect.x = 2 * PADDING * env->scale;
		state->detail->base.rect.w = env->width - 4 * PADDING * env->scale;
	}

	draw_view(state->view, env);

	// FPS counter in the bottom right margin
	char fps[32];
	snprintf(fps, 32, "FPS: %.0f", 1/env->delta_time);
	int fps_size = font_size(state->font_small);
	Vec2 fps_extent = measure_text(state->font_small, fps, fps_size);
	float s = env->scale;
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

static void toggle_sidebar(View* view, void* user_data)
{
	(void)view;

	AppState* state = user_data;
	state->sidebar_opened = !state->sidebar_opened;
}