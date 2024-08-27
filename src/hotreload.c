#include "hotreload.h"
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#endif	

void load_module(AppModule *module, char* file_path)
{
	assert(module != NULL);

#ifdef _WIN32
	if (module->handle)
	{
		FreeLibrary(module->handle);
	}

	module->handle = LoadLibrary(file_path);
	if (module->handle == NULL)
	{
		MessageBox(0, "Error occurred during module loading", "Error", MB_OK | MB_ICONERROR);
		exit(1);
	}

	module->app_load = (void (*)(void))GetProcAddress(module->handle, "app_load");
	module->app_init = (void (*)(Env*))GetProcAddress(module->handle, "app_init");
	module->app_update = (void (*)(Env*))GetProcAddress(module->handle, "app_update");
	module->app_pre_reload = (AppStateHandle (*)(void))GetProcAddress(module->handle, "app_pre_reload");
	module->app_post_reload = (void (*)(AppStateHandle))GetProcAddress(module->handle, "app_post_reload");
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