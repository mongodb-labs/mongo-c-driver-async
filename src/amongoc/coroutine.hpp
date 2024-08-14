/**
 * @file coroutine.hpp
 * @brief Coroutine utilities for the amongoc library (internal)
 * @date 2024-08-06
 *
 * This file contains utility types and functions for using coroutines within
 * the amongoc library. This is for internal library use only.
 */
#pragma once

#include <coroutine>

// C++ internal headers
#include "./loop.hpp"
#include "./nano/concepts.hpp"
#include "./nano/nano.hpp"
#include "./nano/simple.hpp"
#include "./nano/stop.hpp"
#include "./nano/util.hpp"
#include "amongoc/loop.h"

// C library headers
#include <amongoc/box.h>
#include <amongoc/emitter.h>
#include <amongoc/handler.h>
#include <amongoc/operation.h>
#include <amongoc/status.h>

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <exception>
#include <list>
#include <memory>
#include <system_error>
#include <type_traits>
#include <utility>

namespace amongoc {

/**
 * @brief Presents a unique_ptr-like interface for a coroutine handle
 *
 * @tparam P The promise type of the underlying coroutine
 */
template <typename P>
class unique_co_handle {
public:
    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true);
    // The associated coroutine handle type
    using coroutine_handle_type = std::coroutine_handle<P>;

    // Default-construct (to null)
    constexpr unique_co_handle() = default;

    // Take ownership of a coroutine handle
    constexpr explicit unique_co_handle(coroutine_handle_type&& co) noexcept
        : _co(std::exchange(co, nullptr)) {}

    // Move-construct
    constexpr unique_co_handle(unique_co_handle&& other) noexcept
        : _co(other.release()) {}

    // Move-assign
    constexpr unique_co_handle& operator=(unique_co_handle&& other) const noexcept { reset(); }

    ~unique_co_handle() { reset(); }

    // Destroy the associated coroutine (if any) and set to null
    void reset() noexcept {
        if (_co) {
            _co.destroy();
        }
        _co = nullptr;
    }

    // Release ownership of the coroutine handle and return it to the caller
    [[nodiscard(
        "release() returns the coroutine handle without destroying it. "
        "Did you mean to use reset()?")]]  //
    constexpr coroutine_handle_type
    release() noexcept {
        return std::exchange(_co, nullptr);
    }

    // Get the raw coroutine_handle
    [[nodiscard]] constexpr coroutine_handle_type get() const noexcept { return _co; }

    // Resume the associated coroutine
    void resume() const { _co.resume(); }
    // Check if the associated coroutine is suspended at its final suspend point
    [[nodiscard]] bool done() const noexcept { return _co.done(); }

    // Get the associated promise
    std::add_lvalue_reference_t<P> promise() const noexcept { return _co.promise(); }

    // Check if not-null
    constexpr explicit operator bool() const noexcept { return !!_co; }
    // Default comparison
    [[nodiscard]] bool operator==(const unique_co_handle&) const = default;

private:
    coroutine_handle_type _co;
};

// Deduce the promise type when constructed from a coroutine handle
template <typename P>
explicit unique_co_handle(std::coroutine_handle<P>) -> unique_co_handle<P>;

/**
 * @brief A basic awaitable that performs the given action during await_suspend.
 *
 * await_resume() returns void. await_ready() always returns `false`
 */
template <typename F>
struct suspends_by {
    F _fn;

    // Never ready
    static constexpr bool await_ready() noexcept { return false; }
    // No return value for co_await
    static constexpr void await_resume() noexcept {}

    // Suspend by invoking the wrapped function
    template <typename P>
    constexpr decltype(auto) await_suspend(std::coroutine_handle<P> co) noexcept {
        return this->_fn(co);
    }
};

template <typename F>
explicit suspends_by(F&&) -> suspends_by<F>;

/**
 * @brief An awaitable adaptor for a nanosender
 *
 * @tparam S The sender type to be awaited
 */
template <nanosender S, typename Promise>
struct nanosender_awaitable {
    // The adapted nanosender
    S _sender;

    // The receiver that handles the sender's result
    struct receiver {
        // Pointer to the awaitable
        nanosender_awaitable* self;
        // Handle on the suspended coroutine
        std::coroutine_handle<Promise> co;

