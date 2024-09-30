#pragma once

#include <amongoc/box.h>
#include <amongoc/emitter.h>
#include <amongoc/handler.h>

#include <array>
#include <utility>

namespace amongoc {

/**
 * @brief A "compressed" box
 *
 * @tparam Nbytes The number of bytes that are available for the box
 * @tparam WithDtor Whether the box has a destructor
 *
 * This primary definition handles trivial boxes with no destructor
 */
template <std::size_t Nbytes, bool WithDtor>
struct compressed_box {
    std::array<char, Nbytes> buffer;

    [[nodiscard]] unique_box recover() && noexcept {
        box ret{};
        memcpy(ret._storage.u.trivial_inline.bytes + 0, buffer.data(), Nbytes);
        ret._storage.inline_size = Nbytes;
        return mlib_fwd(ret).as_unique();
    }
};

// Specialize for an empty box with no destructor (e.g. nil)
template <>
struct compressed_box<0, false> {
    [[nodiscard]] unique_box recover() && noexcept { return amongoc_nil.as_unique(); }
};

// Specialize for non-empty boxes that have destructors
template <std::size_t Nbytes>
struct compressed_box<Nbytes, true> {
    compressed_box() = default;

    amongoc_box_destructor   dtor;
    std::array<char, Nbytes> buffer;

    compressed_box(compressed_box&& other)
        : dtor(other.dtor)
        , buffer(other.buffer) {
        other.dtor = nullptr;
    }

    [[nodiscard]] unique_box recover() && noexcept {
        box ret{};
        memcpy(ret._storage.u.nontrivial_inline.bytes + 0, buffer.data(), Nbytes);
        memcpy(ret._storage.u.nontrivial_inline.dtor_bytes + 0, &dtor, sizeof dtor);
        ret._storage.inline_size = Nbytes;
        ret._storage.has_dtor    = true;
        dtor                     = nullptr;
        return mlib_fwd(ret).as_unique();
    }

    ~compressed_box() {
        if (dtor) {
            // Restore the unique_box and immediately destroy it:
            (void)std::move(*this).recover();
        }
    }
};

// Specialize for the weird empty box with a destructor
template <>
struct compressed_box<0, true> {
    compressed_box() = default;

    amongoc_box_destructor dtor;

    compressed_box(compressed_box&& other)
        : dtor(other.dtor) {
        other.dtor = nullptr;
    }

    [[nodiscard]] unique_box recover() && noexcept {
        box ret = amongoc_nil;
        memcpy(ret._storage.u.nontrivial_inline.dtor_bytes + 0, &dtor, sizeof dtor);
        dtor = nullptr;
        return mlib_fwd(ret).as_unique();
    }

    ~compressed_box() {
        if (dtor) {
            // Restore the unique_box and immediately destroy it:
            (void)std::move(*this).recover();
        }
    }
};

// Case where we cannot compress a box, so just store that box
struct uncompressed_box {
    unique_box box;

    [[nodiscard]] unique_box recover() && noexcept { return mlib_fwd(box); }
};

/**
 * @brief Stores a single pointer to a dynamically allocated box data
 */
struct dynamic_box {
    // The allocated box data
    _amongoc_dynamic_box* ptr;

    explicit dynamic_box(_amongoc_dynamic_box* p)
        : ptr(p) {}
    dynamic_box(dynamic_box&& other) noexcept
        : dynamic_box(std::exchange(other.ptr, nullptr)) {}

    dynamic_box& operator=(dynamic_box&& other) noexcept {
        if (ptr) {
            (void)std::move(*this).recover();
        }
        ptr = std::exchange(other.ptr, nullptr);
        return *this;
    }

    unique_box recover() && noexcept {
        box r;
        r._storage.is_dynamic = true;
        r._storage.has_dtor   = ptr->destroy != nullptr;
        r._storage.u.dynamic  = std::exchange(ptr, nullptr);
        return unique_box(std::move(r));
    }
};

template <std::size_t N, bool AddDtor>
constexpr compressed_box<N, AddDtor> _make_compressed(unique_box&& box, ::amongoc_box& c_box) {
    compressed_box<N, AddDtor> ret;
    if constexpr (N) {
        memcpy(ret.buffer.data(), box.data(), c_box._storage.inline_size);
    }
    if constexpr (AddDtor) {
        // Be sure to call release() so that the unique_box does not destroy its value
        // after compression is finished.
        memcpy(&ret.dtor,
               mlib_fwd(box).release()._storage.u.nontrivial_inline.dtor_bytes + 0,
               sizeof ret.dtor);
    } else {
        // The box is trivial and has no destructor, so we don't need to care what happens to the
        // box.
    }
    return ret;
}

template <typename F>
auto _compress_p1(unique_box&& box, ::amongoc_box&, std::index_sequence<>, F&& fn) {
    return mlib_fwd(fn)(uncompressed_box{mlib_fwd(box)});
}

template <std::size_t Try, std::size_t... Sz, typename F>
auto _compress_p1(unique_box&&    box,
                  ::amongoc::box& c_box,
                  std::index_sequence<Try, Sz...>,
                  F&& fn) {
    if (c_box._storage.inline_size <= Try) {
        if (c_box._storage.has_dtor) {
            return mlib_fwd(fn)(_make_compressed<Try, true>(mlib_fwd(box), c_box));
        } else {
            return mlib_fwd(fn)(_make_compressed<Try, false>(mlib_fwd(box), c_box));
        }
    } else {
        return _compress_p1(mlib_fwd(box), c_box, std::index_sequence<Sz...>{}, mlib_fwd(fn));
    }
}

template <std::size_t... Sz, typename F>
decltype(auto) unique_box::compress(F&& vis) && {
    if (_box._storage.is_dynamic) {
        // Box is dynamic. Compress it to a single pointer.
        dynamic_box b{std::move(*this).release()._storage.u.dynamic};
        return vis(std::move(b));
    } else {
        return _compress_p1(std::move(*this), _box, std::index_sequence<Sz...>{}, mlib_fwd(vis));
    }
}

/**
 * @brief Acts as a nanosender that holds a compress amongoc_emitter
 */
template <typename CompressedUserdata>
struct compressed_emitter {
    // Vtable pointer for the emitter
    amongoc_emitter_vtable const* vtable;
    // Compressed version of the userdata for the emitter
    [[no_unique_address]] CompressedUserdata userdata;

    using sends_type = emitter_result;

    // Connect the emitter to a handler
    template <typename R>
    unique_operation connect(R&& recv) && {
        // Restore the compressed emitter
        ::amongoc_handler hnd = as_handler(mlib_fwd(recv)).release();
        ::amongoc_emitter em;
        em.userdata = mlib_fwd(userdata).recover().release();
        em.vtable   = vtable;
        return amongoc_emitter_connect(em, hnd).as_unique();
    }
};

template <std::size_t... Sz, typename F>
auto unique_emitter::compress(F&& fn) && {
    auto vt = _emitter.vtable;
    static_assert(((Sz % sizeof(void*) == 0) and ...),
                  "Sizes for emitter compression should be multiples of the pointer size. Smaller "
                  "compression is semantically valid, but produces no runtime benefit, at the cost "
                  "of compilation time and code size.");
    return static_cast<unique_emitter&&>(*this)  //
        .release()
        .userdata.as_unique()
        .compress<Sz...>([&]<typename C>(C&& compressed) {
            return static_cast<F&&>(fn)(compressed_emitter<C>{vt, mlib_fwd(compressed)});
        });
}

}  // namespace amongoc
