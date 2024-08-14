#pragma once

#include "./nano.hpp"
#include "./stop.hpp"
#include "./util.hpp"

#include <neo/meta.hpp>

namespace amongoc::detail {

/**
 * @brief Bottom base class for simultaneous operations, acting as the receiver
 * for the individual input operations
 *
 * @tparam Derived The most-derived simultaneous_operation class that contains the handler
 * @tparam Handler The operation handler type given to simultaneous_operation
 * @tparam N The zero-based index of the operation
 */
template <typename Derived, typename Handler, typename Sent, std::size_t N>
class simul_recv {
public:
    /**
     * @brief Callback for handling th Nth result from the sender bound to this
     * receiver part.
     *
     * @param result The value from the sender
     */
    constexpr void operator()(Sent&& result) noexcept {
        // Invoke the _nth_result method on the most-derived object, part of
        // simultaneous_operation<>
        static_cast<Derived&>(*this).template _nth_result<N>(NEO_FWD(result));
    }

    // We only provide a stop token if the handler provides a stop token as well
    constexpr auto query(get_stop_token_fn) const noexcept
        requires has_stop_token<Handler>
    {
        return _stop_token;
    }

protected:
    // Construct using the handler's attributes
    constexpr explicit simul_recv(Handler& h) noexcept
        // Grab its stop token. If the handler doesn't provide one, constructs
        // a null_stop_token
        : _stop_token(effective_stop_token(h)) {}

    simul_recv(simul_recv const&) = default;
    simul_recv(simul_recv&&)      = default;

private:
    /**
     * @brief The stop token for this receiver. If the handler does not provide
     * a stop token, this will be a null_stop_token object.
     */
    effective_stop_token_t<Handler> _stop_token;
};

/**
 * @brief One base class that handles a single sender in the composed operation
 *
 * @tparam Derived The most-derived simultaneous_operation class
 * @tparam Handler The handler type passed to create_simultaneous_operation
 * @tparam N Zero-based index of the sender
 * @tparam S The sender that this part will handle
 */
template <typename Derived, typename Handler, std::size_t N, nanosender S>
class simul_one_operation : public simul_recv<Derived, Handler, sends_t<S>, N> {
    // The next base class acts as the receier for the sender
    using base = simul_recv<Derived, Handler, sends_t<S>, N>;

protected:
    constexpr explicit simul_one_operation(Handler& h, S&& s)
        // Construct the base with the handler. This allows the base receiver
        // type to access attributes of the handler (such as its stop token)
        : base(h)
        // Perform the connect operation here, binding the next base class as
        // the receiver by-reference. We need to use the base class rather than
        // define the operator() in this class, because the connect() may require
        // that the receiver be a complete type. The simul_recv base class is
        // fully-defined at this point, but our own class is still incomplete
        , _operation(amongoc::connect(NEO_FWD(s), (base&)(*this))) {}

    // Start the single operation that we handle
    constexpr void _start_one() noexcept { _operation.start(); }

private:
    // The actual operation state for the sender that we control
    connect_t<S, base&> _operation;
};

template <typename Derived, typename Seq, typename Handler, typename... Ss>
class simul_op_impl;
template <typename Derived, std::size_t... Ns, typename Handler, typename... Ss>
class simul_op_impl<Derived, std::index_sequence<Ns...>, Handler, Ss...>
    // Variadic inheritance of a base class for each sender that we will store
    : public simul_one_operation<Derived, Handler, Ns, Ss>... {
protected:
    /**
     * @brief Construct the impl
     *
     * @param h The handler object, stored in a sibling class as part of simultaneous_operation
     * @param ss The senders that we will execute
     */
    constexpr explicit simul_op_impl(Handler& h, Ss&&... ss)
        // Pack-expand-construct each base class with each sender that we want to
        // execute
        : simul_one_operation<Derived, Handler, Ns, Ss>(h, NEO_FWD(ss))... {}

    // Start all sub-operations
    constexpr void _start_all() noexcept {
        // Pack-expand to call each _start_one() on each base class
        (this->simul_one_operation<Derived, Handler, Ns, Ss>::_start_one(), ...);
    }
};

// Storage base for the simul operation handler object
template <typename H>
struct simul_handler_storage {
    // Perfect-forward the one constructor argument
    template <typename Arg>
        requires std::constructible_from<H, Arg>
    constexpr explicit simul_handler_storage(Arg&& h)
        : _handler(NEO_FWD(h)) {}

    // The handler object
    H _handler;
};

}  // namespace amongoc::detail
