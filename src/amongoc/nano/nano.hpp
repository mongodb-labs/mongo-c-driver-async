#pragma once

#include "./concepts.hpp"

#include <type_traits>

namespace amongoc {

struct connect_fn {
    /**
     * @brief Connect a sender to a receiver, forming an nanooperation
     *
     * @param s The sender
     * @param r The receiver
     * @return A new nanooperation representing the connected operation
     */
    template <nanosender S, nanoreceiver_for<S> R>
    [[nodiscard]] constexpr nanooperation decltype(auto) operator()(S&& s, R&& r) const noexcept {
        return nanosender_traits<std::remove_cvref_t<S>>::connect(mlib_fwd(s), mlib_fwd(r));
    }
};

/// Connect a sender+receiver pair
inline constexpr amongoc::connect_fn connect;

template <nanosender S, nanoreceiver_for<S> R>
using connect_t = decltype(connect(std::declval<S>(), std::declval<R>()));

}  // namespace amongoc
