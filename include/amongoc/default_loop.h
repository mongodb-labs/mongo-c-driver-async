#pragma once

#include "./alloc.h"
#include "./config.h"
#include "./loop.h"
#include "./status.h"

AMONGOC_EXTERN_C_BEGIN

extern amongoc_status        amongoc_default_loop_init_with_allocator(amongoc_loop* loop,
                                                                      amongoc_allocator) AMONGOC_NOEXCEPT;
static inline amongoc_status amongoc_default_loop_init(amongoc_loop* loop) AMONGOC_NOEXCEPT {
    return amongoc_default_loop_init_with_allocator(loop, amongoc_default_allocator);
}

extern void amongoc_default_loop_run(amongoc_loop* loop) AMONGOC_NOEXCEPT;
extern void amongoc_default_loop_destroy(amongoc_loop* loop) AMONGOC_NOEXCEPT;

AMONGOC_EXTERN_C_END
