#include "./coroutine.hpp"

#include <amongoc/box.hpp>
#include <amongoc/handler.h>

#include <mlib/alloc.h>

using namespace amongoc;

template struct amongoc::co_detail::handoff_finisher_with_stop_token<amongoc::handler_stop_token>;

void amongoc::emitter_promise::unhandled_exception() noexcept {
    try {
        throw;
    } catch (const wire::server_error& err) {
        return_value(
            emitter_result(status::from(err.code()),
                           unique_box::from(this->get_allocator(),
                                            bson::document(err.body(), this->get_allocator()))));
    } catch (std::system_error const& err) {
        return_value(emitter_result(status::from(err.code()),
                                    unique_box::from(::mlib_default_allocator,
                                                     mlib_str_copy(err.what()).str)));
    } catch (amongoc::exception const& err) {
        return_value(err.status());
    } catch (std::bad_alloc const&) {
        return_value(emitter_result(amongoc_status(&amongoc_generic_category, ENOMEM)));
    }
}

namespace {

// Starter function for an coroutine-based amongoc_emitter
struct co_emitter_starter {
    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true, co_emitter_starter);
    unique_co_handle<emitter_promise> _co;

    void operator()(amongoc_handler& h) const {
        // Move the handler from the operation to the coroutine
        emitter_promise& pr = _co.promise();
        if (_co.done()) {
            // The coroutine already returned. Fulfill the handler now
            ::amongoc_handler_complete(&h,
                                       pr.fin_result.status,
                                       std::move(pr.fin_result.value).release());
        } else {
            // The coroutine is still pending. Attach the handler and resume
            _co.promise().fin_handler = std::move(h).as_unique();
            _co.resume();
        }
    }
};

// Connector function object for the amongoc_emitter. Used as the connect
// handler for the emitter
struct co_emitter_connector {
    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true, co_emitter_connector);
    unique_co_handle<emitter_promise> _co;

    unique_operation operator()(unique_handler&& hnd) noexcept {
        return unique_operation::from_starter(mlib_fwd(hnd), co_emitter_starter{std::move(_co)});
    }
};

}  // namespace

amongoc_emitter amongoc::emitter_promise::get_return_object() noexcept {
    auto co = co_handle::from_promise(*this);
    static_assert(box_inlinable_type<co_emitter_connector>);
    return unique_emitter::from_connector(mlib::terminating_allocator,
                                          co_emitter_connector{unique_co_handle(std::move(co))})
        .release();
}
