#include "platform.h"

#include <Foundation/Foundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

void platform_load_module(AppModule *module, char* file_path) {
    if (module->handle != NULL) dlclose(mdoule->handle);

    app->handle = dlopen(file_path, RTLD_NOW);
    if (!app->handle) {
        fprintf(stderr, "ERROR: Error occurred during module loading: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    module->app_init = dlsym(module->handle, "app_init");
    module->app_update = dlsym(module->handle, "app_update");
    module->app_pre_reload = dlsym(module->handle, "app_pre_reload");
    module->app_post_reload = dlsym(module->handle, "app_post_reload");

    char* err = dlerror();
    if (err != NULL) {
        fprintf(stderr, "ERROR: Error occurred during module symbol: %s\n", err);
        exit(EXIT_FAILURE);
    }
}

double platform_get_time(void) {
    @autoreleasepool {
        NSDate *now = [NSDate date];
        NSTimeInterval timeInterval = [now timeIntervalSince1970];
        double currentTimeInMilliseconds = (timeInterval * 1000.0);
        return currentTimeInMilliseconds;
    }
}