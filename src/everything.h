#ifndef EVERYTHING_H
#define EVERYTHING_H

#include "env.h"
#include "config.h"
#include "platform.h"

typedef struct {
  void (*app_init)(void);
    void (*app_update)(Env *env);

    // For hot reloading
    void* (*app_pre_reload)(void);
    void (*app_post_reload)(void* old_state);
} App;

#endif
