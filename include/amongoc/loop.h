#pragma once

#include "./box.h"
#include "./handler.h"
#include "./status.h"

#include <mlib/config.h>

typedef struct amongoc_loop_vtable amongoc_loop_vtable;
typedef struct amongoc_loop        amongoc_loop;

enum amongoc_event_loop_verison { amongoc_event_loop_v0 = 1 };

struct amongoc_const_buffer {
    const void* buf;
    size_t      len;
};

struct amongoc_mutable_buffer {
    void*  buf;
    size_t len;
};

struct amongoc_loop_vtable {
    // The version of the event loop API implemented by this object.
    enum amongoc_event_loop_verison version;

    void (*call_soon)(amongoc_loop* self, amongoc_status st, amongoc_box arg, amongoc_handler recv)
        mlib_noexcept;

    void (*call_later)(amongoc_loop*   self,
                       struct timespec duration,
                       amongoc_box     arg,
                       amongoc_handler recv) mlib_noexcept;

    void (*getaddrinfo)(amongoc_loop*   self,
                        const char*     name,
                        const char*     svc,
                        amongoc_handler on_finish) mlib_noexcept;

    void (*tcp_connect)(amongoc_loop*   self,
                        amongoc_view    addrinfo,
                        amongoc_handler on_connect) mlib_noexcept;

    void (*tcp_write_some)(amongoc_loop*                      self,
                           amongoc_view                       tcp_conn,
                           struct amongoc_const_buffer const* bufs,
                           size_t                             nbufs,
                           amongoc_handler                    on_write) mlib_noexcept;

    void (*tcp_read_some)(amongoc_loop*                        self,
                          amongoc_view                         tcp_conn,
                          struct amongoc_mutable_buffer const* bufs,
                          size_t                               nbufs,
                          amongoc_handler                      on_finish) mlib_noexcept;

    mlib_allocator (*get_allocator)(const amongoc_loop* self) mlib_noexcept;
};

struct amongoc_loop {
    amongoc_box                userdata;
    amongoc_loop_vtable const* vtable;

#if mlib_is_cxx()
    // Nanosender for schedule()
    struct sched_snd;

    constexpr sched_snd schedule() noexcept;

    mlib::allocator<> get_allocator() const noexcept {
        if (vtable->get_allocator) {
            return mlib::allocator<>(vtable->get_allocator(this));
        }
        return mlib::allocator<>(mlib_default_allocator);
    }
#endif
};

mlib_extern_c_begin();
/**
 * @brief Obtain the allocator associated with an event loop, if present
 *
 * @param loop The event loop to be queried
 * @return mlib_allocator The allocator associated with the loop if the loop provides
 * one. Otherwise, returns `mlib_default_allocator`.
 */
inline mlib_allocator amongoc_loop_get_allocator(const amongoc_loop* loop) mlib_noexcept {
    if (loop->vtable->get_allocator) {
        return loop->vtable->get_allocator(loop);
    }
    return mlib_default_allocator;
}
mlib_extern_c_end();
