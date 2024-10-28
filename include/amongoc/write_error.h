#pragma once

#include <amongoc/status.h>

#include <mlib/str.h>

#include <stdint.h>

/**
 * @brief Information about an error while writing data to a collection
 */
typedef struct amongoc_write_error {
    int32_t                  index;
    enum amongoc_server_errc code;
    mlib_str                 errmsg;
} amongoc_write_error;

inline void amongoc_write_error_delete(amongoc_write_error err) mlib_noexcept {
    mlib_str_delete(err.errmsg);
}

#define T amongoc_write_error
#define VecDestroyElement amongoc_write_error_delete
#include <mlib/vec.t.h>
