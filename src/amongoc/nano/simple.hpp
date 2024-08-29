#pragma once

#include "./concepts.hpp"
#include "./result.hpp"
#include "./stop.hpp"
#include "./util.hpp"

#include <amongoc/async.h>
#include <amongoc/box.h>
#include <amongoc/emitter.h>
#include <amongoc/handler.h>
#include <amongoc/operation.h>
#include <amongoc/status.h>

#include <neo/attrib.hpp>
#include <neo/object_t.hpp>

#include <new>

namespace amongoc {

/**
 * @brief A simple nanooperation created from a function that implements the behavior of `start()`
 *
 * @tparam Starter The function that starts the operation (CTAD recommended)
 */
template <typename Starter>
class simple_operation {
public:
    simple_operation() = default;

    constexpr explicit simple_operation(Starter&& start)
        : _start(NEO_FWD(start)) {}

    constexpr void start() noexcept { NEO_INVOKE(_start); }

private:
    NEO_NO_UNIQUE_ADDRESS Starter _start;
};

template <typename S>
explicit simple_operation(S&&) -> simple_operation<S>;

/**
 * @brief A simple nanosender based on a function that implements the behavior of `connect()`
 *
 * @tparam T The result type for the nanosender
 * @tparam Connector The function that implements the `connect()` callback. Must be invocable with a
 * nanoreceiver for `T` and return a nanooperation
 */
template <typename T, typename Connector>
    requires neo::invocable2<Connector, archetype_nanoreceiver<T>>
class simple_sender {
public:
    using sends_type = T;

    constexpr explicit simple_sender(Connector&& fn)
        : _connect(NEO_FWD(fn)) {}

    template <nanoreceiver_of<sends_type> R>
    constexpr nanooperation auto connect(R&& recv) && noexcept {
        return static_cast<Connector&&>(_connect)(NEO_FWD(recv));
    }

    template <nanoreceiver_of<sends_type> R>
    constexpr nanooperation auto connect(R&& recv) const& noexcept
        requires requires(const Connector& conn) { conn(NEO_FWD(recv)); }
    {
        return static_cast<const Connector&>(_connect)(NEO_FWD(recv));
    }

private:
    NEO_NO_UNIQUE_ADDRESS neo::object_t<Connector> _connect;
};

/**
 * @brief Create a simple sender from an invocable
 *
 * @tparam T The result type of the nanosender
 * @param fn The invocable that implements `connect()`
 * @return simple_sender<T, F>
 */
template <typename T, typename F>
constexpr simple_sender<T, F> make_simple_sender(F&& fn) {
    return simple_sender<T, F>(NEO_FWD(fn));
}

// Base class for cxx_recv_as_c_handler
template <typename R>
struct cxx_recv_handler_adaptor_base;

// Handle a receiver that accepts a result<unique_box>
template <nanoreceiver_of<result<unique_box>> R>
struct cxx_recv_handler_adaptor_base<R> {
    NEO_NO_UNIQUE_ADDRESS neo::object_t<R> _recv;

    void invoke(status st, box&& res) {
        auto val = mlib_fwd(res).as_unique();
        if (st.is_error()) {
            static_cast<R&&>(_recv)(result<unique_box>(error(st)));
        } else {
            static_cast<R&&>(_recv)(result<unique_box>(success(mlib_fwd(val))));
        }
    }
};

// Handle a receiver that accepts an emitter_result
template <nanoreceiver_of<emitter_result> R>
struct cxx_recv_handler_adaptor_base<R> {
    NEO_NO_UNIQUE_ADDRESS neo::object_t<R> _recv;

    void invoke(status st, box&& res) noexcept {
        static_cast<R&&>(_recv)(emitter_result(st, NEO_FWD(res).as_unique()));
    }
};

/**
 * @brief Adapt a C++ nanoreceiver to an amongoc_emitter
 */
template <typename R>
struct cxx_recv_as_c_handler : cxx_recv_handler_adaptor_base<R> {
    static void _complete(amongoc_view self, status st, box result) noexcept {
        self.as<cxx_recv_as_c_handler>().invoke(st, NEO_MOVE(result));
    }

