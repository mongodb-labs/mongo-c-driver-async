#pragma once

#include "./box.h"
#include "./emitter_result.h"
#include "./handler.h"
#include "./operation.h"
#include "./status.h"

#include <amongoc/alloc.h>

#include <mlib/config.h>

#if mlib_is_cxx()
namespace amongoc {

class unique_emitter;

}  // namespace amongoc
#endif

struct amongoc_emitter_vtable {
    /**
     * @brief Function pointer to connect an emitter object to a handler
     *
     * @param userdata The amongoc_emitter::userdata object from the associated emitter
     * @param handler The amongoc_handler that is being connected
     * @return amongoc_operation The newly connected operation.
     */
    amongoc_operation (*connect)(amongoc_box userdata, amongoc_handler handler);
};

typedef struct amongoc_emitter amongoc_emitter;
struct amongoc_emitter {
    // V-table for methods of this object
    struct amongoc_emitter_vtable const* vtable;
    // The userdata associated with the object
    amongoc_box userdata;

#if mlib_is_cxx()
    inline amongoc::unique_emitter as_unique() && noexcept;
#endif
};

mlib_extern_c_begin();

/**
 * @brief Connect an emitter to a handler to produce a new operation state
 *
 * @note This function consumes the emitter and handler objects
 *
 * @param emit The emitter that is being connected
 * @param hnd The handler for the operation
 */
static inline amongoc_operation amongoc_emitter_connect(amongoc_emitter emit,
                                                        amongoc_handler hnd) mlib_noexcept {
    return emit.vtable->connect(emit.userdata, hnd);
}

/**
 * @brief Discard and destroy an emitter that is otherwise unused
 *
 * @param em The emitter to be discarded.
 *
 * @note This function should not be used on an emitter object that was consumed by another
 * operation.
 */
static inline void amongoc_emitter_discard(amongoc_emitter emit) mlib_noexcept {
    amongoc_box_destroy(emit.userdata);
}

mlib_extern_c_end();

#if mlib_is_cxx()
namespace amongoc {

using emitter = ::amongoc_emitter;

/**
 * @brief A move-only wrapper around `amongoc_emitter`
 */
class unique_emitter {
public:
    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true);
    explicit unique_emitter(emitter&& em)
        : _emitter(em) {
        em = {};
    }

    unique_emitter(unique_emitter&& other) noexcept
        : _emitter(mlib_fwd(other).release()) {}

    ~unique_emitter() { amongoc_emitter_discard(((unique_emitter&&)*this).release()); }

    unique_emitter& operator=(unique_emitter&& other) noexcept {
        amongoc_emitter_discard(((unique_emitter&&)*this).release());
        _emitter = mlib_fwd(other).release();
        return *this;
    }

    /**
     * @brief Relinquish ownership of the emitter and return it to the caller
     */
    [[nodiscard]] emitter release() && noexcept {
        auto e   = _emitter;
        _emitter = {};
        return e;
    }

    /**
     * @brief Create an amongoc_emitter from an object that is invocable with a unique_handler
     *
     * @param fn The function object, which must return a `unique_operation` when invoked with
     * a `unique_handler` argument
     * @return unique_emitter An emitter that owns the associated connector object
     */
    template <typename F>
    static unique_emitter from_connector(allocator<> alloc, F&& fn)
        noexcept(box_inlinable_type<F>) {
        // Wrap the connector in an object, preserving reference semantics
        struct wrapped {
            AMONGOC_TRIVIALLY_RELOCATABLE_THIS(enable_trivially_relocatable_v<F>);
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

    unique_operation bind_allocator_connect(allocator<> a, auto&& fn) && {
        return static_cast<unique_emitter&&>(*this).connect(unique_handler::from(a, mlib_fwd(fn)));
    }

    unique_operation connect(amongoc::unique_handler&& hnd) && {
        ::amongoc_emitter self = static_cast<unique_emitter&&>(*this).release();
        ::amongoc_handler h    = mlib_fwd(hnd).release();
        return amongoc_emitter_connect(self, h).as_unique();
    }

    template <std::size_t... Sz, typename F>
    auto compress(F&& fn) &&;

private:
    emitter _emitter{};
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
#endif
