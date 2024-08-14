#pragma once

#include "./let.detail.hpp"

#include "./concepts.hpp"
#include "./util.hpp"

#include <utility>

namespace amongoc {

struct let_fn {
    /**
     * @brief Connect a sender with a handler function that returns a new sender
     * to continue a composed asynchronous operation.
     *
     * @param sender The input sender
     * @param handler A handler that returns a new sender
     * @return A new sender that represents the composed operation
     */
    template <nanosender InputSender, detail::valid_let_handler<InputSender> Transformer>
    constexpr nanosender auto operator()(InputSender&& sender,
                                         Transformer&& handler) const noexcept {
        return detail::let_sender<InputSender, Transformer>{NEO_FWD(sender), NEO_FWD(handler)};
    }

    /**
     * @brief Create a pipeable object that can be used to composed a sender
     * with a handler that returns the continuation sender
     */
    template <typename Handler>
    constexpr auto operator()(Handler&& hnd) const noexcept {
        return make_closure(let_fn{}, NEO_FWD(hnd));
    }
};

inline constexpr let_fn let;

template <nanosender S, detail::valid_let_handler<S> H>
using let_t = decltype(let(std::declval<S>(), std::declval<H>()));

}  // namespace amongoc
