#pragma once

#include "./concepts.hpp"
#include "./nano.hpp"
#include "./util.hpp"

#include <neo/fwd.hpp>
#include <neo/invoke.hpp>
#include <neo/object_t.hpp>

#include <concepts>

namespace amongoc::detail {

/**
 * @brief Sender returned by the then() algorithm
 *
 * @tparam InputSender The sender that is being fed to then()
 * @tparam Transformer The user's transformation function
 */
template <nanosender InputSender, neo::invocable2<sends_t<InputSender>> Transformer>
class then_sender {
public:
    constexpr explicit then_sender(InputSender&& s, Transformer&& fn) noexcept
        : _input_sender(NEO_FWD(s))
        , _transformer(NEO_FWD(fn)) {}

    /// The type sent by the then() sender is the type returned by the transformer
    /// function when invoked with the object returned by the input sender
    using sends_type = neo::invoke_result_t<Transformer, sends_t<InputSender>>;

    constexpr auto connect(nanoreceiver_of<sends_type> auto&& recv) && {
        return amongoc::connect(static_cast<InputSender&&>(_input_sender),
                                atop(NEO_FWD(recv), static_cast<Transformer&&>(_transformer)));
    }

    constexpr auto connect(nanoreceiver_of<sends_type> auto&& recv) const&
        requires multishot_nanosender<InputSender> and std::copy_constructible<Transformer>
    {
        return amongoc::connect(static_cast<InputSender const&>(_input_sender),
                                atop(NEO_FWD(recv),
                                     auto(static_cast<Transformer const&>(_transformer))));
    }

    constexpr auto query(valid_query_for<InputSender> auto q) const noexcept {
        return static_cast<InputSender const&>(_input_sender).query(q);
    }

private:
    NEO_NO_UNIQUE_ADDRESS neo::object_t<InputSender> _input_sender;
    NEO_NO_UNIQUE_ADDRESS neo::object_t<Transformer> _transformer;
};

}  // namespace amongoc::detail
