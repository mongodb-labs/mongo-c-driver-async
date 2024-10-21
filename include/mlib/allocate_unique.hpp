#pragma once

#include <mlib/alloc.h>

#include <memory>

namespace mlib {

// A deleter type that uses an `mlib::allocator<>` to destroy an object
struct alloc_deleter {
    mlib::allocator<> alloc;

    template <typename T>
    void operator()(T* ptr) const noexcept {
        auto a = alloc.rebind<T>();
        a.delete_(ptr);
    }
};

// A `unique_ptr` that uses an `mlib::allocator` to deallocate objects
template <typename T>
using unique_ptr = std::unique_ptr<T, alloc_deleter>;

/**
 * @brief Create a `unique_ptr` that uses the given `mlib::allocator<>` for memory management
 *
 * @tparam T The type to be constructed
 * @param a The allocator to be used
 * @param args The constructor arguments
 *
 * @note This does not yet handle arrays, only single objects. Use a `std::array` if you need it.
 */
template <typename T>
constexpr unique_ptr<T> allocate_unique(allocator<> a, auto&&... args) {
    T* p = a.rebind<T>().new_(mlib_fwd(args)...);
    return unique_ptr<T>(p, alloc_deleter{a});
}

}  // namespace mlib
