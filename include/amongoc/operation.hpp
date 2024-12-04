#pragma once

#include <amongoc/box.hpp>
#include <amongoc/handler.hpp>
#include <amongoc/operation.h>

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
