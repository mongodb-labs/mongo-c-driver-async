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

#include <mlib/object_t.hpp>

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
        : _start(mlib_fwd(start)) {}

    constexpr void start() noexcept { mlib::invoke(_start); }

private:
    mlib_no_unique_address Starter _start;
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
    requires mlib::invocable<Connector, archetype_nanoreceiver<T>>
class simple_sender {
public:
    using sends_type = T;

    constexpr explicit simple_sender(Connector&& fn)
        : _connect(mlib_fwd(fn)) {}

    template <nanoreceiver_of<sends_type> R>
    constexpr nanooperation auto connect(R&& recv) && noexcept {
        return static_cast<Connector&&>(_connect)(mlib_fwd(recv));
    }

    template <nanoreceiver_of<sends_type> R>
    constexpr nanooperation auto connect(R&& recv) const& noexcept
        requires requires(const Connector& conn) { conn(mlib_fwd(recv)); }
    {
        return static_cast<const Connector&>(_connect)(mlib_fwd(recv));
    }

private:
    mlib_no_unique_address mlib::object_t<Connector> _connect;
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
    return simple_sender<T, F>(mlib_fwd(fn));
}

/**
 * @brief Adapt a C++ nanoreceiver to an amongoc_handler
 */
template <typename R>
struct cxx_recv_as_c_handler {
    [[no_unique_address]] R _recv;

    struct stopper {
        void* userdata;
        void (*callback)(void*);
        void operator()() noexcept { this->callback(this->userdata); }
    };

    using stop_callback = stop_callback_t<effective_stop_token_t<R>, stopper>;

    static constexpr amongoc_handler_vtable handler_vtable = {
        .complete =
            [](amongoc_handler* hnd, amongoc_status st, amongoc_box value) noexcept {
                auto& self = hnd->userdata.view.as<cxx_recv_as_c_handler>();
                auto  val  = mlib_fwd(value).as_unique();
                R&    r    = self._recv;
                if constexpr (nanoreceiver_of<R, emitter_result>) {
                    // Invoke with an emitter_result
                    static_cast<R&&>(r)(emitter_result(st, mlib_fwd(val)));
                } else if constexpr (nanoreceiver_of<R, result<unique_box>>) {
                    // Invoke with a result<T>
                    if (st.is_error()) {
                        static_cast<R&&>(r)(result<unique_box>(error(st)));
                    } else {
                        static_cast<R&&>(r)(result<unique_box>(success(mlib_fwd(val))));
                    }
                } else {
                    // Neither invocation works. Generate an error that describes the problem:
                    static_assert(nanoreceiver_of<R, result<unique_box>>
                                      or nanoreceiver_of<R, emitter_result>,
                                  "Receiver does not have an appropriate callback signature");
                }
            },
        .register_stop =
            [] {
                if constexpr (has_stop_token<R>) {
                    // Expose the stop token to the handler
                    return [](amongoc_handler const* hnd,
                              void*                  userdata,
                              void (*callback)(void*)) noexcept {
                        auto&                self = hnd->userdata.view.as<cxx_recv_as_c_handler>();
                        stoppable_token auto tk   = get_stop_token(self._recv);
                        return unique_box::make<stop_callback>(allocator<>{hnd->get_allocator()},
                                                               tk,
                                                               stopper{userdata, callback})
                            .release();
                    };
                } else {
                    return nullptr;
                }
            }(),
        .get_allocator =
            [] {
                if constexpr (mlib::has_mlib_allocator<R>) {
                    // Expose the associated allocator to the hnadler
                    return [](amongoc_handler const* self,
                              ::mlib_allocator) noexcept -> ::mlib_allocator {
                        allocator<> a = mlib::get_allocator(
                            self->userdata.view.as<cxx_recv_as_c_handler>()._recv);
                        return a.c_allocator();
                    };
                } else {
                    return nullptr;
                }
            }(),
    };
};

/**
 * @brief Create a C handler object that adapts a C++ receiver.
 *
 * @param cxx_recv A C++ nanoreceiver. The receiver should be invocable with an appropriate type
 * @return amongoc_handler A C-API receiver object that accepts the emitter result and converts it
 * to the appropriate type for the nanoreceiver.
 */
template <typename R>
unique_handler as_handler(R&& cxx_recv) {
    using adaptor_type = cxx_recv_as_c_handler<R>;
    amongoc_handler ret;
    /// TODO: What to do if allocation fails here? Let it throw?
    allocator<> alloc = mlib::get_allocator(cxx_recv, allocator<>(::mlib_default_allocator));
    ret.userdata      = unique_box::from(alloc, adaptor_type{mlib_fwd(cxx_recv)}).release();
    ret.vtable        = &adaptor_type::handler_vtable;
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
unique_emitter as_emitter(allocator<> alloc, S&& sender) noexcept {
    // The composed operation type with a unique_handler. This line will also
    // enforce that the nanosender sends a type that is compatible with the
    // unique_handler::operator()
    using operation_type = connect_t<std::remove_cvref_t<S>, unique_handler>;
    try {
        return unique_emitter::from_connector(  //
            alloc,
            [s = mlib_fwd(sender)](unique_handler&& hnd) mutable -> unique_operation {
                // Create an amongoc_operation from the C++ operation state
                amongoc_operation oper = {};
                // XXX: It would be more efficient to store the handler on the
                // amongoc_operation::handler. How can we make that work?
                oper.userdata
                    = unique_box::make<operation_type>(hnd.get_allocator(), defer_convert([&] {
                                                           return connect(std::move(s),
                                                                          std::move(hnd));
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
unique_handler as_handler(unique_handler&&)              = delete;
unique_emitter as_emitter(allocator<>, unique_emitter&&) = delete;

}  // namespace amongoc
