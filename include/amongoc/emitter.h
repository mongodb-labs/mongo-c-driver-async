#pragma once

#include "./box.h"
#include "./handler.h"
#include "./operation.h"

#include <mlib/alloc.h>
#include <mlib/config.h>
#include <mlib/delete.h>

MLIB_IF_CXX(namespace amongoc { struct unique_emitter; })

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

    mlib_declare_member_deleter(&amongoc_emitter::userdata);

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
mlib_declare_c_deletion_function(amongoc_emitter_delete, amongoc_emitter);

mlib_extern_c_end();
