#pragma once

#include <mlib/config.h>

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

    // Member pointers
    template <typename ObjPtr, typename Object, typename... Args>
    constexpr auto operator()(ObjPtr ptr, Object&& obj, Args&&... args) const
        noexcept(noexcept(mlib_fwd(obj).*ptr(mlib_fwd(args)...)))
            -> decltype(mlib_fwd(obj).*ptr(mlib_fwd(args)...)) {
        return mlib_fwd(obj).*ptr(mlib_fwd(args)...);
    }

    // Member pointers
    template <typename ObjPtr, typename Object>
    constexpr auto operator()(ObjPtr   ptr,
                              Object&& obj) const noexcept  //
        -> decltype(mlib_fwd(obj).*ptr) {
        return mlib_fwd(obj).*ptr;
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
