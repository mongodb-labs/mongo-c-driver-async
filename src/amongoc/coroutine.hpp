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
#include "./nano/stop.hpp"
#include "./nano/util.hpp"

#include <mlib/alloc.h>
#include <mlib/object_t.hpp>

// C library headers
#include <amongoc/alloc.h>
#include <amongoc/async.h>
#include <amongoc/box.h>
#include <amongoc/emitter.h>
#include <amongoc/handler.h>
#include <amongoc/loop.h>
#include <amongoc/nano/query.hpp>
#include <amongoc/nano/result.hpp>
#include <amongoc/operation.h>
#include <amongoc/status.h>

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <exception>
#include <system_error>
#include <type_traits>
#include <utility>

namespace amongoc {

template <typename T>
class co_task;

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
 * await_resume() returns `void`. await_ready() always returns `false`
 */
template <typename F>
struct suspends_by {
    mlib_no_unique_address F _fn;

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
 * @brief An awaiter type for nanosenders
 *
 * The awaiter will connect the nanosender to an internal receiver and launch
 * the composed operation during suspension of the enclosing coroutine. When
 * the nanosender completes, the coroutine wil be resumed immediately.
 *
 * This will skip suspension if it is detected that the operation will complete
 * immediately.
 *
 * @tparam S The type of the nanosender that we are awaiting.
 */
template <nanosender S>
class nanosender_awaiter {
public:
    // Construct an awaiter for a nanosender type
    explicit nanosender_awaiter(S&& s)
        : _sender(mlib_fwd(s)) {}

private:
    // The wrapped nanosender
    mlib_no_unique_address S _sender;

    // Whether the nanosender will complete immediately during `start()`
    // We can use this to skip suspension of the caller.
    bool _is_immediate = amongoc::is_immediate(_sender);

    // Storage for the value that was sent from the nanosender
    std::optional<mlib::object_t<sends_t<S>>> _sent_value;

    // A receiver for the nanosender operation state, templated on the promise type of the awaiting
    // coroutine.
    template <typename Promise>
    struct receiver {
        // Pointer to the owning awaiter
        nanosender_awaiter* self;

        // Handle to the coroutine that suspended for the operation
        std::coroutine_handle<Promise> suspender;

        // Forward queries to the promise (allocators, stop tokens, etc.)
        auto query(valid_query_for<Promise> auto q) const noexcept {
            return q(suspender.promise());
        }

        // Handle the result from the nanosender:
        void operator()(sends_t<S>&& result) {
            // Emplace the result into storage
            self->_sent_value.emplace(mlib_fwd(result));
            // Immediately resume the coroutine that we suspended:
            suspender.resume();
        }
    };

    // A type-erased box that holds the operation state.
    unique_box _operation_state = amongoc_nil.as_unique();

public:
    constexpr bool await_ready() const noexcept {
        // If the sender will complete immediately, then we can skip suspension
        // of the caller.
        return _is_immediate;
    }

    sends_t<S> await_resume() noexcept {
        if (_is_immediate) {
            // The nanosender would complete immediately, meaning await_suspend() was never called.
            // We need to connect the nanosender and run the operation inline.
            amongoc::connect(std::forward<S>(_sender),
                             // XXX: Should this be a terminating allocator?
                             mlib::bind_allocator(mlib::terminating_allocator,
                                                  [&](sends_t<S>&& value) {
                                                      _sent_value.emplace(mlib_fwd(value));
                                                  }))
                .start();
        }
        return static_cast<sends_t<S>&&>(*_sent_value);
    }

