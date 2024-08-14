#pragma once

#include "./box.h"
#include "./config.h"
#include "./emitter_result.h"
#include "./status.h"

#ifdef __cplusplus
namespace amongoc {

class unique_handler;

}  // namespace amongoc
#endif

typedef struct amongoc_handler amongoc_handler;

/**
 * @brief Virtual method table for amongoc_handler objects
 *
 */
struct amongoc_handler_vtable {
    void (*complete)(amongoc_box userdata, amongoc_status st, amongoc_box value) AMONGOC_NOEXCEPT;
    amongoc_box (*register_stop)(amongoc_view hnd_userdata,
                                 void*        userdata,
                                 void (*callback)(void*)) AMONGOC_NOEXCEPT;
};

/**
 * @brief A asynchronous operation handler object, used with amongoc_emitter to
 * generate an asynchronous operation chain.
 */
struct amongoc_handler {
    // Arbitrary userdata for the handler, owned by the handler
    amongoc_box userdata;
    // Virtual method table
    const struct amongoc_handler_vtable* vtable;

#ifdef __cplusplus
    /// Transfer ownership of the handler into a unique_handler
    inline amongoc::unique_handler as_unique() && noexcept;
#endif
};

AMONGOC_EXTERN_C_BEGIN

/**
 * @brief Resolve a handler with the given result
 *
 * @param recv The handler to be resolved. This call takes ownership of the handler.
 * @param st The result status for the operation
 * @param result The result value for the operation. This call takes ownership of that value
 */
static inline void
amongoc_complete(amongoc_handler recv, amongoc_status st, amongoc_box result) AMONGOC_NOEXCEPT {
    // Invoke the callback. The callback takes ownership of the userdata and the result value
    recv.vtable->complete(recv.userdata, st, result);
}

/**
 * @brief Discard and destroy a handler that is otherwise unused.
 *
 * @param hnd The handler to be discarded
 *
 * @note This function should not be used on a handler that was consumed by another operation.
 */
static inline void amongoc_handler_discard(amongoc_handler hnd) AMONGOC_NOEXCEPT {
    amongoc_box_destroy(hnd.userdata);
}

/**
 * @brief Register a stop callback with the given handler
 *
 * @param hnd The handler to be registered with
 * @param userdata Arbitrary userdata pointer
 * @param callback The stop callback
 * @return A stop registration token that should be destroyed to un-register the stop callback
 *
 * If the given handler has no stop callback registration, then this function is a no-op
 */
static inline amongoc_box amongoc_register_stop(const amongoc_handler* hnd,
                                                void*                  userdata,
                                                void (*callback)(void*)) AMONGOC_NOEXCEPT {
    if (hnd->vtable->register_stop) {
        return hnd->vtable->register_stop(hnd->userdata.view, userdata, callback);
    }
    return amongoc_nothing;
}

AMONGOC_EXTERN_C_END

#ifdef __cplusplus
namespace amongoc {

// forward-decl from nano/result.hpp
template <typename T, typename E>
class result;

// forward-decl from nano/stop.hpp
struct get_stop_token_fn;

/**
 * @brief Provides a stoppable token based on an amongoc_handler
 */
class handler_stop_token {
public:
    // Construct a new stop token associated with the given handler object
    explicit handler_stop_token(const amongoc_handler& hnd) noexcept
        : _handler(&hnd) {}

    // Whether the associated handler has stop functionality
    bool stop_possible() const noexcept { return _handler->vtable->register_stop != nullptr; }

    /**
     * @brief NOTE: amongoc_handler only supports callback-based stopping, not stateful stopping
     *
     * @return false Always returns false
     */
    constexpr bool stop_requested() const noexcept { return false; }

    // Default-compare based on the identity of the backing handler
    bool operator==(const handler_stop_token&) const = default;

    /**
     * @brief Stop-callback type for the stop token
     *
     * @tparam F The stop handler function
     */
    template <typename F>
    class callback_type {
    public:
        explicit callback_type(handler_stop_token self, F&& fn) noexcept
            : _fn(AM_FWD(fn))
            , _reg_cookie(amongoc_register_stop(self._handler, this, _do_stop)) {}

        // The address of this object is part of its identity. Prevent it from moving
        callback_type(callback_type&&) = delete;

    private:
        // The C-style stop callback that is registered with the handler.
        static void _do_stop(void* cb) noexcept {
            // Invoke the attached callback
            static_cast<callback_type*>(cb)->_fn();
        }

