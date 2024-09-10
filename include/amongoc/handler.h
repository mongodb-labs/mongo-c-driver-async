#pragma once

#include "./alloc.h"
#include "./box.h"
#include "./emitter_result.h"
#include "./status.h"

#include <mlib/config.h>

#if mlib_is_cxx()
namespace amongoc {

class unique_handler;

}  // namespace amongoc
#endif

typedef struct amongoc_handler amongoc_handler;

/**
 * @brief Virtual method table for amongoc_handler objects
 */
struct amongoc_handler_vtable {
    // Call the completion callback on the handler
    void (*complete)(amongoc_handler* handler, amongoc_status st, amongoc_box value) mlib_noexcept;
    // Register a stop callback with the handler (optional)
    amongoc_box (*register_stop)(amongoc_handler const* handler,
                                 void*                  userdata,
                                 void (*callback)(void*)) mlib_noexcept;
    // Obtain the allocator associated with a handler (optional)
    mlib_allocator (*get_allocator)(amongoc_handler const* handler,
                                    mlib_allocator         dflt) mlib_noexcept;
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

#if mlib_is_cxx()
    /// Transfer ownership of the handler into a unique_handler
    inline amongoc::unique_handler as_unique() && noexcept;

    /**
     * @brief Resolve the handler with the given status and value.
     *
     * @param st The result status
     * @param result The result value
     */
    void complete(amongoc_status st, amongoc::unique_box&& result) & noexcept {
        // The callback takes ownership of the handler and the result
        this->vtable->complete(this, st, mlib_fwd(result).release());
    }

    mlib::allocator<> get_allocator() const noexcept {
        if (vtable->get_allocator) {
            return mlib::allocator<>(vtable->get_allocator(this, ::mlib_default_allocator));
        }
        return mlib::allocator<>(::mlib_default_allocator);
    }
#endif
};

mlib_extern_c_begin();

/**
 * @brief Resolve a handler with the given result
 *
 * @param recv The handler to be resolved. This call takes ownership of the handler.
 * @param st The result status for the operation
 * @param result The result value for the operation. This call takes ownership of that value
 */
static inline void amongoc_handler_complete(amongoc_handler* hnd,
                                            amongoc_status   st,
                                            amongoc_box      result) mlib_noexcept {
    // Invoke the callback. The callback takes ownership of the userdata and the result value
    hnd->vtable->complete(hnd, st, result);
}

/**
 * @brief Destroy a handler object
 *
 * @note This function should not be used on a handler that was consumed by another operation.
 */
