#pragma once

#include <mlib/config.h>

#include <type_traits>

namespace mlib::detail {

#if mlib_have_cxx20()
template <typename T, typename... Conds>
    requires(Conds::value and ...)
using requires_t = T;
#else
template <typename T, typename... Conds>
using requires_t = typename std::enable_if<std::conjunction<Conds...>::value, T>::type;
#endif

template <int N>
struct rank : rank<N - 1> {};

template <>
struct rank<0> {};

}  // namespace mlib::detail
