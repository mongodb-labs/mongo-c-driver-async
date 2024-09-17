/**
 * @file stop.hpp
 * @brief Implementation of stop-token based cancellation
 *
 * The interfaces defined here are based on those in P2300
 */
#pragma once

#include "./concepts.hpp"
#include "./query.hpp"

#include <amongoc/nano/util.hpp>

#include <mlib/object_t.hpp>

#include <neo/concepts.hpp>
#include <neo/like.hpp>

#include <atomic>
#include <cassert>
#include <concepts>
#include <thread>

namespace amongoc {

namespace detail {

template <template <class H> class Tk>
struct check_has_type_alias {};

}  // namespace detail

/**
 * @brief Match a type that provides stop token semantics. Generalizes std::stop_token
 */
template <typename Token>
concept stoppable_token =  //
    requires(const Token tok) {
        typename detail::check_has_type_alias<Token::template callback_type>;
        { tok.stop_requested() } noexcept -> std::same_as<bool>;
        { tok.stop_possible() } noexcept -> std::same_as<bool>;
        { Token(tok) } noexcept;
    }  //
    and std::copyable<Token>             //
    and std::equality_comparable<Token>  //
    and std::swappable<Token>;

/**
 * @brief Match a type that provides stop source semantics. Generalizes std::stop_source
 */
template <typename Src>
concept stoppable_source = requires(Src src, const Src csrc) {
    { csrc.get_token() } -> stoppable_token;
    { csrc.stop_possible() } noexcept -> std::same_as<bool>;
    { csrc.stop_requested() } noexcept -> std::same_as<bool>;
    { src.request_stop() } -> std::same_as<bool>;
};

/**
 * @brief Obtain the stop callback type associate with a stoppable token and a
 * stop handler
 *
 * @tparam Token A stoppable token type
 * @tparam F The handler function invoked during request_stop()
 */
template <stoppable_token Token, mlib::invocable<> F>
using stop_callback_t = Token::template callback_type<F>;

/**
 * @brief Query tag and function object that returns the stop token associated
 * with the given object
 */
inline constexpr struct get_stop_token_fn {
    constexpr stoppable_token auto operator()(const auto& x) const noexcept
        requires requires {
            { x.query(*this) } -> stoppable_token;
        } and (not requires { x.get_stop_token(); })
    {
        return x.query(*this);
    }

    constexpr stoppable_token auto operator()(const auto& x) const noexcept
        requires requires {
            { x.get_stop_token() } -> stoppable_token;
        }
    {
        return x.get_stop_token();
    }
} get_stop_token;

/**
 * @brief Match a type that has a stop token query method that returns a stoppable token
 */
template <typename S>
concept has_stop_token = requires(const S s) { get_stop_token(s); };

// Get the stop token type associated with an object
template <has_stop_token R>
using get_stop_token_t = decltype(get_stop_token(std::declval<const R&>()));

/**
 * @brief Create a stop callback object from the associated token and stop handler
 *
 * @param token The stop token for the associated stop state
 * @param fn The stop handler that will be invoked during request_stop()
 */
template <stoppable_token Token, mlib::invocable<> F>
constexpr stop_callback_t<Token, F> create_stop_callback(Token token, F&& fn) {
    return stop_callback_t<Token, F>(token, mlib_fwd(fn));
}

template <typename Fn>
class in_place_stop_callback;
class in_place_stop_source;
class in_place_stop_token;

/**
 * @brief Provides a zero-allocation immovable stop source for operation cancellation.
 *
 * This is based on the stop source of the same name defined in stdexec
 */
class in_place_stop_source {
public:
    in_place_stop_source()                       = default;
    in_place_stop_source(in_place_stop_source&&) = delete;

    /**
     * @brief Get a new token for this stop source
     */
    inline in_place_stop_token get_token() const noexcept;

    /**
     * @brief Request that associated operations stop
     *
     * @return true If a stop was successfully issued
     * @return false Otherwise (i.e. the state has already been stopped)
     */
    bool request_stop() noexcept {
        // Double-checked lock
        if (not _try_lock_unless_stopped(true)) {
            // We are already stopped (and no lock was taken)
            return false;
        }
        // We are the stopping thread
        _stopping_thread = std::this_thread::get_id();
        while (this->_head_callback) {
            // Pop the callback from the front of the list
            auto cb = this->_head_callback;
            // Disconnect the prev-pointer
            cb->_prev_nextptr = nullptr;
            // Point our head to the next callback
            this->_head_callback = cb->_next;
            // Point the head callback to our local head pointer
            if (this->_head_callback) {
                this->_head_callback->_prev_nextptr = &this->_head_callback;
            }

            // Notify all other threads and future callers that a stop was requested,
            // and unlock the spin locks
            _set_state(stopped);

            bool was_removed_during_exec          = false;
            cb->_did_remove_self_during_execution = &was_removed_during_exec;
            // Execute the callback
            cb->do_execute();
            if (not was_removed_during_exec) {
                cb->_did_remove_self_during_execution = nullptr;
                // This thread did not attempt to remove the callback during execution,
                // but other threads may have attempted to do so. Signal to them that
                // it is safe to proceed. (see `_unregister` for an explaination)
                cb->_exec_done.store(true);
            }

            // Spin lock again
            _lock();
        }

        _set_state(stopped);
        return true;
    }

