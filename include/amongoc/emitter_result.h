#pragma once

#include "./box.h"
#include "./status.h"

#if mlib_is_cxx()

namespace amongoc {

/**
 * @brief Special type to be used to capture the result of an amongoc_emitter as a pair of status
 * and boxed value
 */
class emitter_result {
public:
    // Default-construct to an empty box and success
    emitter_result() noexcept
        : value(amongoc_nil) {}

    // Construct with an empty box and the given status
    explicit emitter_result(amongoc_status st) noexcept
        : status(st)
        , value(amongoc_nil) {}

    // Construct with the given status and an existing box
    explicit emitter_result(amongoc_status st, unique_box&& b) noexcept
        : status(st)
        , value(mlib_fwd(b)) {}

    amongoc_status status{0};
    unique_box     value;
};

}  // namespace amongoc

#endif
