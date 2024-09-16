#pragma once

#include "./concepts.hpp"
#include "./query.hpp"
#include "./simultaneous.hpp"
#include "./stop.hpp"
#include "./util.hpp"

#include <neo/attrib.hpp>
#include <neo/concepts.hpp>
#include <neo/like.hpp>
#include <neo/object_t.hpp>

#include <mutex>
#include <tuple>
#include <variant>

namespace amongoc {

namespace detail {

// Check that `P` is a valid sender predicate for each result type
template <typename P, std::size_t... Ns, typename... Ts>
void _check_sender_predicate(P& p, std::index_sequence<Ns...>, Ts&... ts)
    requires requires {
        // Each invocation of `p(size_constant<N>, r)` must be valid and return a boolean
        ((p(size_constant<Ns>{}, ts) ? 0 : 0), ...);
    };

struct never {
    struct undefined;
    never(undefined);

    explicit operator auto() const noexcept { std::terminate(); }
};

template <typename P, typename... Ts>
struct infer_result_type {
    using type = std::variant<Ts...>;
};

template <typename P>
struct infer_result_type<P> {
    using type = never;
};

template <typename P, typename... Ts>
    requires requires { P::template result_type<Ts...>; }
struct infer_result_type<P, Ts...> {
    using type = P::template result_type<Ts...>;
};

}  // namespace detail

template <typename Predicate, typename... Ss>
concept sender_predicate = requires(Predicate& p, sends_t<Ss>&... res) {
    detail::_check_sender_predicate(p, std::index_sequence_for<Ss...>{}, res...);
};

/**
 * @brief Create a composed operation of one or more senders, where the first
 * operation to finish and whose result value satisfies the predicate will cancel all other
 * operations, and the receiver will be invoked with a variant corresponding to that result.
 *
 * When the resulting operation is started, then all sub-operations will also
 * be started. When an operation resolves and the operation has not already completed,
 * then the predicate will be invoked as `p(size_constant<N>, r)` where `N` is the
 * zero-based index of the sender that completed. If the predicate returns `true`
 * for that result, then all other sub-operations will be cancelled. Once all operations
 * have resolved, the receiver will be invoked with the value that caused the success.
 *
 * @note The operation will wait until all input operations resolve, even after a cancellation
 * has been requested. If any other operations do not properly support cancellation, then the
 * operation will stall waiting for those operations to complete.
 *
 * @note If no result satisfies the predicate, then `std::terminate()` will be invoked!
 *
 * The zero-based index of the result variant corresponds to the zero-based index
 * of the sender that first resolved to generate the value.
 *
 * @tparam Ss The senders that will be raced
 */
template <typename Predicate, nanosender... Ss>
    requires sender_predicate<Predicate, Ss...>
class first_where {
public:
    /// Default-constructible
    first_where() = default;

    /**
     * @brief Forward-construct each sender within the composed operation
     */
    constexpr explicit first_where(Predicate&& pr, neo::explicit_convertible_to<Ss> auto&&... ss)
        : _predicate(NEO_FWD(pr))
        , _senders(NEO_FWD(ss)...) {}

    // We send a variant of the result values from each incoming sender.
    using sends_type = typename detail::infer_result_type<Predicate, sends_t<Ss>...>::type;

    // Move-connect the sender.
    template <nanoreceiver_of<sends_type> R>
    constexpr nanooperation auto connect(R&& recv) && noexcept {
        return _connect(NEO_MOVE(_senders),
                        static_cast<Predicate&&>(_predicate),
                        NEO_FWD(recv),
                        std::index_sequence_for<Ss...>{});
    }

    // Copy-connect the sender. Requires that all input senders are multi-shot as well.
    template <nanoreceiver_of<sends_type> R>
    constexpr nanooperation auto connect(R&& recv) const& noexcept
        requires(multishot_nanosender<Ss> and ...)
    {
        return _connect(_senders,
                        decay_copy(static_cast<Predicate const&>(_predicate)),
                        NEO_FWD(recv),
                        std::index_sequence_for<Ss...>{});
    }

    constexpr bool is_immediate() const noexcept {
        return std::apply(
            [](const auto&... ss) {
                return (amongoc::is_immediate(static_cast<const Ss&>(ss)) and ...);
            },
            _senders);
    }

    constexpr static bool is_immediate() noexcept
        requires(statically_immediate<Ss> and ...)
    {
        return true;
    }

private:
    NEO_NO_UNIQUE_ADDRESS neo::object_t<Predicate> _predicate;
    NEO_NO_UNIQUE_ADDRESS std::tuple<neo::object_t<Ss>...> _senders;

