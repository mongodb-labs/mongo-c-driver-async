#include <amongoc/async.h>

#include <amongoc/box.h>
#include <amongoc/emitter.h>
#include <amongoc/handler.h>
#include <amongoc/operation.h>

#include "./coroutine.hpp"
#include "./nano/first.hpp"
#include "./nano/let.hpp"
#include "./nano/then.hpp"

using namespace amongoc;

emitter amongoc_timeout(amongoc_loop* loop, emitter em, std::timespec tim) noexcept {
    // Create and start a race between the two operations
    std::variant<emitter_result, emitter_result> race
        = co_await first_completed(NEO_FWD(em).as_unique(),
                                   amongoc_schedule_later(loop, tim).as_unique());
    // The winner of the race fulfills the variant
    if (race.index() == 0) {
        // The main task completed first.
        co_return std::get<0>(NEO_MOVE(race));
    } else {
        // The timeout completed first.
        co_return std::make_error_code(std::errc::timed_out);
    }
}

emitter amongoc_then(emitter                  in,
                     amongoc_async_flags      flags,
                     mlib_allocator           alloc,
                     box                      userdata_,
                     amongoc_then_transformer tr) noexcept {
    return as_emitter(  //
               allocator<>{alloc},
               amongoc::then(  //
                   NEO_MOVE(in).as_unique(),
                   [userdata = mlib_fwd(userdata_).as_unique(), flags, tr](
                       emitter_result&& res) mutable noexcept -> emitter_result {
                       if ((flags & amongoc_async_forward_errors) and res.status.is_error()) {
                           // The result is errant and the caller wants to forward errors without
                           // transforming them. Return it now.
                           return mlib_fwd(res);
                       }
                       res.value = tr(mlib_fwd(userdata).release(),
                                      &res.status,
                                      mlib_fwd(res).value.release())
                                       .as_unique();
                       return mlib_fwd(res);
                   }))
        .release();
}

emitter amongoc_let(emitter                 op,
                    amongoc_async_flags     flags,
                    mlib_allocator          alloc,
                    box                     userdata_,
                    amongoc_let_transformer tr) noexcept {
    // Note: Do not try to rewrite this as an "intuitive" coroutine, because `amongoc_let` the
    // transformer may return another `amongoc_let` emitter, which would lead to unbounded recursive
    // awaiting.
    nanosender_of<emitter_result> auto l = amongoc::let(  //
        mlib_fwd(op).as_unique(),
        [flags, userdata = mlib_fwd(userdata_).as_unique(), tr, alloc](
            emitter_result&& res) mutable -> unique_emitter {
            if ((flags & amongoc_async_forward_errors) and res.status.is_error()) {
                // The caller wants us to forward errors directly, so just return the
                // error immediately
                return amongoc_just(res.status, mlib_fwd(res).value.release(), alloc).as_unique();
            }
            // Call the transformer to obtain the next emitter. amongoc::let will handle
            // connecting and starting it.
            return tr(mlib_fwd(userdata).release(), res.status, mlib_fwd(res).value.release())
                .as_unique();
        });
    return as_emitter(allocator<>{alloc}, mlib_fwd(l)).release();
}

emitter amongoc_just(status st, box value, mlib_allocator alloc) noexcept {
    try {
        return unique_emitter::from_connector(  //
                   allocator<>{alloc},          //
                   [value = mlib_fwd(value).as_unique(), st, alloc](
                       unique_handler&& hnd) mutable -> unique_operation {
                       return unique_operation::from_starter(  //
                           allocator<>{alloc},
                           mlib_fwd(hnd),
                           [value = mlib_fwd(value), st](amongoc_operation& op) mutable {
                               op.handler.complete(st, mlib_fwd(value));
                           });
                   })
            .release();
    } catch (std::bad_alloc) {
        return amongoc_alloc_failure();
    }
}

emitter amongoc_schedule_later(amongoc_loop* loop, std::timespec duration_us) {
    try {
        return unique_emitter::from_connector(  //
                   get_allocator(*loop),
                   [=](unique_handler h) {
                       return unique_operation::from_starter(  //
                           get_allocator(*loop),
                           mlib_fwd(h),
                           [loop, duration_us](amongoc_operation& op) mutable {
                               loop->vtable
                                   ->call_later(loop,
                                                duration_us,
                                                amongoc_nil,
                                                std::move(op.handler).as_unique().release());
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

amongoc_operation
amongoc_tie(amongoc_emitter em, amongoc_status* status, amongoc_box* value) mlib_noexcept {
    // Connect to a handler that stores the result values in the pointed-to locations
    return mlib_fwd(em)
        .as_unique()
        .connect(mlib::terminating_allocator,
                 [status, value](amongoc_status st, unique_box&& val) {
                     if (status) {
                         *status = st;
                     }
                     if (value) {
                         *value = mlib_fwd(val).release();
                     }
                 })
        .release();
}

amongoc_operation amongoc_detach(amongoc_emitter em) mlib_noexcept {
    // Connect to a handler that simply discards the result values
    return mlib_fwd(em)
        .as_unique()
        .connect(mlib::terminating_allocator, [](auto&&...) {})
        .release();
}

emitter amongoc_then_just(amongoc_emitter          in,
                          enum amongoc_async_flags flags,
                          amongoc_status           st,
                          amongoc_box              value,
                          mlib_allocator           alloc) noexcept {
    return as_emitter(allocator<>{alloc},
                      amongoc::then(  //
                          mlib_fwd(in).as_unique(),
                          [flags, st, value = mlib_fwd(value).as_unique()](
                              emitter_result&& res) mutable {
                              if ((flags & amongoc_async_forward_errors)
                                  and res.status.is_error()) {
                                  return mlib_fwd(res);
                              }
                              return emitter_result(st, mlib_fwd(value));
                          }))
        .release();
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
