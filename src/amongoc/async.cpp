#include "./box.compress.hpp"
#include "./coroutine.hpp"
#include "./nano/first.hpp"
#include "./nano/let.hpp"
#include "./nano/simple.hpp"
#include "./nano/then.hpp"

#include <amongoc/async.h>
#include <amongoc/box.hpp>
#include <amongoc/emitter.hpp>
#include <amongoc/handler.h>
#include <amongoc/loop.hpp>
#include <amongoc/operation.hpp>

#include <mlib/allocate_unique.hpp>

#include <new>

using namespace amongoc;

emitter amongoc_timeout(amongoc_loop* loop, emitter em, std::timespec tim) noexcept {
    // Create and start a race between the two operations
    co_await ramp_end;
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
    } catch (std::bad_alloc const&) {
        return amongoc_alloc_failure();
    }
}

emitter amongoc_schedule(amongoc_loop* loop) {
    return as_emitter(loop->get_allocator(),
                      loop->schedule() | amongoc::then([](auto) { return emitter_result(); }))
        .release();
}

amongoc_operation(amongoc_tie)(amongoc_emitter em,
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
                .bind_allocator_connect(mlib::allocator<>(alloc), [](emitter_result&&) {})
                .release();
        } else {
            // Only storing the value
            return mlib_fwd(em)
                .as_unique()
                .bind_allocator_connect(mlib::allocator<>(alloc),
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
                .bind_allocator_connect(mlib::allocator<>(alloc),
                                        [status](emitter_result&& res) { *status = res.status; })
                .release();
        } else {
            // Storing both the value and the status
            return mlib_fwd(em)
                .as_unique()
                .bind_allocator_connect(mlib::allocator<>(alloc),
                                        [status, value](emitter_result&& res) {
                                            *status = res.status;
                                            *value  = mlib_fwd(res).value.release();
                                        })
                .release();
        }
    }
}

amongoc_operation(amongoc_detach)(amongoc_emitter em, mlib_allocator alloc) mlib_noexcept {
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
            ::amongoc_handler_complete(&self->handler, nomem, amongoc_nil);
        };
        return ret;
    }};
    emitter                       ret  = {};
    ret.vtable                         = &vtab;
    return ret;
}

void amongoc_detach_start(amongoc_emitter emit) mlib_noexcept {
    auto em = std::move(emit).as_unique();
    struct consigned_operation {
        // Dynamically allocated so it has a stable address
        mlib::unique_ptr<unique_operation> oper;
        // Notify box that we are relocatable
        using enable_trivially_relocatable [[maybe_unused]] = consigned_operation;
        // The handler function, simply discarding the result
        void operator()(emitter_result&&) const {}
        // Take the allocator from the unique pointer
        mlib::allocator<> get_allocator() const noexcept { return oper.get_deleter().alloc; }
    };
    consigned_operation co{mlib::allocate_unique<unique_operation>(::mlib_default_allocator)};
    // Take a stable reference to the operation state storage
    auto& oper = *co.oper;
    // Create the operation state and store it in the dynamic location
    oper = std::move(em).connect(unique_handler::from(std::move(co)));
    // Launch immediately
    oper.start();
}
