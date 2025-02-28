#pragma once

#include <amongoc/status.h>

#include <mlib/delete.h>
#include <mlib/str.h>

#include <stdint.h>

/**
 * @brief Information about an error while writing data to a collection
 */
typedef struct amongoc_write_error {
    int32_t                  index;
    enum amongoc_server_errc code;
    mlib_str                 errmsg;

    mlib_declare_member_deleter(&amongoc_write_error::errmsg);
} amongoc_write_error;

mlib_declare_c_deletion_function(amongoc_write_error_delete, amongoc_write_error);

#define T amongoc_write_error
#define VecDestroyElement amongoc_write_error_delete
#define VecInitElement(Inst, Alloc, ...) Inst->errmsg = mlib_str_new(0, Alloc).str
#include <mlib/vec.t.h>
