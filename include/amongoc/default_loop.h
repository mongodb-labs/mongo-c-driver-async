#pragma once

#include "./alloc.h"
#include "./config.h"
#include "./loop.h"

AMONGOC_EXTERN_C_BEGIN

extern void amongoc_default_loop_init_with_allocator(amongoc_loop* loop, amongoc_allocator);

extern void amongoc_default_loop_run(amongoc_loop* loop);
extern void amongoc_default_loop_destroy(amongoc_loop* loop);

static inline void amongoc_default_loop_init(amongoc_loop* loop) {
    amongoc_default_loop_init_with_allocator(loop, amongoc_default_allocator);
}

AMONGOC_EXTERN_C_END
