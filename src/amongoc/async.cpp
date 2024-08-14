#include <amongoc/async.h>

#include <amongoc/box.h>
#include <amongoc/emitter.h>
#include <amongoc/handler.h>
#include <amongoc/operation.h>

#include "./coroutine.hpp"
#include "./nano/first.hpp"

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
    if ((flags & amongoc_then_forward_errors) and result.status.code != 0) {
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
    auto ud     = NEO_MOVE(userdata_).as_unique();
    auto result = co_await NEO_MOVE(op);
    if ((flags & amongoc_then_forward_errors) and result.status.code != 0) {
        co_return result;
    }
    emitter next_op = tr(ud.release(), result.status, result.value.release());
    co_return co_await NEO_MOVE(next_op);
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
