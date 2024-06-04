#ifndef ENV_H
#define ENV_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct
{
	double delta_time;
	int window_width;
	int window_height;
	uint8_t *buffer;

	size_t key_code;
	bool key_down;

	bool mouse_left_down;
	bool mouse_right_down;
	bool mouse_moved;
	int mouse_x;
	int mouse_y;
} Env;

#endif
