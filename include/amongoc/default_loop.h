#pragma once

#include "./alloc.h"
#include "./loop.h"
#include "./status.h"

#include <mlib/config.h>

mlib_extern_c_begin();

extern amongoc_status        amongoc_default_loop_init_with_allocator(amongoc_loop* loop,
                                                                      amongoc_allocator) mlib_noexcept;
static inline amongoc_status amongoc_default_loop_init(amongoc_loop* loop) mlib_noexcept {
    return amongoc_default_loop_init_with_allocator(loop, amongoc_default_allocator);
}

extern void amongoc_default_loop_run(amongoc_loop* loop) mlib_noexcept;
extern void amongoc_default_loop_destroy(amongoc_loop* loop) mlib_noexcept;

mlib_extern_c_end();