    /**
     * @return true If the stop source has already been asked to stop
     * @return false Otherwise
     */
    bool stop_requested() const noexcept { return (this->_state.load() & stopped) == stopped; }
    /// Returns `true` for in_place_stop_source`
    bool stop_possible() const noexcept { return true; }

private:
    enum state_t { idle = 0, locked = 1, stopped = 2 };
    mutable std::atomic<state_t> _state{idle};

    // Set the stop state without any locking or spinning
    void _set_state(state_t restore_state) const noexcept { _state.store(restore_state); }

    /**
     * @brief Take an exclusive lock on the stop state. Returns the prior state to be
     * used with _unlock()
     */
    state_t _lock() const noexcept {
        auto prev_state = _state.load();
        do {
            // Spin until we are not locked
            while (prev_state & locked) {
                std::this_thread::yield();
                prev_state = _state.load();
            }
        } while (not _state.compare_exchange_weak(prev_state, state_t(prev_state | locked)));
        return prev_state;
    }

    /**
     * @brief Try to take an exclusive lock *unless* a stop has been requested.
     *
     * @param do_set_stopped If `true`, set the `stopped` flag on the internal state
     * @return true If the lock was taken (implied: No stop was requested)
     * @return false If no lock was taken (implied: A stop was requested)
     */
    bool _try_lock_unless_stopped(bool do_set_stopped) const noexcept {
        // Target state value based on whether we want to set the stopped flag:
        const auto target_state = do_set_stopped ? state_t(locked | stopped) : locked;
        // The current internal state
        auto cur_state = _state.load();
        // Spin until we are not locked
        do {
            while (cur_state != idle) {
                // We are locked (or stopping)
                if ((cur_state & stopped)) {
                    // We are already stopped. Do not lock
                    return false;
                } else {
                    assert(cur_state & locked);
                    // We are locked. Spin a while
                    std::this_thread::yield();
                    // Refresh the value
                    cur_state = _state.load();
                }
            }
        }
        // Loop until we set the target state
        while (not _state.compare_exchange_weak(cur_state, target_state));
        assert(_state.load() & locked);
        // We took our spin lock
        return true;
    }

public:
    struct stop_callback_base;

private:
    /// Head of linked-list of registered stop callbacks
    mutable stop_callback_base* _head_callback = nullptr;
    /// The ID of the thread that is executing request_stop()
    std::thread::id _stopping_thread;

    /// Allow the stop_callback_base class to register/unregister itself
    friend stop_callback_base;

    /**
     * @brief Try to register the callback with the stop state
     *
     * @param new_cb The callback to be registered
     * @return true If the callback was registered (implied: No stop was requested yet)
     * @return false If the callback was not registered (implied: A stop was already requested)
     */
    bool _try_register(stop_callback_base* new_cb) const {
        if (not _try_lock_unless_stopped(false)) {
            // A stop was already requested
            return false;
        }
        // We now hold the lock.
        // Set the next pointer to the head of our list
        new_cb->_next = this->_head_callback;
        // Set the back-pointer to refer to our head pointer
        new_cb->_prev_nextptr = &this->_head_callback;
        // Update the current head's back-pointer to refer to the callback's next pointer
        if (this->_head_callback) {
            this->_head_callback->_prev_nextptr = &new_cb->_next;
        }
        // Update our head pointer to refer to the new callback
        this->_head_callback = new_cb;
        // Unlock
        _set_state(idle);
        return true;
    }