    // Impl for connect(). Perfect-forwards the senders from the given tuple
    template <typename Tpl, typename P, typename Recv, std::size_t... Ns>
    constexpr static nanooperation auto
    _connect(Tpl&& tpl, P&& p, Recv&& recv, std::index_sequence<Ns...> is) noexcept {
        return amongoc::create_simultaneous_operation<
            handler<Recv, P>>(defer_convert(
                                  [&] { return handler<Recv, P>(NEO_FWD(recv), NEO_FWD(p)); }),
                              static_cast<neo::forward_like_tuple_t<Tpl, Ss>>(
                                  std::get<Ns>(NEO_FWD(tpl)))...);
    }

    // Simultaneous operation handler. See create_simultaneous_operation()
    // This object is stored within the operation state returned by connect()
    template <nanoreceiver_of<sends_type> R, typename P>
    struct handler {
        // Forward-construct the receiver into place. This constructor is called
        // by simultaneous_operation<>
        constexpr explicit handler(R&& r, P&& pred)
            : _final_recv(NEO_FWD(r))
            , _predicate(NEO_FWD(pred)) {}

        // The final receiver
        NEO_NO_UNIQUE_ADDRESS R _final_recv;
        // The value predicate
        NEO_NO_UNIQUE_ADDRESS P _predicate;
        // The stop token that is used to stop all operations once any of the input
        // operations completes
        in_place_stop_source _stopper;

        // Number of pending operations
        std::size_t _outstanding = sizeof...(Ss);

        // Storage for the final result. Fulfilled when the first accepted operation completes
        std::optional<sends_type> _result;

        // Stopping forwarder that will attach a possible stop signal from _final_recv to the
        // stop token for this object.
        NEO_NO_UNIQUE_ADDRESS stop_forwarder<R, in_place_stop_source&> _stop_fwd{_final_recv,
                                                                                 _stopper};

        // Synchronization for updating internal state
        std::mutex _mtx;

        auto get_stop_token() const noexcept { return _stopper.get_token(); }

        constexpr auto query(valid_query_for<R> auto q) const noexcept { return q(_final_recv); }

        // Callback handler when the Nth result is received from the input
        template <std::size_t N>
        constexpr void nth_result(auto&& x) {
            // Test this result against the user's predicate to determine whether
            // to forward or discard
            const bool       accepted = _predicate(size_constant<N>{}, x);
            std::unique_lock lk{_mtx};
            // Decrement the number of outstanding senders that may still resolve
            // in the future.
            const auto nremain = --_outstanding;
            if (not _result.has_value() and accepted) {
                // We are the first result that is accepted.
                // Stop all pending operations
                _stopper.request_stop();
                // Construct the final result object, to later be sent to the final receiver
                // XXX: This holds the lock while the constructor is running, which is less than
                // ideal. Could this nth_result be made lock-free?
                _result.emplace(std::in_place_index<N>, NEO_FWD(x));
            }
            // Allow other threads to continue
            lk.unlock();
            if (nremain == 0) {
                // We are the last result to be completed
                if (not _result.has_value()) {
                    // We were the last possible operation, and no one will be able to finish here
                    this->_handle_no_accept();
                }
                // Invoke the final receiver with the chosen result object
                NEO_INVOKE(static_cast<R&&>(_final_recv), NEO_MOVE(*_result));
            }
        }

        void _handle_no_accept() {
            // The predicate doesn't tell us what to do. Terminate
            std::terminate();
        }

        void _handle_no_accept()
            requires requires(R recv) { _predicate.on_none_accepted(NEO_FWD(recv)); }
        {
            // Allow the predicate object to decide what should be done next
            _predicate.on_none_accepted(static_cast<R&&>(_final_recv));
        }

        void empty_start() { this->_handle_no_accept(); }
    };
};

template <typename P, nanosender... Ss>
    requires sender_predicate<P, Ss...>
explicit first_where(P&&, Ss&&...) -> first_where<P, Ss...>;

struct always {
    constexpr bool operator()(auto&&...) const noexcept { return true; }
};

/**
 * @brief Create a composed operation of one or more senders, where the first
 * operation to finish will cancel all other operations, and the receiver will
 * be invoked with a variant corresponding to the result.
 *
 * @see `first_where` for more details. This is equivalent to `first_where` with
 * a predicate that immediately accepts any value.
 *
 * @tparam Ss The senders that will be raced
 */
template <nanosender... Ss>
class first_completed : public first_where<always, Ss...> {
public:
    /// Default-constructible
    first_completed() = default;
    /**
     * @brief Forward-construct each sender within the composed operation
     */
    constexpr explicit first_completed(neo::explicit_convertible_to<Ss> auto&&... ss)
        : first_where<always, Ss...>(always{}, NEO_FWD(ss)...) {}
};

template <nanosender... Ss>
explicit first_completed(Ss&&...) -> first_completed<Ss...>;

}  // namespace amongoc
