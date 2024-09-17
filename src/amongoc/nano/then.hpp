#pragma once

#include "./concepts.hpp"
#include "./stop.hpp"
#include "./then.detail.hpp"
#include "./util.hpp"

#include <utility>

namespace amongoc {

struct _then {
    /**
     * @brief Create a new sender that transforms the result from an input sender
     *
     * @param s The input sender
     * @param f A transforming function
     *
     * The returned sender returns the result type of invoking `f` with the result
     * sent by `s`
     */
    template <nanosender InputSender, neo::invocable2<sends_t<InputSender>> Transformer>
    constexpr detail::then_sender<InputSender, Transformer>
    operator()(InputSender&& s, Transformer&& f) const noexcept {
        return detail::then_sender<InputSender, Transformer>(mlib_fwd(s), mlib_fwd(f));
    }

    template <typename F>
    constexpr auto operator()(F&& fn) const noexcept {
        return make_closure(_then{}, mlib_fwd(fn));
    }
};

/**
 * @brief Create a continuation from a sender
 */
inline constexpr _then then;

template <nanosender S, neo::invocable2<sends_t<S>> F>
using then_t = decltype(then(std::declval<S>(), std::declval<F>()));

}  // namespace amongoc
