#pragma once

#include "./box.h"
#include "./handler.h"

#include <mlib/alloc.h>
#include <mlib/config.h>
#include <mlib/delete.h>

MLIB_IF_CXX(namespace amongoc { struct unique_operation; })

typedef struct amongoc_operation amongoc_operation;
typedef void (*amongoc_start_callback)(amongoc_operation* self) mlib_noexcept;

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

    mlib_declare_member_deleter(&amongoc_operation::handler, &amongoc_operation::userdata);

#if mlib_is_cxx()
    // Transfer the operation into a unique_operation object
    [[nodiscard]] inline amongoc::unique_operation as_unique() && noexcept;
#endif
};

mlib_extern_c_begin();

/**
 * @brief Launch an asynchronous operation
 *
 * @param op Pointer to the operation to be started
 */
static inline void amongoc_start(amongoc_operation* op) mlib_noexcept { op->start_callback(op); }

/**
 * @brief Destroy an asynchronous operation object
 *
 * @param op The operation to be destroyed
 */
mlib_declare_c_deletion_function(amongoc_operation_delete, amongoc_operation);

mlib_extern_c_end();

#if mlib_is_cxx()

/**
 * @brief Provides managed ownership over an `amongoc_operation`
 */
struct amongoc::unique_operation : mlib::unique<::amongoc_operation> {
    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true, unique_operation);
    using unique_operation::unique::unique;

    // Launch the asynchronous operation
    void start() noexcept { amongoc_start(&**this); }

    /**
     * @brief Create an operation from an initiation function
     *
     * @param fn A function that, when called, will initiate the operation
     */
    template <typename F>
    static unique_operation from_starter(unique_handler&& hnd, F&& fn)
        noexcept(box_inlinable_type<F>) {
        amongoc_operation ret;
        ret.handler  = mlib_fwd(hnd).release();
        ret.userdata = unique_box::from(::amongoc_handler_get_allocator(&ret.handler,
                                                                        ::mlib_default_allocator),
                                        mlib_fwd(fn))
                           .release();
        static_assert(
            requires(amongoc_handler& h) { fn(h); },
            "The starter must accept a handler as its sole parameter");
        ret.start_callback = [](amongoc_operation* self) noexcept {
            self->userdata.view.as<std::remove_cvref_t<F>>()(self->handler);
        };
        return mlib_fwd(ret).as_unique();
    }
};

amongoc::unique_operation amongoc_operation::as_unique() && noexcept {
    return amongoc::unique_operation((amongoc_operation&&)(*this));
}

#endif  // C++
