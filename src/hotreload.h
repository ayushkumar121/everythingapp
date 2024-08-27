#pragma once

#include "env.h"

#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#endif

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

#ifdef _WIN32
	HMODULE handle;
#else
	void* handle;
#endif
} AppModule;

void load_module(AppModule *module, char* file_path);
