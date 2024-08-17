#pragma once

#include "env.h"

#include <stdint.h>

typedef struct
{
	void* state;
	size_t size;
} AppStateHandle;

typedef struct
{
	void (*app_load)(void);
	void (*app_init)(Env *env);
	void (*app_update)(Env *env);

	// For hot reloading
	AppStateHandle (*app_pre_reload)(void);
	void (*app_post_reload)(AppStateHandle);

	void* handle;
} AppModule;

void load_module(AppModule *module, char* file_path);
