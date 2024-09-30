#pragma once

#include <amongoc/async.h>
#include <amongoc/box.h>
#include <amongoc/default_loop.h>
#include <amongoc/emitter.h>
#include <amongoc/emitter_result.h>
#include <amongoc/operation.h>

#include <mlib/alloc.h>

namespace amongoc::testing {

/**
 * @brief A text fixture class that constructs an initializes a default event loop,
 * and has a utility to execute async operations
 */
struct loop_fixture {
    // The event loop for this test
    amongoc::default_event_loop loop;

    /**
     * @brief Execute the operation defined be the emitter and return its result
     *
     * @param em The emitter to execute. NOTE: It should be associated with our event loop, or
     * it won't work!
     */
    emitter_result run_to_completion(amongoc_emitter em) noexcept {
        emitter_result ret;
        amongoc_box    box;
        auto           op = ::amongoc_tie(em, &ret.status, &box, ::mlib_default_allocator);
        ::amongoc_start(&op);
        loop.run();
        ::amongoc_operation_destroy(op);
        ret.value = mlib_fwd(box).as_unique();
        return ret;
    }
};

}  // namespace amongoc::testing
