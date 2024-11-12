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
