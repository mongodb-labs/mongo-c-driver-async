#pragma once

#include <type_traits>

namespace amongoc {

namespace detail {

template <typename T, typename = T>
struct has_enable_trivial_typedef : std::false_type {};

// This spec is selected if `enable_trivially_relocatable` is defined to be the same type as T
template <typename T>
struct has_enable_trivial_typedef<T, typename T::enable_trivially_relocatable> : std::true_type {};

}  // namespace detail

/**
 * @brief Trait variable that tags types which are trivially-relocatable
 */
template <typename T>
constexpr bool enable_trivially_relocatable_v
    = (std::is_trivially_destructible_v<T> and std::is_trivially_move_constructible_v<T>)
    or detail::has_enable_trivial_typedef<T>::value;

// Use within the public section of a class body to declare whether a type is trivially relocatable
#define AMONGOC_TRIVIALLY_RELOCATABLE_THIS(Condition, ThisType)                                    \
    using enable_trivially_relocatable [[maybe_unused]]                                            \
    = std::conditional_t<(Condition), ThisType, void>

}  // namespace amongoc
