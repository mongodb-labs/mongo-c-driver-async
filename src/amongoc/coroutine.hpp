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

// C library headers
#include <amongoc/alloc.h>
#include <amongoc/box.h>
#include <amongoc/emitter.h>
#include <amongoc/handler.h>
#include <amongoc/loop.h>
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
    requires has_allocator<Promise>
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
            self->_sent_value.emplace(AM_FWD(result));
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
    // Whether the attached nanosender is an immediate sender
    bool _is_immediate = amongoc::is_immediate(_sender);

    // If the sender is an immediate, do not suspend the coroutine at all.
    constexpr bool await_ready() const noexcept { return _is_immediate; }

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
    sends_t<S> await_resume() noexcept {
        if (_is_immediate) {
            // The nanosender would complete immediately, meaning await_suspend() was never called.
            // We need to connect the emitter and run the operation inline.
            amongoc::connect(std::forward<S>(_sender), [&](sends_t<S>&& value) {
                _sent_value.emplace(NEO_FWD(value));
            }).start();
        }
        return static_cast<sends_t<S>&&>(*_sent_value);
    }
};

struct coroutine_promise_allocator_mixin {
    struct alloc_state {
        cxx_allocator<char> alloc;
        alignas(std::max_align_t) char tail[1];
    };

    // Allocate the coroutine state using our amongoc_allocator
    template <typename T>
    void* operator new(std::size_t n, cxx_allocator<T> const& alloc_, const auto&...) {
        cxx_allocator<char> alloc   = alloc_;
        auto                storage = alloc.allocate(n + sizeof(alloc_state));
        // Pilfer a copy of the allocator within the allocated region
        auto p = new (storage) alloc_state{alloc};
        // Return the tail of the struct as the storage for the coroutine
        return p->tail;
    }

    // Adapt the C allocator to the C++ interface
    void* operator new(std::size_t n, const amongoc_allocator& alloc, const auto&...) {
        return operator new(n, cxx_allocator<>(alloc));
    }

    // Allocate using the allocator from the event loop
    void* operator new(std::size_t n, amongoc_loop& loop, auto const&...) {
        return operator new(n, loop.get_allocator());
    }
    void* operator new(std::size_t n, amongoc_loop* loop, auto const&...) {
        return operator new(n,
                            loop ? loop->get_allocator()
                                 : cxx_allocator<>{amongoc_default_allocator});
    }

    void operator delete(void* p, std::size_t n) {
        // Adjust the pointer to coroutine storage back to the pointer to alloc_state
        // that we created in new()
        auto base = (alloc_state*)(static_cast<char*>(p) - offsetof(alloc_state, tail));
        base->alloc.deallocate(reinterpret_cast<char*>(base), n + sizeof(alloc_state));
    }

    template <typename T>
    coroutine_promise_allocator_mixin(const cxx_allocator<T>& a, auto&&...)
        : alloc(a) {}

    coroutine_promise_allocator_mixin(const amongoc_allocator& a, auto&&...)
        : alloc(a) {}

    coroutine_promise_allocator_mixin(amongoc_loop& loop, auto&&...)
        : alloc(loop.get_allocator()) {}

    coroutine_promise_allocator_mixin(amongoc_loop* loop, auto&&...)
        : alloc(loop ? loop->get_allocator() : cxx_allocator<>{amongoc_default_allocator}) {}

    cxx_allocator<> alloc;

    cxx_allocator<> get_allocator() const noexcept { return alloc; }
};

/**
 * @brief Coroutine promise for amongoc_emitter-base coroutines
 */
struct emitter_promise : coroutine_promise_allocator_mixin {
    using coroutine_promise_allocator_mixin::coroutine_promise_allocator_mixin;

    // The handler for the emitter coroutine
    unique_handler fin_handler;
    // Result storage for the coroutine
    emitter_result fin_result;

    // Coroutine handle type
    using co_handle = std::coroutine_handle<emitter_promise>;

    // We have a stop token based on the stop functionality on the associated handler
    stoppable_token auto query(get_stop_token_fn q) const noexcept { return q(fin_handler); }