    // Perform suspension. Connects the nanosender to a receiver and launches the operation
    template <typename Promise>
    void await_suspend(std::coroutine_handle<Promise> suspender) noexcept {
        // The type of operation state for the given promise
        using operation_type = connect_t<S, receiver<Promise>>;
        // Connect the sender and receiver in our type-erased box
        _operation_state = unique_box::make<operation_type>(  //
            mlib::get_allocator(suspender.promise()),
            defer_convert([&] -> operation_type {
                return amongoc::connect(static_cast<S&&>(_sender),
                                        receiver<Promise>{this, suspender});
            }));
        // Immediately launch the operation
        _operation_state.as<operation_type>().start();
    }
};

/**
 * @brief A `co_await` operator for nanosenders. This allows any `nanosender` to
 * be used as the operand of a `co_await` expression.
 *
 * @param s The nanosender being awaited
 */
template <nanosender S>
nanosender_awaiter<S> operator co_await(S && s) {
    return nanosender_awaiter<S>{mlib_fwd(s)};
}

/**
 * @brief Mixin base class for coroutine promises that will automatically
 * handle dynamic allocation and allocator association for amongoc coroutines
 */
class coroutine_promise_allocator_mixin {
public:
    // Allocate the coroutine state using our mlib_allocator
    void* operator new(std::size_t n, allocator<char> const& alloc, const auto&...) noexcept {
        char* storage;
        try {
            storage = alloc.allocate(n + sizeof(alloc_state));
        } catch (std::bad_alloc) {
            // Allocation failure. Signal to the runtime that allocation failed
            // by returning a null pointer
            return nullptr;
        }
        // Pilfer a copy of the allocator within the allocated region
        auto p = new (storage) alloc_state{alloc};
        // Return the tail of the struct as the storage for the coroutine
        return p->tail;
    }

    // Allocate using the allocator from the event loop
    void* operator new(std::size_t n, amongoc_loop* loop, auto const&...) noexcept {
        return operator new(n, loop ? loop->get_allocator() : allocator<>{mlib_default_allocator});
    }

    // Allocate where the first parameter provides an allocator
    void*
    operator new(std::size_t n, mlib::has_mlib_allocator auto const& x, auto const&...) noexcept {
        return operator new(n, mlib::get_allocator(x));
    }

    void operator delete(void* p, std::size_t n) {
        // Adjust the pointer to coroutine storage back to the pointer to alloc_state
        // that we created in new()
        auto base = (alloc_state*)(static_cast<char*>(p) - offsetof(alloc_state, tail));
        base->alloc.deallocate(reinterpret_cast<char*>(base), n + sizeof(alloc_state));
    }

    coroutine_promise_allocator_mixin(amongoc_loop* loop, auto&&...)
        : _alloc(loop ? loop->get_allocator() : allocator<>{mlib_default_allocator}) {}

    coroutine_promise_allocator_mixin(const allocator<>& a, auto&&...)
        : _alloc(a) {}

    template <mlib::has_mlib_allocator X>
    coroutine_promise_allocator_mixin(const X& x, auto&&...)
        : _alloc(amongoc::get_allocator(x)) {}

    // Expose the allocator associated with the coroutine
    allocator<> get_allocator() const noexcept { return _alloc; }

private:
    allocator<> _alloc;

    struct alloc_state {
        allocator<char> alloc;
        alignas(std::max_align_t) char tail[1];
    };
};

/**
 * @brief Coroutine promise for amongoc_emitter-based coroutines
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
    handler_stop_token get_stop_token() const noexcept { return fin_handler.get_stop_token(); }

    // The final suspend will invoke the final handler with the coroutine's result
    static auto final_suspend() noexcept {
        // Note: Don't complete the handler within final_suspend directly. We want
        // to complete the handler after the coroutine suspends.
        return suspends_by([](co_handle co) noexcept {
            emitter_promise& self = co.promise();
            // This should be the final substantial line of code, because it is possible that the
            // handler will destroy the coroutine promise during completion.
            self.fin_handler.complete(self.fin_result.status, std::move(self.fin_result.value));
        });
    }
    // Always start suspended
    static std::suspend_always initial_suspend() noexcept { return {}; }

    // Starter function object for the amongoc_operation
    struct starter {
        AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true);
        unique_co_handle<emitter_promise> _co;

        void operator()(amongoc_handler& h) const {
            // Move the handler from the operation to the coroutine
            _co.promise().fin_handler = std::move(h).as_unique();
            _co.resume();
        }
    };

    // Connector function object for the amongoc_emitter
    struct connector {
        AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true);
        unique_co_handle<emitter_promise> _co;

        unique_operation operator()(unique_handler&& hnd) noexcept {
            return unique_operation::from_starter(mlib_fwd(hnd), starter{std::move(_co)});
        }
    };

    // Create an amongoc_emitter from the coroutine handle and promise
    amongoc_emitter get_return_object() noexcept {
        auto co = co_handle::from_promise(*this);
        static_assert(box_inlinable_type<connector>);
        return unique_emitter::from_connector(mlib::terminating_allocator,
                                              connector{unique_co_handle(std::move(co))})
            .release();
    }

    static amongoc_emitter get_return_object_on_allocation_failure() noexcept {
        return amongoc_alloc_failure();
    }

    void unhandled_exception() noexcept {
        try {
            throw;
        } catch (std::system_error const& err) {
            return_value(err.code());
        } catch (amongoc::exception const& err) {
            return_value(err.status());
        } catch (std::bad_alloc) {
            return_value(emitter_result(amongoc_status(&amongoc_generic_category, ENOMEM)));
        }
    }

    void return_value(std::nullptr_t) noexcept { return_value(emitter_result()); }

    void return_value(unique_box&& b) noexcept { return_value(emitter_result(0, std::move(b))); }
    void return_value(status st) noexcept { return_value(emitter_result(st)); }
    void return_value(std::error_code ec) noexcept { return_value(status::from(ec)); }
    void return_value(emitter_result r) noexcept { fin_result = std::move(r); }
};

/**
 * @brief A generic coroutine return type that works with stop tokens an our
 * custom allocator, and can also be used as a nanosender
 *
 * @tparam T The success type of the coroutine
 */
template <typename T>
class co_task {
public:
    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true);

