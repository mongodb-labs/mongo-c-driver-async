#include "./box.compress.hpp"
#include "./coroutine.hpp"
#include "./nano/first.hpp"
#include "./nano/let.hpp"
#include "./nano/simple.hpp"
#include "./nano/then.hpp"

#include <amongoc/async.h>
#include <amongoc/box.h>
#include <amongoc/emitter.h>
#include <amongoc/handler.h>
#include <amongoc/operation.h>

#include <new>

using namespace amongoc;

emitter amongoc_timeout(amongoc_loop* loop, emitter em, std::timespec tim) noexcept {
    // Create and start a race between the two operations
    std::variant<emitter_result, emitter_result> race
        = co_await first_completed(mlib_fwd(em).as_unique(),
                                   amongoc_schedule_later(loop, tim).as_unique());
    // The winner of the race fulfills the variant
    if (race.index() == 0) {
        // The main task completed first.
        co_return std::get<0>(std::move(race));
    } else {
        // The timeout completed first.
        co_return std::make_error_code(std::errc::timed_out);
    }
}

emitter amongoc_schedule_later(amongoc_loop* loop, std::timespec duration_us) {
    try {
        return unique_emitter::from_connector(  //
                   loop->get_allocator(),
                   [=](unique_handler&& h) {
                       return unique_operation::from_starter(  //
                           mlib_fwd(h),
                           [loop, duration_us](amongoc_handler& hnd) mutable {
                               // Transfer the handler into the event loop
                               loop->vtable->call_later(loop,
                                                        duration_us,
                                                        amongoc_nil,
                                                        std::move(hnd).as_unique().release());
                           });
                   })
            .release();
    } catch (std::bad_alloc) {
        return amongoc_alloc_failure();
    }
}

emitter amongoc_schedule(amongoc_loop* loop) {
    return as_emitter(get_allocator(*loop),
                      loop->schedule() | amongoc::then([](auto) { return emitter_result(); }))
        .release();
}

amongoc_operation amongoc_tie(amongoc_emitter em,
                              amongoc_status* status,
                              amongoc_box*    value,
                              mlib_allocator  alloc) mlib_noexcept {
    // This function returns a different emitter depending on whether
    // the pointer values are null. If they are, we can returne an emitter
    // of a reduced size, reducing the need for memory allocations
    if (status == nullptr) {
        if (value == nullptr) {
            // We aren't going to store the value nor the status, so use an empty callback,
            // saving an allocation.
            return mlib_fwd(em)
                .as_unique()
                .bind_allocator_connect(allocator<>(alloc), [](emitter_result&&) {})
                .release();
        } else {
            // Only storing the value
            return mlib_fwd(em)
                .as_unique()
                .bind_allocator_connect(allocator<>(alloc),
                                        [value](emitter_result&& res) {
                                            *value = mlib_fwd(res).value.release();
                                        })
                .release();
        }
    } else {
        if (value == nullptr) {
            // We are only storing the status
            return mlib_fwd(em)
                .as_unique()
                .bind_allocator_connect(allocator<>(alloc),
                                        [status](emitter_result&& res) { *status = res.status; })
                .release();
        } else {
            // Storing both the value and the status
            return mlib_fwd(em)
                .as_unique()
                .bind_allocator_connect(allocator<>(alloc),
                                        [status, value](emitter_result&& res) {
                                            *status = res.status;
                                            *value  = mlib_fwd(res).value.release();
                                        })
                .release();
        }
    }
}

amongoc_operation amongoc_detach(amongoc_emitter em, mlib_allocator alloc) mlib_noexcept {
    // Connect to a handler that simply discards the result values
    return amongoc_tie(em, nullptr, nullptr, alloc);
}

emitter amongoc_alloc_failure() noexcept {
    // We return an emitter with no state. We don't need to allocate memory (yet)
    static amongoc_emitter_vtable vtab = {.connect = [](amongoc_box /* nil */, amongoc_handler h) {
        static amongoc_status nomem{&amongoc_generic_category, ENOMEM};
        amongoc_operation     ret = {};
        ret.handler               = h;
        ret.start_callback        = [](amongoc_operation* self) noexcept {
            self->handler.complete(nomem, amongoc_nil.as_unique());
        };
        return ret;
    }};
    emitter                       ret  = {};
    ret.vtable                         = &vtab;
    return ret;
}
