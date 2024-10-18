#pragma once

#include <type_traits>

namespace amongoc {

/**
 * @brief Trait variable that tags types which are trivially-relocatable
 */
template <typename T>
constexpr bool enable_trivially_relocatable_v
    = (std::is_trivially_destructible_v<T> and std::is_trivially_move_constructible_v<T>)
    or requires {
           typename T::enable_trivially_relocatable;
           requires std::is_same_v<typename T::enable_trivially_relocatable, T>;
       };

// Use within the public section of a class body to declare whether a type is trivially relocatable
#define AMONGOC_TRIVIALLY_RELOCATABLE_THIS(Condition, ThisType)                                    \
    using enable_trivially_relocatable [[maybe_unused]]                                            \
    = std::conditional_t<(Condition), ThisType, void>

}  // namespace amongoc