        // The receiver has a stop token if the coroutine's promise has a stop token
        stoppable_token auto query(get_stop_token_fn) const noexcept
            requires has_stop_token<Promise>
        {
            return get_stop_token(co.promise());
        }

        // Emplace the result and resume the associated coroutine
        void operator()(sends_t<S>&& result) {
            // Put the sent value into the awaitable's storage to be returned at await_resume
            self->_sent_value.emplace(NEO_FWD(result));
            // Resume the coroutine immediately
            co.resume();
        }
    };

    // The operation from connecting the nanosender to our receiver
    using operation_type = connect_t<S, receiver>;

    // The executed operation. This is constructed and started when the coroutine is suspended
    std::optional<operation_type> _operation{};
    // Storage for the eventual nanosender's result, returned from the co_await
    std::optional<sends_t<S>> _sent_value{};

    // The awaitable is never immediately ready
    constexpr bool await_ready() const noexcept { return false; }

    // Handle suspension
    void await_suspend(std::coroutine_handle<Promise> co) noexcept {
        // Construct the operation
        _operation.emplace(defer_convert([&] {
            return amongoc::connect(std::forward<S>(_sender), receiver{this, co});
        }));
        // Start the operation immediately
        _operation->start();
    }

    // Upon resumption, return the value that was sent by the sender. This is filled in
    // receiver::operator()
    sends_t<S> await_resume() noexcept { return static_cast<sends_t<S>&&>(*_sent_value); }
};

/**
 * @brief co_await semantics for an amongoc_emitter
 *
 */
struct emitter_awaitable {
    explicit emitter_awaitable(emitter&& em) noexcept
        : em(std::exchange(em, emitter{})) {}

    // Never immediately ready
    constexpr static bool await_ready() noexcept { return false; }

    // The awaited emitter
    unique_emitter em;
    // Storage for the operation to be connected
    unique_operation op;
    // Storage for the final result
    emitter_result ret{};

    // On resume, return the result of the operation
    emitter_result await_resume() noexcept { return NEO_MOVE(ret); }

    /**
     * @brief await_suspend() within a coroutine that has a stop token
     *
     * @tparam Promise The promise type, which provides a stop token
     * @param co The parent coroutine
     */
    template <typename Promise>
        requires has_stop_token<Promise>
    void await_suspend(std::coroutine_handle<Promise> co) noexcept {
        struct stopper {
            void* userdata;
            void (*callback)(void*);
            void operator()() { this->callback(this->userdata); }
        };
        using stop_callback = stop_callback_t<get_stop_token_t<Promise>, stopper>;

        struct state {
            emitter_awaitable*             self;
            std::coroutine_handle<Promise> co;
        };
        static const amongoc_handler_vtable vtable = {
            .complete =
                [](box p, status st, box value) noexcept {
                    unique_box ub = NEO_MOVE(p).as_unique();
                    state&     s  = ub.as<state>();
                    s.self->ret   = emitter_result(st, unique_box(NEO_MOVE(value)));
                    s.co.resume();
                },
            .register_stop
            = [](amongoc_view p, void* userdata, void (*callback)(void*)) noexcept -> box {
                return unique_box::make<stop_callback>(get_stop_token(p.as<state>().co.promise()),
                                                       stopper{userdata, callback})
                    .release();
            },
        };
        amongoc_handler handler;
        handler.userdata = unique_box::from(state{this, co}).release();
        handler.vtable   = &vtable;
        op               = AM_FWD(em).connect(unique_handler(NEO_MOVE(handler)));
        op.start();
    }

    void await_suspend(std::coroutine_handle<> co) noexcept {
        op = AM_FWD(em).connect([co, this](status st, unique_box&& val) {
            ret = emitter_result(st, NEO_MOVE(val));
            co.resume();
        });
        op.start();
    }
};

template <typename Alloc>
struct emitter_promise_alloc;

template <>
struct emitter_promise_alloc<std::allocator<void>> {};

template <>
struct emitter_promise_alloc<loop_allocator<>> {
    struct alloc_state {
        amongoc_loop* loop;
        alignas(std::max_align_t) char tail[1];
    };

    void* operator new(std::size_t n, amongoc_loop& loop, auto&&...) {
        // Pilfer a pointer to the loop within the allocated region
        auto storage = loop_allocator<char>(loop).allocate(n + sizeof(alloc_state));
        auto p       = new (storage) alloc_state{&loop};
        // Return the tail of the struct as the storage for the coroutine
        return p->tail;
    }
    void* operator new(std::size_t n, amongoc_loop* loop, auto&&...) {
        return operator new(n, *loop);
    }

