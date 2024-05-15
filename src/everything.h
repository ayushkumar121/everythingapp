#ifndef EVERYTHING_H
#define EVERYTHING_H

#include "env.h"
#include "config.h"
#include "platform.h"

typedef struct {
  void (*app_init)(void);
  void (*app_update)(Env *env);
} App;

#endif
