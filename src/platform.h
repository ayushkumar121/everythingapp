#ifndef PLATFORM_H
#define PLATFORM_H

#include "env.h"

#include <stdint.h>

double platform_get_time(void);

/* Hot reloding */

typedef struct {
  void (*app_init)(void);
    void (*app_update)(Env *env);

    // For hot reloading
    void* (*app_pre_reload)(void);
    void (*app_post_reload)(void* old_state);

    void* handle;
} AppModule;

void platform_load_module(AppModule *module, char* file_path);

#endif
