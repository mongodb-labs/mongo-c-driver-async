#pragma once

#include "./box.h"
#include "./config.h"
#include "./emitter.h"
#include "./emitter_result.h"
#include "./handler.h"
#include "./operation.h"
#include "./status.h"

#ifdef __cplusplus
#include <concepts>
#endif

typedef struct amongoc_loop_vtable amongoc_loop_vtable;
typedef struct amongoc_loop        amongoc_loop;

enum amongoc_event_loop_verison { amongoc_event_loop_v1 = 1 };

struct amongoc_loop_vtable {
    // The version of the event loop API implemented by this object.
    enum amongoc_event_loop_verison version;

    void (*call_soon)(amongoc_loop* self, amongoc_status st, amongoc_box arg, amongoc_handler recv);

    void (*call_later)(amongoc_loop*   self,
                       int64_t         timeout_us,
                       amongoc_box     arg,
                       amongoc_handler recv);

    void (*getaddrinfo)(amongoc_loop*   self,
                        const char*     name,
                        const char*     svc,
                        amongoc_handler on_finish);

    void (*tcp_connect)(amongoc_loop* self, amongoc_view addrinfo, amongoc_handler on_connect);

    void (*tcp_write_some)(amongoc_loop*   self,
                           amongoc_view    tcp_conn,
                           const char*     data,
                           size_t          len,
                           amongoc_handler on_write);

    void (*tcp_read_some)(amongoc_loop*   self,
                          amongoc_view    tcp_conn,
                          char*           dest,
                          size_t          maxlen,
                          amongoc_handler on_finish);

    void* (*allocate)(amongoc_loop* self, size_t sz);
    void (*deallocate)(amongoc_loop* self, void*);
};

struct amongoc_loop {
    amongoc_box                userdata;
    amongoc_loop_vtable const* vtable;

#ifdef __cplusplus
    // Nanosender for schedule()
    struct sched_snd {
        amongoc_loop* loop;

        using sends_type = decltype(nullptr);

        template <typename R>
        struct op {
            amongoc_loop*           loop;
            [[no_unique_address]] R _recv;

            void start() noexcept {
                loop->vtable->call_soon(  //
                    loop,
                    amongoc_okay,
                    amongoc_nothing,
                    amongoc::unique_handler::from([this](amongoc_status,
                                                         amongoc::unique_box) mutable {
                        static_cast<R&&>(_recv)(nullptr);
                    }).release());
            }
        };

        template <typename R>
        op<R> connect(R&& r) const noexcept {
            return op<R>{loop, AM_FWD(r)};
        }
    };

    sched_snd schedule() noexcept { return {this}; }
#endif
};

AMONGOC_EXTERN_C_BEGIN

/**
 * @brief Schedule a completion on the given event loop.
 *
 * @param loop The event loop upon which to schedule the completion
 * @return amongoc_emitter An emitter that will call `amongoc_complete()` from
 *      within the event loop. The emitter always resolves with zero status and
 *      an amongoc_nothing value
 */
amongoc_emitter amongoc_schedule(amongoc_loop* loop);

/**
 * @brief Schedule the completion of an operation after a duration has elapsed
 *
 * @param loop The event loop on which to schedule the wait
 * @param duration_us The duration to wait (microseconds)
 * @return amongoc_emitter An emitter that will `amongoc_complete` after the
 *      given duration has elapsed.
 *
 * @note The emitter may complete early with non-zero status in the case of an error or cancellation
 */
amongoc_emitter amongoc_schedule_later(amongoc_loop* loop, int64_t duration_us);

AMONGOC_EXTERN_C_END