static inline void amongoc_handler_destroy(amongoc_handler hnd) mlib_noexcept {
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
static inline amongoc_box amongoc_handler_register_stop(const amongoc_handler* hnd,
                                                        void*                  userdata,
                                                        void (*callback)(void*)) mlib_noexcept {
    if (hnd->vtable->register_stop) {
        return hnd->vtable->register_stop(hnd, userdata, callback);
    }
    return amongoc_nil;
}

/**
 * @brief Obtain the allocator associated with the given handler.
 *
 * @param hnd The handle to be queried
 * @param dflt The allocator that should be returned in case the handler does not provide an
 * allocator
 */
static inline mlib_allocator amongoc_handler_get_allocator(const amongoc_handler* hnd,
                                                           mlib_allocator dflt) mlib_noexcept {
    if (hnd->vtable->get_allocator) {
        return hnd->vtable->get_allocator(hnd, dflt);
    }
    return dflt;
}

mlib_extern_c_end();

#if mlib_is_cxx()
namespace amongoc {

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
            : _fn(mlib_fwd(fn))
            , _reg_cookie(amongoc_handler_register_stop(self._handler, this, _do_stop)) {}

        // The address of this object is part of its identity. Prevent it from moving
        callback_type(callback_type&&) = delete;

    private:
        // The C-style stop callback that is registered with the handler.
        static void _do_stop(void* cb) noexcept {
            // Invoke the attached callback
            static_cast<callback_type*>(cb)->_fn();
        }

        // The function object to be called
        [[no_unique_address]] F _fn;
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
        : _handler(mlib_fwd(o).release()) {}

    // Move-assign
    unique_handler& operator=(unique_handler&& o) noexcept {
        amongoc_handler_destroy(_handler);
        _handler = mlib_fwd(o).release();
        return *this;
    }

    // Discard the associated handler object
    ~unique_handler() {
        // This is a no-op if the handler has been released
        amongoc_handler_destroy(_handler);
    }

    /**
     * @brief Relinquish ownership of the underlying `::amongoc_handler` and return it
     */
    [[nodiscard]] amongoc_handler release() && noexcept {
        auto h   = _handler;
        _handler = {};
        return h;
    }

    /**
     * @brief Resolve the handler with the given status and value.
     *
     * @param st The result status
     * @param result The result value
     */
    void complete(amongoc_status st, unique_box&& result) & noexcept {
        // The callback takes ownership of the handler and the result
        _handler.complete(st, mlib_fwd(result));
    }

    /// Allow invocation with an emitter_result, implementing nanoreceiver<emitter_result>
    void operator()(emitter_result&& r) noexcept { complete(r.status, mlib_fwd(r).value); }

    /**
     * @brief Register a stop callback with the handler. @see `amongoc_register_stop`
     */
    [[nodiscard]] unique_box register_stop(void* userdata, void (*callback)(void*)) noexcept {
        return amongoc_handler_register_stop(&_handler, userdata, callback).as_unique();
    }

    /// Test whether the handler is able to request a stop
    [[nodiscard]] bool stop_possible() const noexcept {
        return _handler.vtable->register_stop != nullptr;
    }

    // Obtain the stop token for this handler
    handler_stop_token get_stop_token() const noexcept { return handler_stop_token(_handler); }

    // Obtain the allocator associated with the handler.
    allocator<> get_allocator() const noexcept {
        return allocator<>(::amongoc_handler_get_allocator(&_handler, ::mlib_default_allocator));
    }

    /**
     * @brief Create a handler object that will invoke the given invocable
     * object when it is completed
     */
    template <typename F>
    static unique_handler from(allocator<> alloc, F&& fn) noexcept(box_inlinable_type<F>) {
        using wrapper_type = wrapper<F>;
        wrapper_type wr{alloc, mlib_fwd(fn)};

        amongoc_handler ret;
        ret.userdata = unique_box::from(alloc, mlib_fwd(wr)).release();
        ret.vtable   = &wrapper_type::vtable;
        return unique_handler(mlib_fwd(ret));

        static_assert(
            requires(emitter_result res) { fn(mlib_fwd(res)); },
            "The from() invocable must be callable as fn(emitter_result&&)");
    }

private:
    amongoc_handler _handler{};

    // Implement the wrapper for invocable objects, used by from()
    template <typename R>
    struct wrapper {
        allocator<>             _alloc;
        [[no_unique_address]] R _fn;

        static void _complete(amongoc_handler* self, status st, box result) noexcept {
            auto& fn = self->userdata.view.as<wrapper>()._fn;
            static_cast<R&&>(fn)(emitter_result(st, unique_box(mlib_fwd(result))));
        }

        static ::mlib_allocator _get_allocator(amongoc_handler const* self,
                                               ::mlib_allocator) noexcept {
            allocator<> a = self->userdata.view.as<wrapper>()._alloc;
            return a.c_allocator();
        }

        static constexpr amongoc_handler_vtable vtable = {
            .complete      = &_complete,
            .get_allocator = &_get_allocator,
        };
    };

    template <typename R>
        requires mlib::has_mlib_allocator<R>
    struct wrapper<R> {
        explicit wrapper(allocator<>, R&& r)
            : _fn(mlib_fwd(r)) {}

        [[no_unique_address]] R _fn;

        static void _complete(amongoc_handler* self, status st, box result) noexcept {
            auto& fn = self->userdata.view.as<wrapper>()._fn;
            static_cast<R&&>(fn)(emitter_result(st, unique_box(mlib_fwd(result))));
        }

        static ::mlib_allocator _get_allocator(amongoc_handler const* self,
                                               ::mlib_allocator) noexcept {
            allocator<> a = mlib::get_allocator(self->userdata.view.as<wrapper>()._fn);
            return a.c_allocator();
        }

        static constexpr amongoc_handler_vtable vtable = {
            .complete      = &_complete,
            .get_allocator = &_get_allocator,
        };
    };
};

}  // namespace amongoc

amongoc::unique_handler amongoc_handler::as_unique() && noexcept {
    return amongoc::unique_handler((amongoc_handler&&)*this);
}
#endif