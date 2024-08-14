#pragma once

#include "./box.h"
#include "./config.h"
#include "./handler.h"
#include "./operation.h"
#include "./status.h"

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
 * @brief Create a "detached" operation from an emitter. This returns a simple operation
 * object that can be started. The final result from the emitter will simply be destroyed
 * when it resolves.
 */
amongoc_operation amongoc_detached_operation(amongoc_emitter emit) AMONGOC_NOEXCEPT;

/**
 * @brief Create an operation from an emitter which will store the final result
 * status and value of the emitter in the pointed-to locations
 *
 * @param em The emitter to be connected
 * @param status Storage destination for the output status (optional)
 * @param value Storage destination for the output result (optional)
 * @return amongoc_operation An operation than, when complete, will update `*status`
 * and `*value` with the result of the emitter.
 *
 * If either parameter is a null pointer, then the associated object will be ignored.
 *
 * @note The pointed-to locations must remain valid until the operation is complete or is destroyed
 */
amongoc_operation amongoc_emitter_tie_output(amongoc_emitter em,
                                             amongoc_status* status,
                                             amongoc_box*    value) AMONGOC_NOEXCEPT;

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
 * @brief Special type to be used to capture the result of an amongoc_emitter as a pair of status
 * and boxed value
 */
class emitter_result {
public:
    // Default-construct to an empty box and success
    emitter_result() noexcept
        : value(amongoc_nothing) {}

    // Construct with an empty box and the given status
    explicit emitter_result(amongoc_status st) noexcept
        : status(st)
        , value(amongoc_nothing) {}

    // Construct with the given status and an existing box
    explicit emitter_result(amongoc_status st, unique_box&& b) noexcept
        : status(st)
        , value(AM_FWD(b)) {}

    amongoc_status status{0};
    unique_box     value;
};

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
        : _emitter(other.release()) {}

    ~unique_emitter() { amongoc_emitter_discard(release()); }

    unique_emitter& operator=(unique_emitter&& other) noexcept {
        amongoc_emitter_discard(release());
        _emitter = other.release();
        return *this;
    }

    /**
     * @brief Relinquish ownership of the emitter and return it to the caller
     */
    [[nodiscard]] emitter release() noexcept {
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
    static unique_emitter from_connector(F&& fn) noexcept {
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
                return op.release();
            },
        };
        amongoc_emitter ret;
        ret.userdata = unique_box::from(wrapped{AM_FWD(fn)}).release();
        ret.vtable   = &vtable;
        return unique_emitter(AM_FWD(ret));
    }

    template <typename F>
        requires requires(F fn, status st, unique_box ub) {  //
            AM_FWD(fn)
            (st, AM_FWD(ub));
        }
    unique_operation connect(F fn) && noexcept {
        return ((unique_emitter&&)(*this)).connect(unique_handler::from(AM_FWD(fn)));
    }

    unique_operation connect(amongoc::unique_handler&& hnd) && noexcept {
        return amongoc_emitter_connect(release(), hnd.release()).as_unique();
    }

private:
    emitter _emitter{};
};

template <typename T>
struct nanosender_traits;

template <>
struct nanosender_traits<unique_emitter> {
    using sends_type = emitter_result;

    template <typename R>
    static unique_operation connect(unique_emitter&& em, R&& recv) noexcept {
        return AM_FWD(em).connect(as_handler(AM_FWD(recv)).as_unique());
    }
};

}  // namespace amongoc

amongoc::unique_emitter amongoc_emitter::as_unique() && noexcept {
    return amongoc::unique_emitter((amongoc_emitter&&)(*this));
}
#endif