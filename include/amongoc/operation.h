#pragma once

#include "./alloc.h"
#include "./box.h"
#include "./config.h"
#include "./handler.h"

AMONGOC_EXTERN_C_BEGIN

typedef struct amongoc_operation amongoc_operation;
typedef void (*amongoc_start_callback)(amongoc_operation* self) AMONGOC_NOEXCEPT;

AMONGOC_EXTERN_C_END

#ifdef __cplusplus
namespace amongoc {

class unique_operation;

}  // namespace amongoc
#endif

/**
 * @brief A pending asynchronous operation and continuation
 */
struct amongoc_operation {
    // Arbitrary userdata for the operation, managed by the operation
    amongoc_box userdata;
    // The handler that was attached to this operation
    amongoc_handler handler;
    // Callback used to start the operation. Prefer `amongoc_start`
    amongoc_start_callback start_callback;

#ifdef __cplusplus
    // Transfer the operation into a unique_operation object
    [[nodiscard]] inline amongoc::unique_operation as_unique() && noexcept;
#endif
};

/**
 * @brief Launch an asynchronous operation
 *
 * @param op Pointer to the operation to be started
 */
static inline void amongoc_start(amongoc_operation* op) AMONGOC_NOEXCEPT { op->start_callback(op); }

/**
 * @brief Destroy an asynchronous operation object
 *
 * @param op The operation to be destroyed
 */
static inline void amongoc_operation_destroy(amongoc_operation op) AMONGOC_NOEXCEPT {
    amongoc_handler_destroy(op.handler);
    amongoc_box_destroy(op.userdata);
}

#ifdef __cplusplus
namespace amongoc {

/**
 * @brief Provides managed ownership over an `amongoc_operation`
 */
class unique_operation {
public:
    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true);
    unique_operation() = default;

    // Take ownership of the given operation
    explicit unique_operation(amongoc_operation&& op) noexcept
        : _oper(op) {
        op = {};
    }

    unique_operation(unique_operation&& other) noexcept
        : _oper(AM_FWD(other).release()) {}

    ~unique_operation() { amongoc_operation_destroy(((unique_operation&&)*this).release()); }

    unique_operation& operator=(unique_operation&& other) noexcept {
        amongoc_operation_destroy(_oper);
        _oper = AM_FWD(other).release();
        return *this;
    }

    /**
     * @brief Relinquish ownership of the underlying operation and return it
     */
    [[nodiscard]] amongoc_operation release() && noexcept {
        auto e = _oper;
        _oper  = {};
        return e;
    }

    // Launch the asynchronous operation
    void start() noexcept { amongoc_start(&_oper); }

    /**
     * @brief Create an operation from an initiation function
     *
     * @param fn A function that, when called, will initiate the operation
     */
    template <typename F>
    static unique_operation
    from_starter(cxx_allocator<> a, unique_handler&& hnd, F fn) noexcept(box_inlinable_type<F>) {
        amongoc_operation ret;
        ret.handler  = AM_FWD(hnd).release();
        ret.userdata = unique_box::from(a, (F&&)(fn)).release();
        static_assert(requires { fn(ret); });
        static_assert(
            not requires(const amongoc_operation& cop) { fn(cop); },
            "The starter must accept the operation by non-const lvalue reference");
        ret.start_callback = [](amongoc_operation* self) noexcept {
            self->userdata.view.as<std::remove_cvref_t<F>>()(*self);
        };
        return AM_FWD(ret).as_unique();
    }

private:
    amongoc_operation _oper{};
};

}  // namespace amongoc

amongoc::unique_operation amongoc_operation::as_unique() && noexcept {
    return amongoc::unique_operation((amongoc_operation&&)(*this));
}
#endif