        // The function object to be called
        F _fn;
        // The stop registration cookie
        unique_box _reg_cookie;
    };

private:
    // The handler for this token
    const amongoc_handler* _handler;
};

/**
 * @brief Unique ownership wrapper for an `::amongoc_handler`
 */
struct unique_handler {
public:
    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true);
    // Default-contsruct to nothing
    unique_handler() = default;
    // Take ownership of a handler object
    explicit unique_handler(amongoc_handler&& h) noexcept {
        _handler = h;
        h        = {};
    }

    // Move-construct
    unique_handler(unique_handler&& o) noexcept
        : _handler(o.release()) {}

    // Move-assign
    unique_handler& operator=(unique_handler&& o) noexcept {
        amongoc_handler_discard(_handler);
        _handler = o.release();
        return *this;
    }

    // Discard the associated handler object
    ~unique_handler() {
        // This is a no-op if the handler has been released
        amongoc_handler_discard(_handler);
    }

    /**
     * @brief Relinquish ownership of the underlying `::amongoc_handler` and return it
     */
    [[nodiscard]] amongoc_handler release() noexcept {
        auto h   = _handler;
        _handler = {};
        return h;
    }

    /**
     * @brief Resolve the handler with the given status and value.
     *
     * @param st The result status
     * @param result The result value
     *
     * This method is &&-qualified to signify that it consumes the object
     */
    void complete(amongoc_status st, unique_box&& result) && noexcept {
        // The callback takes ownership of the handler and the result
        amongoc_complete(release(), st, result.release());
    }

    /// Allow invocation with an emitter_result, implementing nanoreceiver<emitter_result>
    void operator()(emitter_result&& r) && noexcept {
        amongoc_complete(release(), r.status, r.value.release());
    }

    /// Allow invocation with a result<T, E>, implementing nanoreceiver<result<T, E>>
    template <typename T, typename E>
    void operator()(result<T, E> res) && {
        if (res.has_value()) {
            amongoc_complete(release(),
                             amongoc_okay,
                             unique_box::from(AM_FWD(res).value()).release());
        } else {
            // NOTE: This expects that status::from() is valid with the error type of the result.
            // The result's default error is std::error_code, so this should work for most result
            // objects.
            amongoc_complete(release(), status::from(res.error()), amongoc_nothing);
        }
    }

    /// Allow invocation with nullptr, implementing nanoreceiver<std::nullptr_t>
    void operator()(decltype(nullptr)) && {
        amongoc_complete(release(), amongoc_okay, amongoc_nothing);
    }

    /**
     * @brief Register a stop callback with the handler. @see `amongoc_register_stop`
     */
    [[nodiscard]] unique_box register_stop(void* userdata, void (*callback)(void*)) noexcept {
        return amongoc_register_stop(&_handler, userdata, callback).as_unique();
    }

    /// Test whether the handler is able to request a stop
    [[nodiscard]] bool stop_possible() const noexcept {
        return _handler.vtable->register_stop != nullptr;
    }

    /**
     * @brief Create a handler object that will invoke the given invocable
     * object when it is completed
     */
    template <typename F>
    static unique_handler from(F&& fn) noexcept {
        static_assert(
            requires(amongoc_status st, amongoc::unique_box b) {
                fn(st, (amongoc::unique_box&&)b);
            }, "The from() invocable must be callable as fn(status, unique_box&&)");
        // Wrapper type for the function object
        struct wrapped {
            AMONGOC_TRIVIALLY_RELOCATABLE_THIS(enable_trivially_relocatable_v<F>);
            // Wrapped function
            F func;
            // Call the underlying function
            void call(status st, unique_box&& value) { static_cast<F&&>(func)(st, AM_FWD(value)); }

            // Completion callback
            static void complete(box self, status st, box value) noexcept {
                AM_FWD(self)  //
                    .as_unique()
                    .as<wrapped>()
                    .call(st, AM_FWD(value).as_unique());
            }
        };
        static amongoc_handler_vtable vt = {.complete = &wrapped::complete};

        amongoc_handler ret;
        ret.userdata = unique_box::from(wrapped{AM_FWD(fn)}).release();
        ret.vtable   = &vt;
        return unique_handler(std::move(ret));
    }

    // Associate a stoppable token with the unique_handler
    handler_stop_token query(const get_stop_token_fn&) const noexcept {
        return handler_stop_token(_handler);
    }

private:
    amongoc_handler _handler{};
};

}  // namespace amongoc

amongoc::unique_handler amongoc_handler::as_unique() && noexcept {
    return amongoc::unique_handler((amongoc_handler&&)*this);
}
#endif