    /**
     * @brief The variant result type of the coroutine. Represents either a
     * successful return value or an unhandled exception.
     *
     * This is most useful for the nanosender interface. `co_await`-ing a
     * co_task will yield only the success type `T` or re-throw an enclosed
     * exception.
     */
    using result_type = amongoc::result<T, std::exception_ptr>;

private:
    // Abstract base that defines the continuation behavior of the coroutine
    struct finisher_base {
        // Callback during final coroutine suspension
        virtual std::coroutine_handle<> on_final_suspend() noexcept = 0;
        // Get the stop token for the coroutine
        virtual in_place_stop_token stop_token() const noexcept = 0;
    };

public:
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    /**
     * @brief Nanosender type for a coroutine.
     *
     * A coroutine's sender will send an instance of result_type, which encodes
     * either a successful return value, or a handle to an uncaught exception.
     */
    class sender {
    public:
        using sends_type = result_type;

        template <nanoreceiver_of<sends_type> R>
        nanooperation auto connect(R&& recv) && noexcept {
            return operation<R>{mlib_fwd(recv), std::move(_co)};
        }

    private:
        template <typename R>
        class operation : public finisher_base {
        public:
            explicit operation(R&& r, unique_co_handle<promise_type>&& co)
                : _recv(mlib_fwd(r))
                , _coroutine(mlib_fwd(co)) {
                _coroutine.promise()._finisher = this;
            }

            // The operation state must be stable since we give away a pointer to it.
            operation(operation&&) = delete;

            // Launch the coroutine
            void start() noexcept { _coroutine.resume(); }

        private:
            // The receiver for the operation
            mlib_no_unique_address R       _recv;
            unique_co_handle<promise_type> _coroutine;

            std::coroutine_handle<> on_final_suspend() noexcept override {
                // Pass the final result to the receiver
                mlib::invoke(static_cast<R&&>(_recv),
                             static_cast<result_type&&>(_coroutine.promise().got_result));
                // Do not resume another coroutine
                return std::noop_coroutine();
            }

            virtual in_place_stop_token stop_token() const noexcept override {
                if constexpr (has_stop_token<R>) {
                    // XXX: This assumes that the stop token is convertible to
                    // an in_place_stop_token, which may not be true.
                    return amongoc::get_stop_token(_recv);
                }
                return {};
            }
        };

        friend co_task;
        explicit sender(unique_co_handle<promise_type>&& co) noexcept
            : _co(mlib_fwd(co)) {}

        unique_co_handle<promise_type> _co;
    };

    /**
     * @brief Convert co_task to a `nanosender` that sends the result of
     * executing the coroutine.
     */
    sender as_sender() && noexcept { return sender{std::move(_co)}; }

    /**
     * @brief The awaiter used for awaiting a `co_task`
     */
    struct awaiter {
        // Construct an awaiter from a handle to the awaited coroutine
        awaiter(handle_type self)
            : _self(self) {}

        awaiter(awaiter&&) = delete;

        // Implementation of a finisher that will resume the parent coroutine when the child
        // completes
        template <typename Promise>
        struct task_handoff_finisher : finisher_base {
            explicit task_handoff_finisher(std::coroutine_handle<Promise> c) noexcept
                : caller(c) {}
            // Handle to the parent coroutine
            std::coroutine_handle<Promise> caller;
            // A stop source for this coroutine
            in_place_stop_source _stop_src;
            // Forward stop requests from the parent coroutine
            stop_forwarder<Promise&, in_place_stop_source> _stop_fwd{caller.promise(), _stop_src};
            // Upon final suspend, resume the caller
            std::coroutine_handle<> on_final_suspend() noexcept override { return caller; }
            // Forward our stop token
            in_place_stop_token stop_token() const noexcept override {
                if constexpr (has_stop_token<Promise>) {
                    return _stop_src.get_token();
                }
                return {};
            }
        };

