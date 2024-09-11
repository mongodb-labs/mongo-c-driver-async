#pragma once

#include "./concepts.hpp"
#include "./nano.hpp"
#include "./util.hpp"

#include <neo/attrib.hpp>

namespace amongoc {

struct tie_fn {
    /**
     * @brief Tie a nanosender with a destination for the sent value
     *
     * @param snd The nanosender that will produce the result value
     * @param dest The target of the assignment when the sender resolves
     * @return constexpr auto A new operation state that for the bound operation
     */
    template <nanosender S, std::assignable_from<sends_t<S>> Dest>
    constexpr nanooperation auto operator()(S&& snd, Dest&& dest) const noexcept {
        struct recv {
            NEO_NO_UNIQUE_ADDRESS Dest _dst;

            void operator()(sends_t<S>&& value) { _dst = NEO_FWD(value); }
        };
        return amongoc::connect(NEO_FWD(snd), recv{NEO_FWD(dest)});
    }

    // Create a partially-applied tie() that assigns a nanosender's value to the target
    template <typename D>
    auto operator()(D&& d) const noexcept {
        return make_closure(tie_fn{}, NEO_FWD(d));
    }
};

/**
 * @brief Tie a nanosender to an assignment destination
 *
 * The value sent by the nanosender will be assigned to the given storage
 */
inline constexpr tie_fn tie;

}  // namespace amongoc
