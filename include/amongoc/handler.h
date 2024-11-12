#pragma once

#include "./box.h"
#include "./status.h"

#include <mlib/alloc.h>
#include <mlib/config.h>
#include <mlib/delete.h>

MLIB_IF_CXX(namespace amongoc { struct unique_handler; })

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

    mlib_declare_member_deleter(&amongoc_handler::userdata);

#if mlib_is_cxx()
    /// Transfer ownership of the handler into a unique_handler
    inline amongoc::unique_handler as_unique() && noexcept;
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
mlib_declare_c_deletion_function(amongoc_handler_delete, amongoc_handler);

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
