#pragma once

#include "./concepts.hpp"
#include "./nano.hpp"
#include "./util.hpp"

#include <amongoc/nano/query.hpp>

#include <mlib/object_t.hpp>

#include <concepts>

namespace amongoc::detail {

/**
 * @brief Sender returned by the then() algorithm
 *
 * @tparam InputSender The sender that is being fed to then()
 * @tparam Transformer The user's transformation function
 */
template <nanosender InputSender, mlib::invocable<sends_t<InputSender>> Transformer>
class then_sender {
public:
    constexpr explicit then_sender(InputSender&& s, Transformer&& fn) noexcept
        : _input_sender(mlib_fwd(s))
        , _transformer(mlib_fwd(fn)) {}

    /// The type sent by the then() sender is the type returned by the transformer
    /// function when invoked with the object returned by the input sender
    using sends_type = mlib::invoke_result_t<Transformer, sends_t<InputSender>>;

    constexpr auto connect(nanoreceiver_of<sends_type> auto&& recv) && {
        return amongoc::connect(static_cast<InputSender&&>(_input_sender),
                                atop(mlib_fwd(recv), static_cast<Transformer&&>(_transformer)));
    }

    constexpr auto connect(nanoreceiver_of<sends_type> auto&& recv) const&
        requires multishot_nanosender<InputSender> and std::copy_constructible<Transformer>
    {
        return amongoc::connect(static_cast<InputSender const&>(_input_sender),
                                atop(mlib_fwd(recv),
                                     decay_copy(static_cast<Transformer const&>(_transformer))));
    }

    template <valid_query_for<InputSender> Q>
    constexpr query_t<Q, InputSender> query(Q q) const noexcept {
        return q(static_cast<InputSender const&>(_input_sender));
    }

    // This sender is immediate if the input sender is also immediate
    constexpr bool is_immediate() const noexcept {
        return amongoc::is_immediate(static_cast<const InputSender&>(_input_sender));
    }

    constexpr static bool is_immediate() noexcept
        requires statically_immediate<InputSender>
    {
        return true;
    }

private:
    mlib_no_unique_address mlib::object_t<InputSender> _input_sender;
    mlib_no_unique_address mlib::object_t<Transformer> _transformer;
};

}  // namespace amongoc::detail
