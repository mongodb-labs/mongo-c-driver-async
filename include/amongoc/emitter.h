#pragma once

#include "./box.h"
#include "./config.h"
#include "./emitter_result.h"
#include "./handler.h"
#include "./operation.h"
#include "./status.h"
#include "amongoc/alloc.h"

#ifdef __cplusplus
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

#ifdef __cplusplus
    inline amongoc::unique_emitter as_unique() && noexcept;
#endif
};

AMONGOC_EXTERN_C_BEGIN

/**
 * @brief Connect an emitter to a handler to produce a new operation state
 *
 * @note This function consumes the emitter and handler objects
 *
 * @param emit The emitter that is being connected
 * @param hnd The handler for the operation
 */
static inline amongoc_operation amongoc_emitter_connect(amongoc_emitter emit,
                                                        amongoc_handler hnd) AMONGOC_NOEXCEPT {
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
static inline void amongoc_emitter_discard(amongoc_emitter emit) AMONGOC_NOEXCEPT {
    amongoc_box_destroy(emit.userdata);
}

AMONGOC_EXTERN_C_END

#ifdef __cplusplus
namespace amongoc {

using emitter = ::amongoc_emitter;

/**
 * @brief A move-only wrapper around `amongoc_emitter`
 */
class unique_emitter {
public:
    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true);
    unique_emitter() = default;

    explicit unique_emitter(emitter&& em)
        : _emitter(em) {
        em = {};
    }

    unique_emitter(unique_emitter&& other) noexcept
        : _emitter(AM_FWD(other).release()) {}

    ~unique_emitter() { amongoc_emitter_discard(((unique_emitter&&)*this).release()); }

    unique_emitter& operator=(unique_emitter&& other) noexcept {
        amongoc_emitter_discard(((unique_emitter&&)*this).release());
        _emitter = AM_FWD(other).release();
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
    static unique_emitter from_connector(cxx_allocator<> alloc, F&& fn) noexcept {
        // Wrap the connector in an object, preserving reference semantics
        struct wrapped {
            AMONGOC_TRIVIALLY_RELOCATABLE_THIS(enable_trivially_relocatable_v<F>);
            F _fn;
        };

        // The vtable for this object.
        static amongoc_emitter_vtable vtable = {
            .connect = [](amongoc_box userdata, amongoc_handler recv) -> amongoc_operation {
                // Invoke the wrapped function object. Wrap the handler in a unique_handler
                unique_operation op = static_cast<F&&>(
                    AM_FWD(userdata).as_unique().as<wrapped>()._fn)(AM_FWD(recv).as_unique());
                return AM_FWD(op).release();
            },
        };
        amongoc_emitter ret;
        ret.userdata = unique_box::from(alloc, wrapped{AM_FWD(fn)}).release();
        ret.vtable   = &vtable;
        return unique_emitter(AM_FWD(ret));
    }

    template <typename F>
        requires requires(F fn, status st, unique_box ub) {  //
            AM_FWD(fn)
            (st, AM_FWD(ub));
        }
    unique_operation connect(cxx_allocator<> a, F fn) && noexcept {
        return ((unique_emitter&&)(*this)).connect(unique_handler::from(a, AM_FWD(fn)));
    }

    unique_operation connect(amongoc::unique_handler&& hnd) && noexcept {
        return amongoc_emitter_connect(((unique_emitter&&)*this).release(), AM_FWD(hnd).release())
            .as_unique();
    }

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
    static unique_operation connect(unique_emitter&& em, R&& recv) noexcept {
        // TODO: A custom allocator here for as_handler?
        return AM_FWD(em).connect(
            as_handler(cxx_allocator<>{amongoc_default_allocator}, AM_FWD(recv)));
    }

    // Special: We receive a handler directly, no need to convert it to a C handler
    static unique_operation connect(unique_emitter&& em, unique_handler&& hnd) noexcept {
        return AM_FWD(em).connect(AM_FWD(hnd));
    }
};

}  // namespace amongoc

amongoc::unique_emitter amongoc_emitter::as_unique() && noexcept {
    return amongoc::unique_emitter((amongoc_emitter&&)(*this));
}
#endif
