#pragma once

#include "./nano.hpp"

#include <amongoc/nano/query.hpp>

#include <mlib/config.h>

namespace amongoc::detail {

/**
 * @brief Bottom base class for simultaneous operations, acting as the receiver
 * for the individual input operations
 *
 * @tparam Derived The most-derived simultaneous_operation class that contains the handler
 * @tparam N The zero-based index of the operation
 */
template <typename Derived, typename Sent, std::size_t N>
class simul_recv {
public:
    simul_recv() = default;

    /**
     * @brief Callback for handling th Nth result from the sender bound to this
     * receiver part.
     *
     * @param result The value from the sender
     */
    constexpr void operator()(Sent&& result) noexcept {
        // Invoke the _nth_result method on the most-derived object, part of
        // simultaneous_operation<>
        static_cast<Derived&>(*this).template _nth_result<N>(mlib_fwd(result));
    }

    // Forward queries to _query_handler, implemented below
    constexpr auto query(auto q) const noexcept
        requires requires(const Derived d) { d._query_handler(q); }
    {
        return static_cast<const Derived&>(*this)._query_handler(q);
    }

protected:
    // Prevent accidentally slicing this object
    simul_recv(simul_recv const&) = default;
    simul_recv(simul_recv&&)      = default;
};

/**
 * @brief One base class that handles a single sender in the composed operation
 *
 * @tparam Derived The most-derived simultaneous_operation class
 * @tparam N Zero-based index of the sender
 * @tparam S The sender that this part will handle
 */
template <typename Derived, std::size_t N, nanosender S>
class simul_one_operation : public simul_recv<Derived, sends_t<S>, N> {
    // The next base class acts as the receier for the sender
    using base = simul_recv<Derived, sends_t<S>, N>;

protected:
    constexpr explicit simul_one_operation(S&& s)
        // Perform the connect operation here, binding the next base class as
        // the receiver by-reference. We need to use the base class rather than
        // define the operator() in this class, because the connect() may require
        // that the receiver be a complete type. The simul_recv base class is
        // fully-defined at this point, but our own class is still incomplete
        : _operation(amongoc::connect(mlib_fwd(s), (base&)(*this))) {}

    // Start the single operation that we handle
    constexpr void _start_one() noexcept { _operation.start(); }

private:
    // The actual operation state for the sender that we control
    [[no_unique_address]] connect_t<S, base&> _operation;
};

template <typename Derived, typename Seq, typename... Ss>
class simul_op_impl;
template <typename Derived, std::size_t... Ns, typename... Ss>
class simul_op_impl<Derived, std::index_sequence<Ns...>, Ss...>
    // Variadic inheritance of a base class for each sender that we will store
    : public simul_one_operation<Derived, Ns, Ss>... {
protected:
    /**
     * @brief Construct the impl
     *
     * @param h The handler object, stored in a sibling class as part of simultaneous_operation
     * @param ss The senders that we will execute
     */
    constexpr explicit simul_op_impl(Ss&&... ss)
        // Pack-expand-construct each base class with each sender that we want to
        // execute
        : simul_one_operation<Derived, Ns, Ss>(mlib_fwd(ss))... {}

    // Start all sub-operations
    constexpr void _start_all() noexcept {
        // Pack-expand to call each _start_one() on each base class
        (this->simul_one_operation<Derived, Ns, Ss>::_start_one(), ...);
    }
};

// Storage base for the simul operation handler object
template <typename H>
struct simul_handler_storage {
    // Perfect-forward the one constructor argument
    template <typename Arg>
        requires std::constructible_from<H, Arg>
    constexpr explicit simul_handler_storage(Arg&& h)
        : _handler(mlib_fwd(h)) {}

    // The handler object.
    // XXX: Can't use [[no_unique_address]] here? It seems to break defer_convert()
    H _handler;

    // Handles query operations for simul_recv above
    constexpr auto _query_handler(valid_query_for<H> auto q) const noexcept { return q(_handler); }
};

}  // namespace amongoc::detail
