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

emitter amongoc_timeout_us(amongoc_loop* loop, emitter em, int64_t timeout_us) noexcept {
    // Create and start a race between the two operations
    std::variant<emitter_result, emitter_result> race
        = co_await first_completed(NEO_FWD(em).as_unique(),
                                   amongoc_schedule_later(loop, timeout_us).as_unique());
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
                     amongoc_allocator        alloc,
                     box                      userdata_,
                     amongoc_then_transformer tr) noexcept {
    return as_emitter(  //
               cxx_allocator<>{alloc},
               amongoc::then(  //
                   NEO_MOVE(in).as_unique(),
                   [userdata = AM_FWD(userdata_).as_unique(), flags, tr](
                       emitter_result&& res) mutable noexcept -> emitter_result {
                       if ((flags & amongoc_async_forward_errors) and res.status.is_error()) {
                           // The result is errant and the caller wants to forward errors without
                           // transforming them. Return it now.
                           return AM_FWD(res);
                       }
                       res.value = tr(AM_FWD(userdata).release(),
                                      &res.status,
                                      AM_FWD(res).value.release())
                                       .as_unique();
                       return AM_FWD(res);
                   }))
        .release();
}

emitter amongoc_let(emitter                 op,
                    amongoc_async_flags     flags,
                    amongoc_allocator       alloc,
                    box                     userdata_,
                    amongoc_let_transformer tr) noexcept {
    // Note: Do not try to rewrite this as an "intuitive" coroutine, because `amongoc_let` the
    // transformer may return another `amongoc_let` emitter, which would lead to unbounded recursive
    // awaiting.
    nanosender_of<emitter_result> auto l = amongoc::let(  //
        AM_FWD(op).as_unique(),
        [flags, userdata = AM_FWD(userdata_).as_unique(), tr, alloc](
            emitter_result&& res) mutable -> unique_emitter {
            if ((flags & amongoc_async_forward_errors) and res.status.is_error()) {
                // The caller wants us to forward errors directly, so just return the
                // error immediately
                return amongoc_just(res.status, AM_FWD(res).value.release(), alloc).as_unique();
            }
            // Call the transformer to obtain the next emitter. amongoc::let will handle
            // connecting and starting it.
            return tr(AM_FWD(userdata).release(), res.status, AM_FWD(res).value.release())
                .as_unique();
        });
    return as_emitter(cxx_allocator<>{alloc}, AM_FWD(l)).release();
}

emitter amongoc_just(status st, box value, amongoc_allocator alloc) noexcept {
    return unique_emitter::from_connector(  //
               cxx_allocator<>{alloc},      //
               [value = AM_FWD(value).as_unique(), st, alloc](
                   unique_handler&& hnd) mutable -> unique_operation {
                   return unique_operation::from_starter(  //
                       cxx_allocator<>{alloc},
                       [hnd = AM_FWD(hnd), value = AM_FWD(value), st]() mutable {
                           hnd.complete(st, AM_FWD(value));
                       });
               })
        .release();
}

emitter amongoc_schedule_later(amongoc_loop* loop, int64_t duration_us) {
    return unique_emitter::from_connector(  //
               get_allocator(*loop),
               [=](unique_handler h) {
                   return unique_operation::from_starter(  //
                       get_allocator(*loop),
                       [=, h = AM_FWD(h)] mutable {
                           loop->vtable->call_later(loop,
                                                    duration_us,
                                                    amongoc_nil,
                                                    AM_FWD(h).release());
                       });
               })
        .release();
}

emitter amongoc_schedule(amongoc_loop* loop) {
    return as_emitter(get_allocator(*loop),
                      loop->schedule() | amongoc::then([](auto) { return emitter_result(); }))
        .release();
}

amongoc_operation
amongoc_tie(amongoc_emitter em, amongoc_status* status, amongoc_box* value) AMONGOC_NOEXCEPT {
    // Connect to a handler that stores the result values in the pointed-to locations
    return AM_FWD(em)
        .as_unique()
        .connect(terminating_allocator,
                 [status, value](amongoc_status st, unique_box&& val) {
                     if (status) {
                         *status = st;
                     }
                     if (value) {
                         *value = AM_FWD(val).release();
                     }
                 })
        .release();
}

amongoc_operation amongoc_detach(amongoc_emitter em) AMONGOC_NOEXCEPT {
    // Connect to a handler that simply discards the result values
    return AM_FWD(em).as_unique().connect(terminating_allocator, [](auto&&...) {}).release();
}

emitter amongoc_then_just(amongoc_emitter          in,
                          enum amongoc_async_flags flags,
                          amongoc_status           st,
                          amongoc_box              value,
                          amongoc_allocator        alloc) noexcept {
    return as_emitter(cxx_allocator<>{alloc},
                      amongoc::then(  //
                          AM_FWD(in).as_unique(),
                          [flags, st, value = AM_FWD(value).as_unique()](
                              emitter_result&& res) mutable {
                              if ((flags & amongoc_async_forward_errors)
                                  and res.status.is_error()) {
                                  return AM_FWD(res);
                              }
                              return emitter_result(st, AM_FWD(value));
                          }))
        .release();
}