    void operator delete(void* p, std::size_t n) {
        // Adjust the pointer to coroutine storage back to the pointer to alloc_state
        // that we created in new()
        auto base = (alloc_state*)(static_cast<char*>(p) - offsetof(alloc_state, tail));
        loop_allocator<char>(*base->loop)
            .deallocate(reinterpret_cast<char*>(base), n + sizeof(alloc_state));
    }
};

/**
 * @brief Coroutine promise for amongoc_emitter-base coroutines
 */
template <typename Alloc>
struct emitter_promise : emitter_promise_alloc<Alloc> {
    // The handler for the emitter coroutine
    unique_handler fin_handler;
    // Result storage for the coroutine
    emitter_result fin_result;

    using emitter_promise::emitter_promise_alloc::emitter_promise_alloc;

    // Coroutine handle type
    using co_handle = std::coroutine_handle<emitter_promise>;

    // We have a stop token based on the stop functionality on the associated handler
    stoppable_token auto query(get_stop_token_fn q) const noexcept { return q(fin_handler); }

    // The final suspend will invoke the final handler with the coroutine's result
    static auto final_suspend() noexcept {
        return suspends_by([](co_handle co) noexcept {
            emitter_promise& self = co.promise();
            AM_FWD(self.fin_handler)
                .complete(self.fin_result.status, NEO_MOVE(self.fin_result.value));
        });
    }
    // Always start suspended
    static std::suspend_always initial_suspend() noexcept { return {}; }

    /**
     * @brief Transform an awaited nanosender
     *
     * @param s A nanosender that is being awaited
     * @return nanosender_awaitable<S, emitter_promise> The new awaitable adaptor
     */
    template <nanosender S>
    nanosender_awaitable<S, emitter_promise> await_transform(S&& s) noexcept {
        return {NEO_FWD(s)};
    }

    emitter_awaitable await_transform(emitter&& em) noexcept {
        return emitter_awaitable{NEO_MOVE(em)};
    }

    // Starter function object for the amongoc_operation
    struct starter {
        AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true);
        unique_co_handle<emitter_promise> _co;

        void operator()() const { _co.resume(); }
    };

    // Connector function object for the amongoc_emitter
    struct connector {
        AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true);
        unique_co_handle<emitter_promise> _co;

        unique_operation operator()(unique_handler hnd) noexcept {
            _co.promise().fin_handler = NEO_MOVE(hnd);
            return unique_operation::from_starter(starter{NEO_MOVE(_co)});
        }
    };

    // Create an amongoc_emitter from the coroutine handle and promise
    amongoc_emitter get_return_object() noexcept {
        auto co = co_handle::from_promise(*this);
        static_assert(box_inlinable_type<connector>);
        return unique_emitter::from_connector(connector{unique_co_handle(NEO_MOVE(co))}).release();
    }

    void unhandled_exception() {
        try {
            throw;
        } catch (std::system_error const& err) {
            return_value(err.code());
        }
    }

    void return_value(std::nullptr_t) noexcept { return_value(emitter_result()); }

    void return_value(unique_box&& b) noexcept { return_value(emitter_result(0, NEO_MOVE(b))); }
    void return_value(status st) noexcept { return_value(emitter_result(st)); }
    void return_value(emitter_result r) noexcept { fin_result = NEO_MOVE(r); }
    void return_value(std::error_code ec) noexcept { return_value(status::from(ec)); }
};

/**
 * @brief A coroutine return type that presents as a nanosender
 *
 * @tparam T The result type of the coroutine
 */
template <typename T>
class co_task {
public:
    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true);
    /**
     * @brief Coroutine acts as a sender by sending the result type
     */
    using sends_type = T;

    /**
     * @brief Connect the coroutine to a receiver that receives the return value of
     * the coroutine
     *
     * @param recv The receiver of the operation
     *
     * The resulting operation will enqueue the coroutine on the associated event loop.
     * When the coroutine completes, the receiver will be enqueued on the event loop.
     */
    template <nanoreceiver_of<T> R>
    nanooperation auto connect(R&& recv) && noexcept {
        assert(_co);
        return operation<R>{NEO_FWD(recv), NEO_MOVE(_co)};
    }

