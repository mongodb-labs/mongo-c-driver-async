#include <amongoc/emitter.h>

using namespace amongoc;

amongoc_operation amongoc_emitter_tie_output(amongoc_emitter em,
                                             amongoc_status* status,
                                             amongoc_box*    value) AMONGOC_NOEXCEPT {
    // Connect to a handler that stores the result values in the pointed-to locations
    return AM_FWD(em)
        .as_unique()
        .connect([=](amongoc_status st, unique_box&& val) {
            if (status) {
                *status = st;
            }
            if (value) {
                *value = val.release();
            }
        })
        .release();
}

amongoc_operation amongoc_detached_operation(amongoc_emitter em) AMONGOC_NOEXCEPT {
    // Connect to a handler that simply discards the result values
    return AM_FWD(em).as_unique().connect([=](auto&&...) {}).release();
}
