#pragma once

#include <amongoc/box.hpp>
#include <amongoc/emitter.h>
#include <amongoc/handler.hpp>
#include <amongoc/operation.hpp>

namespace amongoc {

using emitter = ::amongoc_emitter;

/**
 * @brief A move-only wrapper around `amongoc_emitter`
 */
struct unique_emitter : mlib::unique<::amongoc_emitter> {
    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true, unique_emitter);
    using unique_emitter::unique::unique;

    /**
     * @brief Create an amongoc_emitter from an object that is invocable with a unique_handler
     *
     * @param fn The function object, which must return a `unique_operation` when invoked with
     * a `unique_handler` argument
     * @return unique_emitter An emitter that owns the associated connector object
     */
    template <typename F>
    static unique_emitter from_connector(mlib::allocator<> alloc, F&& fn)
        noexcept(box_inlinable_type<F>) {
        // Wrap the connector in an object, preserving reference semantics
        struct wrapped {
            AMONGOC_TRIVIALLY_RELOCATABLE_THIS(enable_trivially_relocatable_v<F>, wrapped);
            F _fn;
        };

        // The vtable for this object.
        static amongoc_emitter_vtable vtable = {
            .connect = [](amongoc_box self_, amongoc_handler recv) -> amongoc_operation {
                // Invoke the wrapped function object. Wrap the handler in a unique_handler
                wrapped          self = mlib_fwd(self_).as_unique().take<wrapped>();
                unique_handler   h    = mlib_fwd(recv).as_unique();
                unique_operation op   = mlib_fwd(self)._fn(mlib_fwd(h));
                return mlib_fwd(op).release();
            },
        };
        amongoc_emitter ret;
        ret.userdata = unique_box::from(alloc, wrapped{mlib_fwd(fn)}).release();
        ret.vtable   = &vtable;
        return unique_emitter(mlib_fwd(ret));
    }

    unique_operation bind_allocator_connect(mlib::allocator<> a, auto&& fn) && {
        return static_cast<unique_emitter&&>(*this).connect(unique_handler::from(a, mlib_fwd(fn)));
    }

    unique_operation connect(amongoc::unique_handler&& hnd) && {
        ::amongoc_emitter self = static_cast<unique_emitter&&>(*this).release();
        ::amongoc_handler h    = mlib_fwd(hnd).release();
        return amongoc_emitter_connect(self, h).as_unique();
    }

    template <std::size_t... Sz, typename F>
    auto compress(F&& fn) &&;
};

template <typename T>
struct nanosender_traits;

template <>
struct nanosender_traits<unique_emitter> {
    using sends_type = emitter_result;

    // XXX: This relys on delayed lookup for as_handler(), which is defined in a
    // private header. Move this whole specialization to a private header?
    template <typename R>
    static unique_operation connect(unique_emitter&& em, R&& recv) {
        return mlib_fwd(em).connect(as_handler(mlib_fwd(recv)));
    }

    // Special: We receive a handler directly, no need to convert it to a C handler
    static unique_operation connect(unique_emitter&& em, unique_handler&& hnd) {
        return mlib_fwd(em).connect(mlib_fwd(hnd));
    }

    static constexpr bool is_immediate(unique_emitter const&) noexcept { return false; }
};

}  // namespace amongoc

amongoc::unique_emitter amongoc_emitter::as_unique() && noexcept {
    return amongoc::unique_emitter((amongoc_emitter&&)(*this));
}