private:
    static constexpr bool supports_exceptions
        = requires(std::exception_ptr e) { T::from_exception(e); };
    // Abstract base that defines the continuation when connected to a receiver
    struct finisher_base {
        // Callback when the coroutine returns
        virtual void on_return(T&&) noexcept = 0;
        void         on_exception(std::exception_ptr e) noexcept {
            try {
                if constexpr (supports_exceptions) {
                    on_return(T::from_exception(e));
                } else {
                    std::rethrow_exception(e);
                }
            } catch (std::exception const& ex) {
                std::fprintf(stderr,
                             "Attempted to throw an unsupported exception from within a co_task<> "
                                     "that does not support it: what(): %s\n",
                             ex.what());
                std::fflush(stderr);
                std::terminate();
            }
        }
        // Get the stop token for the coroutine
        virtual in_place_stop_token stop_token() const noexcept = 0;
    };

public:
    /// Coroutine promise type for the co_task
    struct promise_type {
        std::optional<T>   _return_value;
        std::exception_ptr _exception;
        finisher_base*     _recv;

        static auto final_suspend() noexcept {
            return suspends_by([](std::coroutine_handle<promise_type> co) {
                promise_type& self = co.promise();
                if (self._recv) {
                    if (self._exception) {
                        self._recv->on_exception(self._exception);
                    } else {
                        self._recv->on_return(NEO_MOVE(*self._return_value));
                    }
                }
            });
        }

        template <nanosender S>
        nanosender_awaitable<S, promise_type> await_transform(S&& s) noexcept {
            return {NEO_FWD(s)};
        }

        emitter_awaitable await_transform(emitter&& em) noexcept {
            return emitter_awaitable{NEO_MOVE(em)};
        }

        in_place_stop_token query(get_stop_token_fn) const noexcept {
            // This should only be called after _recv has been connected
            assert(_recv);
            return _recv->stop_token();
        }

        // Always start suspended
        std::suspend_always initial_suspend() const noexcept { return {}; }

        // Create the returned co_task
        co_task<T> get_return_object() noexcept {
            return co_task<T>(co_handle::from_promise(*this));
        }

        void unhandled_exception() noexcept { _exception = std::current_exception(); }

        // Emplace the return value in the return storage
        template <std::convertible_to<T> U>
        void return_value(U&& u) noexcept {
            _return_value.emplace(NEO_FWD(u));
        }
    };

private:
    using co_handle = std::coroutine_handle<promise_type>;

    explicit co_task(co_handle&& co) noexcept
        : _co(NEO_MOVE(co)) {}

    unique_co_handle<promise_type> _co;

    /**
     * @brief Nanooperation used when the coroutine is used as a nanosender
     *
     * @tparam R The receiver of the coroutine result
     */
    template <nanoreceiver_of<T> R>
    struct operation {
        explicit operation(R&& recv, unique_co_handle<promise_type>&& co) noexcept
            : _co(NEO_MOVE(co))
            , _launcher(NEO_FWD(recv)) {}

        struct finisher : finisher_base {
            explicit finisher(R&& r) noexcept
                : _recv(NEO_FWD(r)) {}

            R                    _recv;
            in_place_stop_source _stopper;

            stop_forwarder<R, in_place_stop_source> _stop_fwd{_recv, _stopper};

            void                on_return(T&& x) noexcept override { NEO_FWD(_recv)(NEO_FWD(x)); }
            in_place_stop_token stop_token() const noexcept override {
                return _stopper.get_token();
            }
        };

        unique_co_handle<promise_type> _co;
        finisher                       _launcher;

        void start() noexcept {
            // Attach the receiver continuation to the coroutine's promise
            _co.promise()._recv = &_launcher;
            // Do the initial resume of the coroutine now.
            _co.resume();
        }
    };
};

}  // namespace amongoc

template <typename... Ts>
struct std::coroutine_traits<amongoc_emitter, Ts...> {
    using promise_type = amongoc::emitter_promise<std::allocator<void>>;
};

template <typename... Ts>
struct std::coroutine_traits<amongoc_emitter, amongoc_loop*, Ts...> {
    using promise_type = amongoc::emitter_promise<amongoc::loop_allocator<>>;
};

template <typename... Ts>
struct std::coroutine_traits<amongoc_emitter, amongoc_loop&, Ts...> {
    using promise_type = amongoc::emitter_promise<amongoc::loop_allocator<>>;
};
