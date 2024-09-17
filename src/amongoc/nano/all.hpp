#pragma once

#include "./concepts.hpp"
#include "./query.hpp"
#include "./simultaneous.hpp"
#include "./util.hpp"

#include <mlib/object_t.hpp>

#include <atomic>
#include <cstddef>
#include <optional>
#include <tuple>

namespace amongoc {

/**
 * @brief Create a sender that resolves after all input senders resolve
 *
 * @tparam Ss The input senders to be executed
 *
 * The result type is a tuple of all objects that are sent by the input senders
 */
template <nanosender... Ss>
class when_all {
public:
    constexpr explicit when_all(Ss&&... ss)
        : _senders(mlib_fwd(ss)...) {}

    // Sends a tuple of all sub-results
    using sends_type = std::tuple<sends_t<Ss>...>;

    // Move-connect the composed operation
    template <nanoreceiver_of<sends_type> R>
    constexpr nanooperation auto connect(R&& recv) && {
        return _connect(std::move(_senders), mlib_fwd(recv), std::index_sequence_for<Ss...>{});
    }

    // Copy-connect the composed operation
    template <nanoreceiver_of<sends_type> R>
    constexpr nanooperation auto connect(R&& recv) const&
        requires(multishot_nanosender<Ss> and ...)
    {
        return _connect(decay_copy(_senders), mlib_fwd(recv), std::index_sequence_for<Ss...>{});
    }

private:
    // Input senders of the operation
    std::tuple<mlib::object_t<Ss>...> _senders;

    template <typename Tpl, typename R, std::size_t... Ns>
    constexpr static nanooperation auto _connect(Tpl&& tpl, R&& recv, std::index_sequence<Ns...>) {
        return amongoc::create_simultaneous_operation<
            handler<R>>(mlib_fwd(recv),
                        static_cast<neo::forward_like_tuple_t<Tpl, Ss>>(
                            std::get<Ns>(mlib_fwd(tpl)))...);
    }

    // Simultaneous operation handler for the composed operation
    template <nanoreceiver_of<sends_type> R>
    struct handler {
        // Move-construct the handler into place
        explicit handler(R&& recv)
            : _recv(mlib_fwd(recv)) {}

        // The user's final receiver
        R _recv;

        // Optional storage for each sub-operation's result
        std::tuple<std::optional<mlib::object_t<sends_t<Ss>>>...> _opts{};

        // Number of pending operations.
        std::atomic_size_t _n_remaining{sizeof...(Ss)};

        template <valid_query_for<R> Q>
        constexpr query_t<Q, R> query(Q q) const noexcept {
            return q(_recv);
        }

        // Handle the Nth result from a sub-operation
        template <std::size_t N>
        void nth_result(auto&& x) {
            // Construct the result:
            std::get<N>(_opts).emplace(mlib_fwd(x));
            if (_n_remaining.fetch_sub(1) == 1) {
                // All results have been fulfilled.
                _finish(std::index_sequence_for<Ss...>{});
            }
        }

        template <std::size_t... Ns>
        void _finish(std::index_sequence<Ns...>) {
            // Construct the final result tuple
            auto fin = sends_type(static_cast<sends_t<Ss>&&>(*std::get<Ns>(_opts))...);
            // Invoke the final receiver
            NEO_INVOKE(std::move(_recv), std::move(fin));
        }

        // Invoked when when_all() is given no senders to wait on (finishes immediately)
        void empty_start() { _finish(std::index_sequence<>{}); }
    };
};

template <nanosender... Ss>
explicit when_all(Ss&&...) -> when_all<Ss...>;

}  // namespace amongoc