    // The final suspend will invoke the final handler with the coroutine's result
    static auto final_suspend() noexcept {
        return suspends_by([](co_handle co) noexcept {
            emitter_promise& self = co.promise();
            self.fin_handler.complete(self.fin_result.status, NEO_MOVE(self.fin_result.value));
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

        unique_operation operator()(unique_handler&& hnd) noexcept {
            _co.promise().fin_handler = AM_FWD(hnd);
            return unique_operation::from_starter(terminating_allocator, starter{NEO_MOVE(_co)});
        }
    };

    // Create an amongoc_emitter from the coroutine handle and promise
    amongoc_emitter get_return_object() noexcept {
        auto co = co_handle::from_promise(*this);
        static_assert(box_inlinable_type<connector>);
        return unique_emitter::from_connector(terminating_allocator,
                                              connector{unique_co_handle(NEO_MOVE(co))})
            .release();
    }

    void unhandled_exception() noexcept {
        try {
            throw;
        } catch (std::system_error const& err) {
            return_value(err.code());
        } catch (amongoc::exception const& err) {
            return_value(err.status());
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
        virtual std::coroutine_handle<> on_return(T&&) noexcept = 0;
        virtual std::coroutine_handle<> on_exception(std::exception_ptr e) noexcept {
            try {
                if constexpr (supports_exceptions) {
                    return on_return(T::from_exception(e));
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
    struct promise_type : coroutine_promise_allocator_mixin {
        // Inherit constructors
        using coroutine_promise_allocator_mixin::coroutine_promise_allocator_mixin;

        // Our handle type
        using co_handle = std::coroutine_handle<promise_type>;

        // Storage for the return value
        std::optional<neo::object_box<T>> _return_value;
        // A possible exception raised by the coroutine
        std::exception_ptr _exception;
        // Handles the completion of the coroutine. Set by connect() or nested_awaitable
        finisher_base* _finisher = nullptr;

        // Final suspend of the co_task
        static auto final_suspend() noexcept {
            return suspends_by(
                [](std::coroutine_handle<promise_type> co) -> std::coroutine_handle<> {
                    promise_type& self = co.promise();
                    // We will only be launched after a finisher is connected
                    assert(self._finisher);
                    if (self._exception) {
                        // Let the finisher decide how to handle the exception
                        return self._finisher->on_exception(self._exception);
                    } else {
                        // Regular return value. Note that we only pass an r-value reference,
                        // and it is up to the target to move-from the return value.
                        // nested_awaitable will not move from the return value and instead leaves
                        // it in place
                        return self._finisher->on_return(NEO_MOVE(*self._return_value).get());
                    }
                });
        }

        template <nanosender S>
        nanosender_awaitable<S, promise_type> await_transform(S&& s) noexcept {
            return {NEO_FWD(s)};
        }

        /**
         * @brief Special awaitable that handles nested awaiting of co_task
         *
         * This allows the propagation of exceptions between co_task, which is
         * not always possible when treating the co_task as a plain nanosender
         */
        template <typename U>
        struct nesting_awaitable {
            // The other task type
            using other_type = co_task<U>;
            // The other promise type
            using other_promise_type = std::coroutine_traits<other_type>::promise_type;
            // The other coroutine's handle type
            using other_handle_type = std::coroutine_handle<other_promise_type>;

            // The other co_task's abstract finisher base class
            using other_finisher_type = other_type::finisher_base;

            // Construct an awaitable from our own handle and ther other coroutine's handle
            nesting_awaitable(co_handle self, other_handle_type h)
                : _other_co(h)
                , _handoff(self) {}

            // Implementation of a finisher that will resume the parent coroutine when the child
            // completes
            struct task_handoff_finisher : other_finisher_type {
                explicit task_handoff_finisher(co_handle h)
                    : self(h) {}

                co_handle self;
                // Don't move-from the return value/exception: We'll do that during await_resume()
                std::coroutine_handle<> on_return(U&&) noexcept override {
                    // Return our own coroutine handle so that the child resumes us during its final
                    // suspend
                    return self;
                }
                std::coroutine_handle<> on_exception(std::exception_ptr) noexcept override {
                    return self;
                }
                // Forward our stop token
                in_place_stop_token stop_token() const noexcept override {
                    return self.promise().get_stop_token();
                }
            };

            // Handle to the coroutine we are awaiting
            other_handle_type _other_co;
            // The finisher that will resume us when the other coroutine finishes
            task_handoff_finisher _handoff;

            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> self) {
                // Connect our special resuming finisher with the child
                _other_co.promise()._finisher = &_handoff;
                // Tell the runtime to resume the child coroutine immediately.
                return _other_co;
            }

            U await_resume() const {
                if (_other_co.promise()._exception) {
                    // The child threw an exception. Re-throw it now
                    std::rethrow_exception(_other_co.promise()._exception);
                } else {
                    // Perfect-forward from the child's object_box
                    return NEO_MOVE(*_other_co.promise()._return_value).get();
                }
            }

            // We are never immediately ready
            static constexpr bool await_ready() noexcept { return false; }
        };

        template <typename U>
        nesting_awaitable<U> await_transform(co_task<U>&& other) noexcept {
            return nesting_awaitable<U>{co_handle::from_promise(*this), other._co.get()};
        }

        in_place_stop_token get_stop_token() const noexcept {
            // This should only be called after _finisher has been connected
            assert(_finisher);
            return _finisher->stop_token();
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
    // Allow other task's to access our internal state
    template <typename>
    friend class co_task;

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
            , _recv_invoker(NEO_FWD(recv)) {}

        struct recv_finisher : finisher_base {
            explicit recv_finisher(R&& r) noexcept
                : _recv(NEO_FWD(r)) {}

            R                    _recv;
            in_place_stop_source _stopper;

            stop_forwarder<R, in_place_stop_source> _stop_fwd{_recv, _stopper};

            std::coroutine_handle<> on_return(T&& x) noexcept override {
                NEO_FWD(_recv)(NEO_FWD(x));
                return std::noop_coroutine();
            }
            in_place_stop_token stop_token() const noexcept override {
                return _stopper.get_token();
            }
        };

        unique_co_handle<promise_type> _co;
        recv_finisher                  _recv_invoker;

        void start() noexcept {
            // Attach the receiver continuation to the coroutine's promise
            _co.promise()._finisher = &_recv_invoker;
            // Do the initial resume of the coroutine now.
            _co.resume();
        }
    };
};

}  // namespace amongoc

template <typename... Ts>
struct std::coroutine_traits<amongoc_emitter, Ts...> {
    using promise_type = amongoc::emitter_promise;
};
