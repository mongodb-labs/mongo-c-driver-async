#pragma once

#include "./concepts.hpp"
#include "./nano.hpp"
#include "./query.hpp"
#include "./stop.hpp"
#include "./util.hpp"

#include <concepts>
#include <optional>

namespace amongoc::detail {

/**
 * @brief Intermediate receiver for the let() algorithm
 *
 * @tparam Transformer The transformer function provided to let()
 * @tparam NextReceiver The user's final receiver for the final result
 * @tparam NextOperation The operation state type that we will construct for the chained operation
 */
template <typename T, typename Transformer, typename NextReceiver, typename NextOperation>
class let_recv {
public:
    constexpr explicit let_recv(Transformer&& h, NextReceiver&& r)
        : _transform(NEO_FWD(h))
        , _next_recv(NEO_FWD(r)) {}

    constexpr void operator()(T&& result) noexcept {
        // The let() receiver must be invoked at most once. A well-formed operation will only
        // invoke its receiver one time, so this should never occur. If this assertion fires, it
        // indicates that the input sender is erroneous.
        assert(not _next_operation.has_value() && "let() transformer was invoked multiple times");
        // Invoke the transformer to obtain the next sender in the chained operation
        nanosender auto next_sender
            = NEO_INVOKE(static_cast<Transformer&&>(_transform), NEO_FWD(result));
        // Construct the operation state from the new sender and our final receiver
        _next_operation.emplace(amongoc::defer_convert([&] {
            return amongoc::connect(NEO_MOVE(next_sender), static_cast<NextReceiver&&>(_next_recv));
        }));
        // Initiate the next operation immediately
        _next_operation->start();
    }

    template <valid_query_for<NextReceiver> Q>
    constexpr query_t<Q, NextReceiver> query(Q q) const {
        return q(_next_recv);
    }

private:
    NEO_NO_UNIQUE_ADDRESS Transformer  _transform;
    std::optional<NextOperation>       _next_operation;
    NEO_NO_UNIQUE_ADDRESS NextReceiver _next_recv;
};

/**
 * @brief Sender type returned by the let() algorithm
 *
 * @tparam InputSender The sender that is being transformed
 * @tparam Transformer The user's transformation function
 */
template <typename InputSender, typename Transformer>
class let_sender {
public:
    constexpr explicit let_sender(InputSender&& in, Transformer&& tr)
        : _input_sender(NEO_FWD(in))
        , _transformer(NEO_FWD(tr)) {}

    /// The sender type that is returned by the user's transformer function when fed the result from
    /// the input sender
    using intermediate_sender_type = neo::invoke_result_t<Transformer, sends_t<InputSender>>;
    /// The final sent type that comes from the generated intermediate sender
    using sends_type = sends_t<intermediate_sender_type>;

    /// Forward the scheduler from the input sender
    template <valid_query_for<InputSender> Q>
    constexpr query_t<Q, InputSender> query(Q q) const noexcept {
        return q(_input_sender.get());
    }

    // Move-connect the operation
    template <nanoreceiver_of<sends_type> R>
    constexpr nanooperation auto connect(R&& recv) && noexcept {
        // Perfect-forward the sender and transformer
        return op<R>{_input_sender.forward(), _transformer.forward(), NEO_FWD(recv)};
    }

    // Copy-connect the operation
    template <nanoreceiver_of<sends_type> R>
    constexpr nanooperation auto connect(R&& recv) const& noexcept
        requires std::copy_constructible<Transformer> and std::copy_constructible<InputSender>
        and multishot_nanosender<InputSender>
    {
        // Copy the input sender and the transformer
        return op<R>{auto(_input_sender.get()), auto(_transformer.get()), NEO_FWD(recv)};
    }

    // We are an immediate sender type if:
    // 1. The input sender is immediate
    // 2. The intermediate sender is statically immediate (i.e. we know it is immediate before
    //    even getting an instance of it)
    constexpr bool is_immediate() const noexcept
        requires statically_immediate<intermediate_sender_type>
    {
        return amongoc::is_immediate(_input_sender.get());
    }

    // We are statically immediate if both sender types are statically immediate
    constexpr static bool is_immediate() noexcept
        requires statically_immediate<InputSender>
        and statically_immediate<intermediate_sender_type>
    {
        return true;
    }

private:
    /// The let operation state type
    template <typename FinalReceiver>
    class op {
    public:
        constexpr explicit op(InputSender&& snd, Transformer&& hnd, FinalReceiver&& recv) noexcept
            /// Connect the input sender to the intermediate receiver
            : _input_operation(
                amongoc::connect(NEO_FWD(snd),
                                 intermediate_receiver(NEO_FWD(hnd), NEO_FWD(recv)))) {}

        /// The intermediate operation state that will be created after the user's
        /// transformer is invoked and connected to the final receiver
        using next_operation = connect_t<intermediate_sender_type, FinalReceiver>;

        /// The intermediate receiver. This accepts the incoming value and constucts+starts the
        /// subsequent operation.
        using intermediate_receiver
            = let_recv<sends_t<InputSender>, Transformer, FinalReceiver, next_operation>;

        /// The input operation state.
        using input_operation = connect_t<InputSender, intermediate_receiver>;

        /// Start the input operation.
        constexpr void start() noexcept { _input_operation.start(); }

    private:
        NEO_NO_UNIQUE_ADDRESS input_operation _input_operation;
    };

    /// The input sender
    NEO_NO_UNIQUE_ADDRESS neo::object_box<InputSender> _input_sender;
    /// The user's transformation function
    NEO_NO_UNIQUE_ADDRESS neo::object_box<Transformer> _transformer;
};

/// Require that the given handler returns a new sender when invoked with the given sender's
/// result type
template <typename Handler, typename Sender>
concept let_handler_returns_sender = nanosender<neo::invoke_result_t<Handler, sends_t<Sender>>>;

/**
 * @brief Match a handler that is invocable with the type for the given sender and
 * returns a new sender
 */
template <typename Handler, typename Sender>
concept valid_let_handler                          //
    = nanosender<Sender>                           //
    and neo::invocable2<Handler, sends_t<Sender>>  //
    and let_handler_returns_sender<Handler, Sender>;

}  // namespace amongoc::detail
