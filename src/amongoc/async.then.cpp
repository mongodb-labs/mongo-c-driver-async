#include <amongoc/async.h>
#include <amongoc/box.compress.hpp>
#include <amongoc/nano/simple.hpp>
#include <amongoc/nano/then.hpp>

using namespace amongoc;

namespace {

template <bool ForwardErrors, typename CompressedUserdata>
struct then_continuation {
    amongoc_then_transformer                 tr;
    [[no_unique_address]] CompressedUserdata ud;

    emitter_result operator()(emitter_result&& res) && {
        if constexpr (ForwardErrors) {
            if (res.status.is_error()) {
                return mlib_fwd(res);
            }
        }
        res.value = tr(mlib_fwd(ud).recover().release(), &res.status, mlib_fwd(res).value.release())
                        .as_unique();
        return mlib_fwd(res);
    }
};

}  // namespace

template <bool ForwardErrors, typename CompressedEmitter, typename CompressedUserdata>
static unique_emitter _then(CompressedEmitter&&      em,
                            CompressedUserdata&&     ud,
                            mlib::allocator<> const& alloc,
                            amongoc_then_transformer tr) {
    return as_emitter(alloc,
                      amongoc::then(mlib_fwd(em),
                                    then_continuation<ForwardErrors, CompressedUserdata>{tr,
                                                                                         mlib_fwd(
                                                                                             ud)}));
}

emitter amongoc_then(emitter                  in,
                     amongoc_async_flags      flags,
                     mlib_allocator           alloc_,
                     box                      userdata_,
                     amongoc_then_transformer tr) noexcept {
    auto alloc = mlib::allocator<>{alloc_};
    return mlib_fwd(in)
        .as_unique()
        .compress<0, 8, 16>([&](auto&& in) -> unique_emitter {
            return mlib_fwd(userdata_).as_unique().compress<0, 8, 16>(
                [&](auto&& ud_compressed) -> unique_emitter {
                    if (flags & amongoc_async_forward_errors) {
                        return ::_then<true>(mlib_fwd(in), mlib_fwd(ud_compressed), alloc, tr);
                    } else {
                        return ::_then<false>(mlib_fwd(in), mlib_fwd(ud_compressed), alloc, tr);
                    }
                });
        })
        .release();
}

emitter amongoc_then_just(amongoc_emitter          in,
                          enum amongoc_async_flags flags,
                          amongoc_status           st,
                          amongoc_box              value,
                          mlib_allocator           alloc) noexcept {
    if (st == amongoc_okay) {
        // We are replacing the value, but the status will be replaced with `amongoc_okay`.
        // In this case, we can pass the value as the userdata to a then() transform, and
        // rely on then() to perform value compression for us.
        return amongoc_then(in,
                            flags,
                            alloc,
                            value,
                            [](amongoc_box value, status* st, amongoc_box result) noexcept {
                                // Immediately discard the old value
                                amongoc_box_destroy(result);
                                // We are setting the status to `okay`
                                *st = amongoc_okay;
                                // Return the replacement value, which was passed to us
                                // via the userdata parameter
                                return value;
                            });
    } else {
        // We are replacing both the status and value. We need to pass the status
        // and the value to the then() algorithm.
        mlib::allocator<> cx_alloc{alloc};
        return mlib_fwd(value).as_unique().compress<0, 8, 16>([&]<typename Compressed>(
                                                                  Compressed&& compressed)
                                                                  -> emitter {
            struct value_with_status {
                status                           st;
                [[no_unique_address]] Compressed value;
            };
            return amongoc_then(  //
                in,
                flags,
                alloc,
                unique_box::from(cx_alloc, value_with_status{st, mlib_fwd(compressed)}).release(),
                [](amongoc_box     pair_,
                   amongoc_status* st,
                   amongoc_box     result) noexcept -> amongoc_box {
                    amongoc_box_destroy(result);
                    value_with_status& pair = pair_.view.as<value_with_status>();
                    *st                     = pair.st;
                    return std::move(pair).value.recover().release();
                });
        });
    }
}
