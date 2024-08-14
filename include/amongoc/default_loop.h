#pragma once

#include "./config.h"
#include "./loop.h"

AMONGOC_EXTERN_C_BEGIN

extern void amongoc_init_default_loop(amongoc_loop* loop);
extern void amongoc_run_default_loop(amongoc_loop* loop);
extern void amongoc_destroy_default_loop(amongoc_loop* loop);

AMONGOC_EXTERN_C_END