        // Optimize: If the promise type exposes an in-place stop token directly, don't
        // use a stop_forwarder.
        template <typename Promise>
            requires std::convertible_to<get_stop_token_t<Promise>, in_place_stop_token>
        struct task_handoff_finisher<Promise> : finisher_base {
            explicit task_handoff_finisher(std::coroutine_handle<Promise> c) noexcept
                : caller(c) {}

            /// This object does not need a stable address, so we can trivially relocate it.
            /// This will enable the inline-box optimization for `_handoff`
            AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true);

            // Handle to the parent coroutine
            std::coroutine_handle<Promise> caller;
            // Upon final suspend, resume the caller
            std::coroutine_handle<> on_final_suspend() noexcept override { return caller; }
            // Forward the stop token directly from the promise
            in_place_stop_token stop_token() const noexcept override {
                return amongoc::get_stop_token(caller.promise());
            }
        };

        // Handle to the coroutine that is being awaited
        handle_type _self;
        // The finisher that will resume us when the other coroutine finishes. This is type-erased
        // because we the finisher is templated on the awaiting promise type.
        unique_box _handoff = amongoc_nil.as_unique();

        template <typename Promise>
            requires mlib::has_mlib_allocator<Promise>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> parent) {
            // Connect our special resuming finisher with the child
            using fin = task_handoff_finisher<Promise>;
            // XXX: We may be able to optimize a dynamic allocation away since we can guarantee
            // that the finisher object will not be moved after it is constructed.
            _handoff = unique_box::make<fin>(mlib::get_allocator(parent.promise()), parent);
            _self.promise()._finisher = &_handoff.as<fin>();
            // Tell the runtime to launch the child coroutine immediately
            return _self;
        }

        T await_resume() const {
            result_type& r = _self.promise().got_result;
            if (r.has_error()) {
                // The child threw an exception. Re-throw it now
                std::rethrow_exception(r.error());
            } else {
                // Perfect-forward from the child's return value
                return static_cast<T&&>(r.value());
            }
        }

        // We are never immediately ready
        static constexpr bool await_ready() noexcept { return false; }
    };

    awaiter operator co_await() noexcept { return awaiter{_co.get()}; }

    /// Coroutine promise type for the co_task
    struct promise_type : coroutine_promise_allocator_mixin {
        // Inherit constructors
        using coroutine_promise_allocator_mixin::coroutine_promise_allocator_mixin;

        // Storage for the final result. Will hold either a value or an exception
        result_type got_result = amongoc::error(std::exception_ptr());

        // Handles the completion of the coroutine.
        finisher_base* _finisher = nullptr;

        // Final suspend of the co_task
        static auto final_suspend() noexcept {
            return suspends_by(
                [](std::coroutine_handle<promise_type> co) -> std::coroutine_handle<> {
                    promise_type& self = co.promise();
                    // We will only be launched after a finisher is connected
                    assert(self._finisher);
                    return self._finisher->on_final_suspend();
                });
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
            return co_task<T>(handle_type::from_promise(*this));
        }

        // If we fail to allocate, throw bad_alloc() immediately
        [[noreturn]]
        static co_task<T> get_return_object_on_allocation_failure() {
            throw std::bad_alloc();
        }

        void unhandled_exception() noexcept {
            this->got_result.emplace_error(std::current_exception());
        }

        // Emplace the return value in the return storage
        template <std::convertible_to<T> U>
        void return_value(U&& u) noexcept {
            this->got_result.emplace_value(mlib_fwd(u));
        }

        void return_value(T&& item) noexcept { this->got_result.emplace_value(mlib_fwd(item)); }
    };

private:
    explicit co_task(handle_type&& co) noexcept
        : _co(std::move(co)) {}

    unique_co_handle<promise_type> _co;
};

}  // namespace amongoc

// Tell the compiler how to convert an `amongoc_emitter` function into a coroutine
template <typename... Ts>
struct std::coroutine_traits<amongoc_emitter, Ts...> {
    using promise_type = amongoc::emitter_promise;
};
