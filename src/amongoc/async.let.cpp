#include <amongoc/async.h>
#include <amongoc/box.compress.hpp>
#include <amongoc/nano/let.hpp>
#include <amongoc/nano/simple.hpp>

using namespace amongoc;

namespace {

/**
 * EXPLAIN: The amongoc_let function uses compress() twice, which generates a huge
 * number of specializations of amongoc::let(). To reduce compile times, we define
 * the let() continuations in separate structs that throw away compile-time
 * information that isn't relevant, allowing the compiler's template memoization
 * to perform more cache hits. This significantly reduces the compile time from
 * several minutes.
 *
 * Additionally, the body of each continuation object should be as small as possible,
 * and only deal with the details that make it distinct from each other specialization.
 */
template <bool ForwardErrors>
struct let_forward_errors;

// Applies the transformer. This is out-of-line of the continuation so that the templated
// continuation functions are as small as possible.
unique_emitter
apply_transform(amongoc_let_transformer tr, unique_box&& userdata, emitter_result&& res) {
    return tr(mlib_fwd(userdata).release(), res.status, mlib_fwd(res).value.release()).as_unique();
}

// If we need to forward errors, we'll need to grab the allocator so we can call amongoc_just()
template <>
struct let_forward_errors<true> {
    template <typename CompressedUserdata, typename GetAllocator>
    struct continuation {
        amongoc_let_transformer                  transform;
        [[no_unique_address]] CompressedUserdata userdata;
        [[no_unique_address]] GetAllocator       get_alloc;

        unique_emitter operator()(emitter_result&& res) {
            if (res.status.is_error()) {
                return amongoc_just(res.status, mlib_fwd(res).value.release(), get_alloc())
                    .as_unique();
            }
            return apply_transform(transform, mlib_fwd(userdata).recover(), mlib_fwd(res));
        }
    };
};

// If we aren't forwarding errors, we don't rely on the allocator, and we can share
// the continuation type regardless of the allocator source.
template <>
struct let_forward_errors<false> {
    template <typename CompressedUserdata>
    struct continuation_ {
        amongoc_let_transformer                  transform;
        [[no_unique_address]] CompressedUserdata userdata;

        // Mimic the constructor that takes the get_alloc object. We don't need it, so just throw it
        // away
        continuation_(amongoc_let_transformer tr, CompressedUserdata&& ud, auto /* get_alloc */)
            : transform(tr)
            , userdata(mlib_fwd(ud)) {}

        unique_emitter operator()(emitter_result&& res) {
            return apply_transform(transform, mlib_fwd(userdata).recover(), mlib_fwd(res));
        }
    };

    // Alias continuation<> to throw away the GetAllocator type
    template <typename CompressedUserdata, typename GetAllocator>
    using continuation = continuation_<CompressedUserdata>;
};

}  // namespace

template <bool ForwardErrors,
          typename CompressedEmitter,
          typename GetAllocator,
          typename CompressedUserdata>
static unique_emitter _let(CompressedEmitter&&     in,
                           CompressedUserdata&&    ud,
                           GetAllocator            get_alloc,
                           amongoc_let_transformer tr) {
    using C = let_forward_errors<ForwardErrors>::template continuation<CompressedUserdata,
                                                                       GetAllocator>;
    nanosender_of<emitter_result> auto l
        = amongoc::let(mlib_fwd(in), C{tr, mlib_fwd(ud), get_alloc});
    return as_emitter(mlib::allocator<>{get_alloc()}, mlib_fwd(l));
}

emitter(amongoc_let)(emitter                 in_,
                     amongoc_async_flags     flags,
                     mlib_allocator          alloc,
                     box                     userdata_,
                     amongoc_let_transformer tr) noexcept {
    auto ud    = mlib_fwd(userdata_).as_unique();
    auto in    = mlib_fwd(in_).as_unique();
    auto let_1 = [&](auto get_alloc) -> unique_emitter {
        return mlib_fwd(in).compress<0 /* , 8, 16 */>([&](auto&& compressed_in) -> unique_emitter {
            return mlib_fwd(ud).compress<0 /* , 8, 16 */>(
                [&](auto&& compressed_ud) -> unique_emitter {
                    if (flags & amongoc_async_forward_errors) {
                        return _let<true>(mlib_fwd(compressed_in),
                                          mlib_fwd(compressed_ud),
                                          get_alloc,
                                          tr);
                    } else {
                        return _let<false>(mlib_fwd(compressed_in),
                                           mlib_fwd(compressed_ud),
                                           get_alloc,
                                           tr);
                    }
                });
        });
    };

    if (alloc.impl == mlib_default_allocator.impl) {
        // Optimize: The emitter doesn't need to store the allocator, it can just
        // pull the default allocator later when it is needed.
        return let_1([] { return ::mlib_default_allocator; }).release();
    } else {
        return let_1(constant(alloc)).release();
    }
}
