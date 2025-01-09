#include <amongoc/async.h>
#include <amongoc/box.compress.hpp>
#include <amongoc/handler.h>

using namespace amongoc;

emitter(amongoc_just)(status st, box value, mlib_allocator alloc_) noexcept {
    // Make unique outside of just_1 to reduce code size of just_1
    auto&&            uniq = mlib_fwd(value).as_unique();
    mlib::allocator<> alloc{alloc_};

    // Accepts the status getter and compressed box and produces the final emitter
    auto just_2 = [&]<typename GetStatus, typename Compressed>(GetStatus    get_st,
                                                               Compressed&& c) -> unique_emitter {
        struct starter {
            [[no_unique_address]] Compressed value;
            [[no_unique_address]] GetStatus  get_status;
            void                             operator()(amongoc_handler& hnd) {
                ::amongoc_handler_complete(&hnd, get_status(), mlib_fwd(value).recover().release());
            }
        };

        struct connector {
            [[no_unique_address]] Compressed value;
            [[no_unique_address]] GetStatus  get_status;

            unique_operation operator()(unique_handler&& hnd) {
                return unique_operation::from_starter(mlib_fwd(hnd),
                                                      starter{mlib_fwd(value), get_status});
            }
        };

        return unique_emitter::from_connector(alloc, connector{mlib_fwd(c), get_st});
    };

    // Accepts the status getter and compresses the value box
    auto just_1 = [&](auto get_status) -> unique_emitter {
        return mlib_fwd(uniq).compress<0, 1, 2, 4, 8, 12, 16, 24>(
            [&](auto&& compressed) -> unique_emitter {
                return just_2(get_status, mlib_fwd(compressed));
            });
    };

    try {
        if (st == amongoc_okay) {
            return just_1([] { return amongoc_okay; }).release();
        } else {
            return just_1([st] { return st; }).release();
        }
    } catch (std::bad_alloc const&) {
        return amongoc_alloc_failure();
    }
}
