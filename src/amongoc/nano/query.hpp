#pragma once

namespace amongoc {

/**
 * @brief Match a type that is valid to be used as a query tag on another object
 *
 * @tparam Q The candate query tag type
 * @tparam T The query target
 *
 * Requires that the target type have a `query() const&` method that accepts the query tag
 * as an argument
 */
template <typename Q, typename T>
concept valid_query_for = requires(const T& obj, const Q q) { q(obj); };

template <typename Q, typename T>
    requires valid_query_for<Q, T>
using query_t = decltype(Q{}(*(const T*)(nullptr)));

}  // namespace amongoc
