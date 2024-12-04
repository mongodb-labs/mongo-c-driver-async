/**
 * @file delete.h
 * @brief C type deletion registration
 * @date 2024-10-29
 *
 * This file is used to define how to "delete" C types that acts as owners or
 * resources.
 *
 * Refer to `dev/deletion` for details
 */
#pragma once

#include <mlib/config.h>

#if mlib_is_cxx()
#include <type_traits>
#endif

/**
 * @brief Use inline within a struct to declare a deletion spec that invokes
 * a deletion on struct members. In C, expands to an empty declaration.
 */
#define mlib_declare_member_deleter(...)                                                           \
    MLIB_IF_ELSE(mlib_have_cxx20())                                                                \
    (using deleter = ::mlib::delete_members<__VA_ARGS__>)(mlib_static_assert(true))

/**
 * @brief Place at global scope to declare the associated between a simple type
 * and a deletion function for that type. In C, expands to an empty declaration
 */
#define mlib_assoc_deleter(T, DelFn)                                                               \
    MLIB_IF_CXX(extern "C++" template <> struct mlib::unique_deleter<T> : ::mlib::just_invokes<DelFn> {};) mlib_static_assert(true, "")

/**
 * @brief Declares a C API function `FuncName` that invokes the default deletion
 * method for `Type`.
 *
 * This declaration must be included within at least one C++ source file to generate
 * the implementation of the function.
 *
 * The `gnu::used` attribute forces the compiler to emit the definition in an including
 * translation unit, even if it isn't called in that TU.
 */
#define mlib_declare_c_deletion_function(FuncName, Type)                                           \
    MLIB_IF_ELSE(mlib_have_cxx20())                                                                \
    (extern "C" MLIB_IF_GNU_LIKE([[gnu::used]]) inline void FuncName(Type inst) noexcept {         \
        ::mlib::delete_unique(inst);                                                               \
    } static_assert(true, "")) /*                */                                                \
        (void FuncName(Type inst))

#if mlib_is_cxx()

namespace mlib {

/**
 * @brief Specialize to determine how to "delete" an instance of T
 *
 * This is used as an aide for automatic deletion of C types
 */
template <typename T>
struct unique_deleter {};

/**
 * @brief An invocable that calls the default deletion method defined by `unique_deleter`
 */
inline constexpr struct delete_unique_fn {
    template <typename T>
    constexpr auto operator()(T& inst) const noexcept -> decltype(mlib::unique_deleter<T>{}(inst)) {
        mlib::unique_deleter<T>{}(inst);
    }

    template <typename T>
    constexpr auto
    operator()(T&& inst) const noexcept -> decltype(mlib::unique_deleter<T>{}(inst)) {
        mlib::unique_deleter<T>{}(inst);
    }
} delete_unique;

#if mlib_have_cxx20()
template <typename T>
concept unique_deletable = requires(T& inst) { mlib::delete_unique(inst); };

/**
 * @brief A deleter that detects an ADL-visible `mlib_delete_this()` function
 */
template <typename T>
    requires requires(T& instance) { mlib_delete_this(instance); }
struct unique_deleter<T> {
    void operator()(T& inst) { mlib_delete_this(inst); }
};

/**
 * @brief A deleter that detects a nested type `deleter` and defers to that
 */
template <typename T>
    requires requires { typename T::deleter; }
struct unique_deleter<T> : T::deleter {};
#endif  // ≥C++20

/**
 * @brief A deleter that invokes `delete_unique` on each of the given class members
 */
template <auto... MemPointers>
struct delete_members {
    template <typename T>
    constexpr void operator()(T& inst) const noexcept {
        (delete_unique(inst.*MemPointers), ...);
    }
};

/**
 * @brief An invocable object that holds a compile-time constant invocable (usually a function
 * pointer)
 *
 * @tparam Fn The underlying invocable object.
 */
template <auto Fn>
struct just_invokes {
    template <typename... Args>
    constexpr auto operator()(Args&&... args) const MLIB_RETURNS(Fn(mlib_fwd(args)...));
};

}  // namespace mlib

#endif  // C++