    /**
     * @brief Unregister the given callback with the stop state
     *
     * @param cb The callback being unregistered
     *
     * This function is called during the destructor for the callback object.
     */
    void _unregister(stop_callback_base* cb) const noexcept {
        // Lock completely
        const auto prev_state = _lock();

        if (cb->_prev_nextptr) {
            // The callback has not been executed and is still part of the linked list.
            // Update the previous pointer to point to the next callback, splicing this
            // callback out of the list
            *cb->_prev_nextptr = cb->_next;
            if (cb->_next) {
                // Update the next callback in the list to the predecesor
                cb->_next->_prev_nextptr = cb->_prev_nextptr;
            }
            // Unlock
            _set_state(prev_state);
        } else {
            // The callback is currently executing
            auto stopping_thread = this->_stopping_thread;
            // Unlock
            _set_state(prev_state);
            if (std::this_thread::get_id() == stopping_thread) {
                // We are the thread that is requesting the stop.
                if (cb->_did_remove_self_during_execution) {
                    // Notify the stop state's request_stop that the callback attempted to
                    // remove itself while the stop callback was executing.
                    *cb->_did_remove_self_during_execution = true;
                }
                // We are about to destroy the callback object while it is ostensibly still
                // executing! This is safe as long as the stop handler itself does not attempt to
                // access any state stored within itself (akin to a `delete this`).
            } else {
                // The callback is running on another thread. We are executing
                // within the destructor of the callback, so we need to spin here
                // until execution finishes to keep the callback state alive during
                // shutdown.
                while (not cb->_exec_done.load()) {
                    // Spin until the callback is finished.
                    std::this_thread::yield();
                }
            }
        }
    }

public:
    /// Abstract base class for stop callback objects
    class stop_callback_base {
    protected:
        friend in_place_stop_source;
        /**
         * @brief The stop source of which this callback is associated
         */
        const in_place_stop_source* _src = nullptr;

        /**
         * @brief Pointer to the next callback in the linked list.
         *
         * This is non-null if the callback is part of the linked list. This
         * pointer is not set to null as part of execution/removal (see _prev_ptr)
         * instead
         */
        stop_callback_base* _next = nullptr;

        /**
         * @brief Pointer to the previous next-pointer in the linked list.
         *
         * This is non-null as long as the callback is still part of the list.
         * This pointer is set to null when it has been removed from the list,
         * including just before it is executed as part of a stop request
         */
        stop_callback_base** _prev_nextptr = nullptr;

        /// Pointer to a flag that indicates whether we were removed during callback execution by
        /// the same thread that is executing the callback
        bool* _did_remove_self_during_execution = nullptr;
        /// Have we finished execution?
        std::atomic_bool _exec_done{false};

        virtual ~stop_callback_base() = default;

        /**
         * @brief Try to register the callback with the stop state, or execute immediately
         *
         * @param src The stop state we are registering against
         *
         * If the stop state has already been stopped, then the registration will
         * be skipped and we will execute the callback immediately.
         *
         * This function is called within the constructor of derived classes
         */
        void _try_register_or_execute(const in_place_stop_source& src) noexcept {
            if (src._try_register(this)) {
                // Save our source, to be used to disconnect during destruction
                _src = &src;
            } else {
                // We did not register, meaning that the stop source was already
                // stopped. In this case, execute inline immediately.
                this->do_execute();
            }
        }

        /**
         * @brief Remove the callback from the stop state, preventing it from
         * being called in case of request_stop().
         *
         * This function is called in the destructor of derived classes. This
         * function is a no-op if the callback was not registered with the
         * stop state (which will be the case if the stop state was already stopped)
         */
        void _unregister() noexcept {
            if (_src) {
                _src->_unregister(this);
            }
        }

        /// Abstract: Execute the associated callback code
        virtual void do_execute() noexcept = 0;
    };
};

/**
 * @brief Stop token type returned from in_place_stop_source
 *
 */
class in_place_stop_token {
public:
    // Default-constructed is not associated with any stop state
    in_place_stop_token() = default;

    /// Get the callback type for this token
    template <typename Fn>
    using callback_type = in_place_stop_callback<Fn>;

    /// Returns `true` if the token has an associated stop-state
    constexpr bool stop_possible() const noexcept { return _src and _src->stop_possible(); }
    /// Returns `true` if we have a stop state and a stop has been requested
    constexpr bool stop_requested() const noexcept { return _src and _src->stop_requested(); }

    bool operator==(const in_place_stop_token&) const = default;

private:
    constexpr in_place_stop_token(const in_place_stop_source& src) noexcept
        : _src(&src) {}

    // Allow the stop source to call our private constructor
    friend in_place_stop_source;

    // Allow the stop callback to access our stop-source
    template <typename>
    friend class in_place_stop_callback;

    /// The stop-source associated with this token
    in_place_stop_source const* _src = nullptr;
};

in_place_stop_token in_place_stop_source::get_token() const noexcept {
    return in_place_stop_token(*this);
}

/**
 * @brief Stop callback implementation type for in-place stop sources.
 *
 * @tparam F The wrapped invocable type.
 */
template <typename F>
class in_place_stop_callback : in_place_stop_source::stop_callback_base {
public:
    /**
     * @brief Construct a new callback and register it with a stop state
     *
     * @param token The stop token associated with a stop state. If the token
     * is not associated with a stop-state, then this constructor is a no-op
     */
    template <neo::implicit_convertible_to<F> Arg>
    constexpr explicit in_place_stop_callback(in_place_stop_token token, Arg&& arg)
        : _func(mlib_fwd(arg)) {
        if (token._src) {
            this->_try_register_or_execute(*token._src);
        }
    }

