#pragma once

#include "./invoke.hpp"

#include <memory>  // std::addressof
#include <type_traits>

namespace mlib {

/**
 * @brief Like reference_wrapper, but takes a reference type, is shallow-comparable,
 * and not implicitly convertible to the target.
 *
 * @tparam Ref The wrapped reference type
 */
template <typename Ref>
class reference_object {
public:
    // The referred-to type
    using type = std::remove_reference_t<Ref>;

    // Construct by binding a reference to an object
    constexpr reference_object(Ref arg) noexcept
        : _pointer(std::addressof(arg)) {}

    // Explicit-convert to the target reference type
    constexpr explicit operator Ref() const noexcept { return *_pointer; }
    // Obtain a C++ reference to the referred-to object
    constexpr Ref get() const noexcept { return static_cast<Ref>(*this); }

    /**
     * @brief Invoke the referred-to object.
     *
     * This function is unavailable if the invocation would be ill-formed.
     *
     * @param args The arguments to apply to the object
     */
    template <typename... Args>
    constexpr mlib::invoke_result_t<Ref, Args...> operator()(Args&&... args) const
        noexcept(std::is_nothrow_invocable_v<Ref, Args...>) {
        return (*_pointer)(static_cast<Args&&>(args)...);
    }

    bool operator==(const reference_object&) const  = default;
    auto operator<=>(const reference_object&) const = default;

private:
    type* _pointer;
};

/**
 * @brief A very basic unit type
 */
struct unit {
    // Default-constructor is a no-op
    constexpr unit() noexcept = default;
    // Construct from a literal zero, also a no-op
    constexpr unit(decltype(nullptr)) noexcept {}
    // Explicitly convert from anything (just discards the argument)
    template <typename X>
    constexpr explicit unit(X&&) noexcept {}

    // Default equality for a unit is always `true`
    constexpr bool operator==(unit const&) const noexcept = default;
    // Default ordering for a unit is always `equal`
    constexpr std::strong_ordering operator<=>(unit const&) const noexcept = default;
};

namespace detail {

template <bool IsReference, bool IsVoid>
struct object_t_impl {
    template <typename T>
    using f = T;
};

template <>
struct object_t_impl<true /* is reference */, false> {
    template <typename T>
    using f = mlib::reference_object<T>;
};

template <>
struct object_t_impl<false, true /* is void */> {
    template <typename>
    using f = mlib::unit;
};

}  // namespace detail

/**
 * @brief Coerce the given type to an regular language object type with idealized
 * semantics of the wrapped type
 *
 * @tparam T An object type, reference type, or void type
 *
 * If `T` is an object, yields `T`. If a reference, yields a `reference_object<T>`.
 * If `void`, yields `mlib::unit`.
 */
template <typename T>
using object_t = detail::object_t_impl<std::is_reference_v<T>, std::is_void_v<T>>::template f<T>;

/**
 * @brief Wrap an argument using `object_t`
 *
 * Coerces the argument to a language object type with the semantics of the
 * given type.
 *
 * Most useful for capturing references in cases where the language would otherwise
 * perform an unwanted decay-copy (e.g. in lambda expression captures)
 */
constexpr auto as_object
    = []<typename T>(T&& t) -> object_t<T> { return object_t<T>(static_cast<T&&>(t)); };

struct unwrap_object_fn {
    template <typename T>
    constexpr T&& operator()(T&& obj) const noexcept {
        return static_cast<T&&>(obj);
    }

    template <typename Ref>
    constexpr Ref operator()(reference_object<Ref> ref) const noexcept {
        return static_cast<Ref>(ref);
    }

    constexpr void operator()(unit) const noexcept {}
};

/**
 * @brief Invert the type transformation performed by `object_t`/`as_object`
 */
constexpr unwrap_object_fn unwrap_object;

}  // namespace mlib