    static constexpr amongoc_handler_vtable handler_vtable = {
        .complete = &_complete,
    };
};

// Specialization of the nanoreceiver adapter that exposes stop callbacks to the C handler
template <typename R>
    requires has_stop_token<R>
struct cxx_recv_as_c_handler<R> : cxx_recv_handler_adaptor_base<R> {
    static void _complete(amongoc_view self, status st, box result) noexcept {
        self.as<cxx_recv_as_c_handler>().invoke(st, NEO_MOVE(result));
    }

    struct stopper {
        void* userdata;
        void (*callback)(void*);
        void operator()() noexcept { this->callback(this->userdata); }
    };

    using stop_callback = stop_callback_t<effective_stop_token_t<R>, stopper>;

    static box
    _register_stop(amongoc_view self_, void* userdata, void (*callback)(void*)) noexcept {
        auto&                self = self_.as<cxx_recv_as_c_handler>();
        stoppable_token auto tk   = get_stop_token(static_cast<R const&>(self._recv));
        /// TODO: Pass an allocator here
        return unique_box::make<stop_callback>(cxx_allocator<>{amongoc_default_allocator},
                                               tk,
                                               stopper{userdata, callback})
            .release();
    }

    static constexpr amongoc_handler_vtable handler_vtable = {
        .complete      = &_complete,
        .register_stop = &_register_stop,
    };
};

/**
 * @brief Create a C handler object that adapts a C++ receiver.
 *
 * @param cxx_recv A C++ nanoreceiver. The receiver should be invocable with an appropriate type
 * @return amongoc_handler A C-API receiver object that accepts the emitter result and converts it
 * to the appropriate type for the nanoreceiver. The conversion process depends on the type that
 * is epxected by the nanoreceiver (see `cxx_recv_handler_adaptor_base::invoke`)
 */
template <typename R>
unique_handler as_handler(cxx_allocator<> alloc, R&& cxx_recv) {
    using adaptor_type = cxx_recv_as_c_handler<R>;
    amongoc_handler ret;
    /// TODO: What to do if allocation fails here? Let it throw?
    ret.userdata = unique_box::from(alloc, adaptor_type{{mlib_fwd(cxx_recv)}}).release();
    ret.vtable   = &adaptor_type::handler_vtable;
    return mlib_fwd(ret).as_unique();
};

/**
 * @brief Convert a C++ nanosender to a C API amongoc_emitter
 *
 * @note The nanosender must resolve with a result<T>
 *
 * @param sender The nanosender to be converted
 * @return A new emitter that adapts the nanosender to the C API
 */
template <nanosender S>
unique_emitter as_emitter(cxx_allocator<> alloc, S&& sender) noexcept {
    // The composed operation type with a unique_handler. This line will also
    // enforce that the nanosender sends a type that is compatible with the
    // unique_handler::operator()
    using operation_type = connect_t<std::remove_cvref_t<S>, unique_handler>;
    try {
        return unique_emitter::from_connector(  //
            alloc,
            [s = NEO_FWD(sender), alloc](unique_handler hnd) mutable -> unique_operation {
                // Create an amongoc_operation from the C++ operation state
                amongoc_operation oper = {};
                // XXX: It would be more efficient to store the handler on the
                // amongoc_operation::handler. How can we make that work?
                oper.userdata = unique_box::make<operation_type>(alloc, defer_convert([&] {
                                                                     return connect(NEO_MOVE(s),
                                                                                    NEO_MOVE(hnd));
                                                                 }))
                                    .release();
                oper.start_callback = [](amongoc_operation* self) noexcept {
                    self->userdata.view.as<operation_type>().start();
                };
                return mlib_fwd(oper).as_unique();
            });
    } catch (std::bad_alloc) {
        return amongoc_alloc_failure().as_unique();
    }
}

// Emitters and handlers are valid senders/receiver, but we don't want to double-wrap them
unique_handler as_handler(cxx_allocator<>, unique_handler&&) = delete;
unique_emitter as_emitter(cxx_allocator<>, unique_emitter&&) = delete;

}  // namespace amongoc
