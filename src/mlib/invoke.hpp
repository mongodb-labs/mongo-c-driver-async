#pragma once

#include <mlib/config.h>

#include <functional>
#include <utility>

namespace mlib {

template <typename F, typename... Args>
concept plain_invocable = requires(F&& fn, Args&&... args) { mlib_fwd(fn)(mlib_fwd(args)...); };

struct invoke_fn {
    // Plain invocation overload: This is the common case.
    template <typename F, typename... Args>
        requires plain_invocable<F, Args...>
    constexpr decltype(auto) operator()(F&& fn, Args&&... args) const
        noexcept(noexcept(mlib_fwd(fn)(mlib_fwd(args)...))) {
        return mlib_fwd(fn)(mlib_fwd(args)...);
    }

    // Other invocables: We'll fall back on std::invoke to handle these
    template <typename F, typename... Args>
        requires(not plain_invocable<F, Args...>) and std::invocable<F, Args...>
    constexpr auto operator()(F&& fn, Args&&... args) const
        noexcept(std::is_nothrow_invocable_v<F, Args...>)
            -> decltype(std::invoke(mlib_fwd(fn), mlib_fwd(args)...)) {
        return std::invoke(mlib_fwd(fn), mlib_fwd(args)...);
    }
};

/**
 * @brief Implement `std::invoke`, but with significantly less compile/debug
 * overhead for the common case of a plain invocable object.
 */
inline constexpr invoke_fn invoke;

template <typename F, typename... Args>
using invoke_result_t = decltype(invoke(std::declval<F>(), std::declval<Args>()...));

template <typename F, typename... Args>
concept invocable = plain_invocable<F, Args...> or std::invocable<F, Args...>;

}  // namespace mlib
