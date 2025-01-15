#pragma once

#include <mlib/type_traits.hpp>

#include <memory>
#include <type_traits>

namespace mlib::detail {

/**
 * @brief Backport implementation of C++20 std::construct_at
 *
 */
inline constexpr struct construct_at_fn {
    template <typename T, typename... Args>
    mlib_always_inline constexpr auto operator()(T* ptr, Args&&... args) const
        -> requires_t<T*, std::is_constructible<T, Args...>> {
#if __cpp_constexpr_dynamic_alloc
        return std::construct_at(ptr, mlib_fwd(args)...);
#else
        return ::new (static_cast<void*>(ptr)) T(mlib_fwd(args)...);
#endif
    }
} construct_at;

/**
 * @brief Backport implementation of C++20 std::uninitialized_construct_using_allocator
 */
struct uninitialized_construct_using_allocator_fn {
    // Fallback: Regular constructor with no allocator
    template <typename T, typename Alloc, typename... Args>
    static constexpr T*
    _try_trailing(std::false_type, T* ptr, const Alloc& alloc [[maybe_unused]], Args&&... args) {
        return construct_at(ptr, mlib_fwd(args)...);
    }

    // Trailing uses-allocator construct:
    template <typename T, typename Alloc, typename... Args>
    static constexpr T* _try_trailing(std::true_type, T* ptr, const Alloc& alloc, Args&&... args) {
        return construct_at(ptr, mlib_fwd(args)..., alloc);
    }

    // No leading allocator construct: Try with trailing allocator:
    template <typename T, typename Alloc, typename... Args>
    static constexpr T*
    _try_leading_with_tag(std::false_type, T* ptr, const Alloc& alloc, Args&&... args) {
        return _try_trailing(std::conjunction<std::uses_allocator<T, Alloc>,
                                              std::is_constructible<T, Args..., Alloc>>{},
                             ptr,
                             alloc,
                             mlib_fwd(args)...);
    }

    // Leading tagged using-allocator construct:
    template <typename T, typename Alloc, typename... Args>
    static constexpr T*
    _try_leading_with_tag(std::true_type, T* ptr, const Alloc& alloc, Args&&... args) {
        return construct_at(ptr, std::allocator_arg, alloc, mlib_fwd(args)...);
    }

    /**
     * @brief In-place construct an object at pointer-to-storage `ptr` using the
     * given allocator and arguments.
     *
     * @param ptr Pointer to uninitialized storage for a new object
     * @param allocator An allocator to be imbued into the object
     * @param args Other constructor arguments to pass along
     * @return T* a pointer to the constructed object
     */
    template <typename T, typename Alloc, typename... Args>
    constexpr T* operator()(T* ptr, const Alloc& allocator, Args&&... args) const {
        return _try_leading_with_tag(
            std::conjunction<std::uses_allocator<T, Alloc>,
                             std::is_constructible<T, std::allocator_arg_t, Alloc, Args...>>{},
            ptr,
            allocator,
            mlib_fwd(args)...);
    }
};

inline constexpr uninitialized_construct_using_allocator_fn uninitialized_construct_using_allocator;

}  // namespace mlib::detail
