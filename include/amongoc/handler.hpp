#pragma once

#include <amongoc/box.hpp>
#include <amongoc/emitter_result.hpp>
#include <amongoc/handler.h>
#include <amongoc/relocation.hpp>

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
struct unique_handler : mlib::unique<::amongoc_handler> {
    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true, unique_handler);
    using unique_handler::unique::unique;

    // Test whether this handler has a value
    constexpr bool has_value() const noexcept { return get().vtable != nullptr; }

    /**
     * @brief Resolve the handler with the given status and value.
     *
     * @param st The result status
     * @param result The result value
     */
    void complete(amongoc_status st, unique_box&& result) & noexcept {
        // The callback takes ownership of the handler and the result
        ::amongoc_handler_complete(&get(), st, mlib_fwd(result).release());
    }

    /// Allow invocation with an emitter_result, implementing nanoreceiver<emitter_result>
    void operator()(emitter_result&& r) noexcept { complete(r.status, mlib_fwd(r).value); }

    /**
     * @brief Register a stop callback with the handler. @see `amongoc_handler_register_stop`
     */
    [[nodiscard]] unique_box register_stop(void* userdata, void (*callback)(void*)) noexcept {
        return amongoc_handler_register_stop(&get(), userdata, callback).as_unique();
    }

    /// Test whether the handler is able to request a stop
    [[nodiscard]] bool stop_possible() const noexcept {
        return get().vtable->register_stop != nullptr;
    }

    // Obtain the stop token for this handler
    handler_stop_token get_stop_token() const noexcept { return handler_stop_token(get()); }

    // Obtain the allocator associated with the handler.
    mlib::allocator<> get_allocator() const noexcept {
        return ::amongoc_handler_get_allocator(&get(), ::mlib_default_allocator);
    }

    /**
     * @brief Create a handler object that will invoke the given invocable
     * object when it is completed
     */
    template <typename F>
    static unique_handler from(mlib::allocator<> alloc, F&& fn) noexcept(box_inlinable_type<F>) {
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

    /**
     * @brief Create a handler that invokes the given invocable when it completes.
     *
     * @param fn The invocable. Must have an associated allocator
     */
    template <typename F>
        requires mlib::has_mlib_allocator<F>
    static unique_handler from(F&& fn) noexcept(box_inlinable_type<F>) {
        auto a = mlib::get_allocator(fn);
        return from(a, mlib_fwd(fn));
    }

private:
    // Implement the wrapper for invocable objects, used by from()
    template <typename R>
    struct wrapper {
        mlib::allocator<>       _alloc;
        [[no_unique_address]] R _fn;
        AMONGOC_TRIVIALLY_RELOCATABLE_THIS(amongoc::enable_trivially_relocatable_v<R>, wrapper);

        static void _complete(amongoc_handler* self, status st, box result) noexcept {
            auto& fn = self->userdata.view.as<wrapper>()._fn;
            static_cast<R&&>(fn)(emitter_result(st, unique_box(mlib_fwd(result))));
        }

        static ::mlib_allocator _get_allocator(amongoc_handler const* self,
                                               ::mlib_allocator) noexcept {
            mlib::allocator<> a = self->userdata.view.as<wrapper>()._alloc;
            return a.c_allocator();
        }

        static constexpr amongoc_handler_vtable vtable = {
            .complete      = &_complete,
            .register_stop = nullptr,
            .get_allocator = &_get_allocator,
        };
    };

    template <typename R>
        requires mlib::has_mlib_allocator<R>
    struct wrapper<R> {
        explicit wrapper(mlib::allocator<>, R&& r)
            : _fn(mlib_fwd(r)) {}

        [[no_unique_address]] R _fn;
        AMONGOC_TRIVIALLY_RELOCATABLE_THIS(amongoc::enable_trivially_relocatable_v<R>, wrapper);

        static void _complete(amongoc_handler* self, status st, box result) noexcept {
            auto& fn = self->userdata.view.as<wrapper>()._fn;
            static_cast<R&&>(fn)(emitter_result(st, unique_box(mlib_fwd(result))));
        }

        static ::mlib_allocator _get_allocator(amongoc_handler const* self,
                                               ::mlib_allocator) noexcept {
            mlib::allocator<> a = mlib::get_allocator(self->userdata.view.as<wrapper>()._fn);
            return a.c_allocator();
        }

        static constexpr amongoc_handler_vtable vtable = {
            .complete      = &_complete,
            .register_stop = nullptr,
            .get_allocator = &_get_allocator,
        };
    };
};

}  // namespace amongoc

amongoc::unique_handler amongoc_handler::as_unique() && noexcept {
    return amongoc::unique_handler((amongoc_handler&&)*this);
}
