#include <amongoc/async.h>

#include <amongoc/box.h>
#include <amongoc/emitter.h>
#include <amongoc/handler.h>
#include <amongoc/operation.h>

#include "./coroutine.hpp"
#include "./nano/first.hpp"
#include "./nano/let.hpp"

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

emitter amongoc_then(emitter                  op,
                     amongoc_then_flags       flags,
                     box                      userdata_,
                     amongoc_then_transformer tr) noexcept {
    auto userdata = NEO_MOVE(userdata_).as_unique();
    auto result   = co_await NEO_MOVE(op);
    if ((flags & amongoc_then_forward_errors) and result.status.is_error()) {
        // The result is errant and the caller wants to forward errors without
        // transforming them. Return it now.
        co_return result;
    }
    // Call the transformer
    status st    = result.status;
    auto   value = tr(userdata.release(), &st, result.value.release()).as_unique();
    // Return the transformed result
    co_return emitter_result(st, NEO_MOVE(value));
}

emitter amongoc_let(emitter                 op,
                    amongoc_then_flags      flags,
                    box                     userdata_,
                    amongoc_let_transformer tr) noexcept {
    // Note: We cannot use a coroutine like with amongoc_then, because the transformer
    // may call amongo_let again, leading to an unbounded recursive awaiting. amongoc::let
    // will handle such possibility correctly.
    nanosender_of<emitter_result> auto l = amongoc::let(  //
        AM_FWD(op).as_unique(),
        [flags, userdata = AM_FWD(userdata_).as_unique(), tr](
            emitter_result&& res) mutable -> unique_emitter {
            if ((flags & amongoc_then_forward_errors) and res.status.is_error()) {
                // The caller wants us to forward errors directly, so just return the
                // error immediately
                return amongoc_just(res.status, res.value.release()).as_unique();
            }
            // Call the transformer to obtain the next emitter. amongoc::let will handle
            // connecting and starting it.
            return tr(userdata.release(), res.status, res.value.release()).as_unique();
        });
    return as_emitter(AM_FWD(l)).release();
    // // co_return co_await NEO_MOVE(next_op);
}

emitter amongoc_just(status st, box value_) noexcept {
    return unique_emitter::from_connector(  //
               [value = AM_FWD(value_).as_unique(),
                st](unique_handler&& hnd) mutable -> unique_operation {
                   return unique_operation::from_starter(
                       [hnd = AM_FWD(hnd), value = AM_FWD(value), st]() mutable {
                           AM_FWD(hnd).complete(st, AM_FWD(value));
                       });
               })
        .release();
}

amongoc_operation
amongoc_tie(amongoc_emitter em, amongoc_status* status, amongoc_box* value) AMONGOC_NOEXCEPT {
    // Connect to a handler that stores the result values in the pointed-to locations
    return AM_FWD(em)
        .as_unique()
        .connect([=](amongoc_status st, unique_box&& val) {
            if (status) {
                *status = st;
            }
            if (value) {
                *value = val.release();
            }
        })
        .release();
}

amongoc_operation amongoc_detach(amongoc_emitter em) AMONGOC_NOEXCEPT {
    // Connect to a handler that simply discards the result values
    return AM_FWD(em).as_unique().connect([=](auto&&...) {}).release();
}