    /// The stop callback is immobile
    in_place_stop_callback(in_place_stop_callback&&) = delete;

    // Call the unregister code during the destructor of the derived class so that
    // the members of this class are still alive during the unregistering process.
    ~in_place_stop_callback() { this->_unregister(); }

private:
    /// The wrapped invocable
    mlib_no_unique_address mlib::object_t<F> _func;

    /// Implement the invocation
    void do_execute() noexcept override { mlib::invoke(static_cast<F&&>(_func)); }
};

/**
 * @brief A stop token type that never has an associate stop state.
 *
 * Constructing a stop callback with this token is a no-op, and such a stop
 * callback will never be invoked.
 */
class null_stop_token {
public:
    constexpr bool stop_requested() const noexcept { return false; }
    constexpr bool stop_possible() const noexcept { return false; }

    template <typename H>
    struct callback_type {
        constexpr explicit callback_type(null_stop_token, const H&) noexcept {}
    };

    bool operator==(const null_stop_token&) const = default;
};

/**
 * @brief Attach a stop token to an object.
 *
 * The new object will have a stop token query.
 *
 * The returned object is invocable if the underlying object is invocable
 */
template <stoppable_token Token, typename Wrapped>
class bind_stop_token : public invocable_cvr_helper<bind_stop_token<Token, Wrapped>> {
    // The bound token
    Token _token;
    // The wrapped object
    mlib::object_t<Wrapped> _wrapped;

public:
    constexpr explicit bind_stop_token(Token tok, Wrapped&& fn)
        : _token(tok)
        , _wrapped(mlib_fwd(fn)) {}

    /// Obtain the wrapped object
    constexpr Wrapped&        base() & { return mlib::unwrap_object(_wrapped); }
    constexpr Wrapped&&       base() && { return mlib::unwrap_object(std::move(_wrapped)); }
    constexpr const Wrapped&  base() const& { return mlib::unwrap_object(_wrapped); }
    constexpr const Wrapped&& base() const&& { return mlib::unwrap_object(std::move(_wrapped)); }

    /// Get the bound stop token
    constexpr Token query(get_stop_token_fn) const noexcept { return _token; }

    /// Invoke the underlying object.
    static constexpr auto invoke(auto&& self, auto&&... args)
        MLIB_RETURNS(std::invoke(mlib_fwd(self)._wrapped, mlib_fwd(args)...));
};

template <typename Token, typename Wrapped>
explicit bind_stop_token(Token, Wrapped&&) -> bind_stop_token<Token, Wrapped>;

/**
 * @brief Get the "effective" stop token for an object
 *
 * @param h An object which may provide a stop token
 * @return A valid stop token for the object
 *
 * If the given argument does not provide a stop token, returns a null_stop_token
 * instead.
 */
template <typename H>
constexpr stoppable_token auto effective_stop_token(const H& h) noexcept {
    if constexpr (has_stop_token<H>) {
        return get_stop_token(h);
    } else {
        return null_stop_token{};
    }
}

template <typename T>
using effective_stop_token_t = decltype(effective_stop_token(std::declval<T const&>()));

/**
 * @brief An invocable object that will call request_stop() on a stop source when invoked
 */
template <stoppable_source S>
class stop_requester {
public:
    stop_requester() = default;
    constexpr explicit stop_requester(S&& src) noexcept
        : _src(mlib_fwd(src)) {}

    constexpr S&       get_stop_source() noexcept { return (S&)_src; }
    constexpr const S& get_stop_source() const noexcept { return (S&)_src; }

    constexpr auto get_token() const noexcept { return get_stop_source().get_token(); }

    constexpr void operator()() noexcept { get_stop_source().request_stop(); }
    constexpr void operator()() const noexcept { get_stop_source().request_stop(); }

private:
    mlib::object_t<S> _src;
};

/**
 * @brief An object that connects a stop-token provider with another stop source
 *
 * @tparam R An object that may provide a stop token
 * @tparam Src The stop source that is attached
 *
 * If the `R` type does not provide a stop token, then this object has no behavior.
 *
 * If a stop is requested from the stop token from R, then a stop request will
 * be issued on the attached stop source.
 */
template <typename R, stoppable_source Src>
class stop_forwarder {
public:
    constexpr explicit stop_forwarder(const R&, Src&) noexcept {}
};

template <has_stop_token R, stoppable_source Src>
class stop_forwarder<R, Src> {
public:
    constexpr explicit stop_forwarder(const R& r, Src& src) noexcept
        : _callback(get_stop_token(r), requester(src)) {}

private:
    using requester = stop_requester<Src&>;

    using stop_token_type = get_stop_token_t<R>;
    using callback_type   = stop_callback_t<stop_token_type, requester>;

    mlib_no_unique_address callback_type _callback;
};

}  // namespace amongoc
