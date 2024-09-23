#pragma once

#include "./alloc.h"
#include "./loop.h"
#include "./status.h"

#include <mlib/config.h>

mlib_extern_c_begin();

extern amongoc_status        amongoc_default_loop_init_with_allocator(amongoc_loop* loop,
                                                                      mlib_allocator) mlib_noexcept;
static inline amongoc_status amongoc_default_loop_init(amongoc_loop* loop) mlib_noexcept {
    return amongoc_default_loop_init_with_allocator(loop, mlib_default_allocator);
}

extern void amongoc_default_loop_run(amongoc_loop* loop) mlib_noexcept;
extern void amongoc_default_loop_destroy(amongoc_loop* loop) mlib_noexcept;

mlib_extern_c_end();

#if mlib_is_cxx()
namespace amongoc {

struct default_event_loop {
public:
    default_event_loop() { ::amongoc_default_loop_init(&_loop); }
    ~default_event_loop() { ::amongoc_default_loop_destroy(&_loop); }

    default_event_loop(default_event_loop&&) = delete;

    void run() noexcept { ::amongoc_default_loop_run(&_loop); }

    amongoc_loop& get() noexcept { return _loop; }

private:
    amongoc_loop _loop;
};

}  // namespace amongoc
#endif
