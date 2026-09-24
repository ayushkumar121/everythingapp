#include "hotreload.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

void load_module(AppModule *module, char* file_path)
{
	assert(module != NULL);

#ifdef _WIN32
	if (module->handle)
	{
		FreeLibrary(module->handle);
		module->handle = NULL;
	}

	// Windows locks loaded DLLs, so load a copy and leave the original free for the build to overwrite
	char loaded_path[MAX_PATH];
	const char *ext = strrchr(file_path, '.');
	int stem_length = ext ? (int)(ext - file_path) : (int)strlen(file_path);
	snprintf(loaded_path, sizeof(loaded_path), "%.*s_loaded.dll", stem_length, file_path);

	if (!CopyFile(file_path, loaded_path, FALSE))
	{
		MessageBox(0, "Failed to copy app module", "Error", MB_OK | MB_ICONERROR);
		exit(1);
	}

	module->handle = LoadLibrary(loaded_path);
	if (module->handle == NULL)
	{
		MessageBox(0, "Error occurred during module loading", "Error", MB_OK | MB_ICONERROR);
		exit(1);
	}

	module->app_load = (void (*)(void))GetProcAddress(module->handle, "app_load");
	module->app_init = (void (*)(Env*))(void (*)(void))GetProcAddress(module->handle, "app_init");
	module->app_update = (void (*)(Env*))(void (*)(void))GetProcAddress(module->handle, "app_update");
	module->app_pre_reload = (AppStateHandle (*)(void))(void (*)(void))GetProcAddress(module->handle, "app_pre_reload");
	module->app_post_reload = (void (*)(AppStateHandle))(void (*)(void))GetProcAddress(module->handle, "app_post_reload");

	OutputDebugString("INFO: Loaded app module\n");
#else
	if (module->handle != NULL) dlclose(module->handle);

	module->handle = dlopen(file_path, RTLD_NOW);
	if (!module->handle)
	{
		fprintf(stderr, "ERROR: Error occurred during module loading: %s\n", dlerror());
		exit(EXIT_FAILURE);
	}

	module->app_load = dlsym(module->handle, "app_load");
	module->app_init = dlsym(module->handle, "app_init");
	module->app_update = dlsym(module->handle, "app_update");
	module->app_pre_reload = dlsym(module->handle, "app_pre_reload");
	module->app_post_reload = dlsym(module->handle, "app_post_reload");

	char* err = dlerror();
	if (err != NULL)
	{
		fprintf(stderr, "ERROR: Error occurred during module symbol: %s\n", err);
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "INFO: Loaded app module\n");
#endif
}