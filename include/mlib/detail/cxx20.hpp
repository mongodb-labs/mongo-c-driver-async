#pragma once

#include <mlib/type_traits.hpp>

#include <memory>

namespace mlib::detail {

/**
 * @brief Backport implementation of C++20 std::construct_at
 *
 */
inline constexpr struct construct_at_fn {
    template <typename T, typename... Args>
    mlib_always_inline constexpr auto
    operator()(T* ptr, Args&&... args) const -> requires_t<T*, std::is_constructible<T, Args...>> {
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
    // Fallback construct if the object does not support allocator construction
    template <typename T, typename Alloc, typename... Args>
    static constexpr requires_t<T*, std::is_constructible<T, Args...>>
    impl(rank<0>, T* ptr, const Alloc&, Args&&... args) {
        return construct_at(ptr, mlib_fwd(args)...);
    }

    // Construct using a trailing allocator argument
    template <typename T, typename Alloc, typename... Args, int = 0>
    static constexpr requires_t<T*,
                                std::uses_allocator<T, Alloc>,
                                std::is_constructible<T, Args..., Alloc>>
    impl(rank<1>, T* ptr, const Alloc& allocator, Args&&... args) {
        return construct_at(ptr, mlib_fwd(args)..., allocator);
    }

    // Construct using a leading allocator with a tag
    template <typename T, typename Alloc, typename... Args, int = 0, int = 0>
    static constexpr requires_t<T*,
                                std::uses_allocator<T, Alloc>,
                                std::is_constructible<T, std::allocator_arg_t, Alloc, Args...>>
    impl(rank<1>, T* ptr, const Alloc& allocator, Args&&... args) {
        return construct_at(ptr, std::allocator_arg, allocator, mlib_fwd(args)...);
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
    constexpr auto operator()(T* ptr, const Alloc& allocator, Args&&... args) const
        -> decltype(impl(rank<2>{}, ptr, allocator, mlib_fwd(args)...)) {
        return impl(rank<2>{}, ptr, allocator, mlib_fwd(args)...);
    }
};

inline constexpr uninitialized_construct_using_allocator_fn uninitialized_construct_using_allocator;

}  // namespace mlib::detail
