#include "hotreload.h"

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

void load_module(AppModule *module, char* file_path)
{
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
}
