#pragma once

#include "./simultaneous.detail.hpp"

namespace amongoc {

/**
 * @brief An operation that executes more than one sub-operation simultaneously.
 * Refer to `create_simultaneous_operation` for more information
 *
 * @tparam Handler The result handler for the operation
 * @tparam Ss The sub-operations that will be executed
 */
template <typename Handler, nanosender... Ss>
class simultaneous_operation
    // Inherit first from a class that stores the handler object
    : detail::simul_handler_storage<Handler>,
      // Inherit second from the impl base class. We do this separation so that
      // the handler object is fully constructed and has a stable location so
      // that it can be passed as a constructor argument to the impl class
      detail::simul_op_impl<simultaneous_operation<Handler, Ss...>,
                            std::index_sequence_for<Ss...>,
                            Ss...> {
public:
    /**
     * @brief Construct the simulnteous_operation object
     *
     * @param h The constructor argument sed to construct the handler object
     */
    constexpr explicit simultaneous_operation(auto&& h, Ss&&... ss)
        // Construct the handler object first:
        : detail::simul_handler_storage<Handler>(mlib_fwd(h))
        , detail::simul_op_impl<simultaneous_operation, std::index_sequence_for<Ss...>, Ss...>(
              // Perfect-forward the senders
              mlib_fwd(ss)...) {}

    // Start all sub-operations
    constexpr void start() noexcept {
        this->_start_all();
        if constexpr (sizeof...(Ss) == 0) {
            this->_handler.empty_start();
        }
    }

private:
    // Allow our simul_recv base class to call _nth_result
    template <typename, typename, std::size_t>
    friend class detail::simul_recv;

    /**
     * @brief Callback invoked when the Nth result value is generated. Called by the
     * simul_recv base class
     *
     * @tparam N The zero-based index of the sender that generated the value
     * @param result The value generated by the sender
     */
    template <std::size_t N>
    constexpr void _nth_result(auto&& result) noexcept {
        // Defer to the handler object
        this->_handler.template nth_result<N>(mlib_fwd(result));
    }
};

/**
 * @brief Create a simultaneous operation object
 *
 * @tparam Handler The handler type that receives the composed operation
 * @param h The constructor argument that is given to create the handler within the composed
 * operation
 * @param ss The senders that will be launched simultaneously for the operation
 * @return A composed operation that executes all input operations simultaneously.
 *
 * Each sub-operation will be connected to an internal receiver that attaches
 * to the constructed handler object.
 *
 * The `Handler` template argument must be given explicitly. The handler object
 * itself will be constructed in-place within the returned operation object using
 * the `h` parameter as the sole constructor argument (this allows immobile
 * handler objects).
 *
 * When the `start()` method is invoked on the composed operation, the `start()`
 * will be invoked on all sub-operations.
 *
 * If the handler object exposes a stop token, then the stop token will be forwarded
 * to each sub-operation automatically.
 */
template <typename Handler, nanosender... Ss>
constexpr nanooperation auto create_simultaneous_operation(auto&& h, Ss&&... ss) noexcept {
    return simultaneous_operation<Handler, Ss...>(mlib_fwd(h), mlib_fwd(ss)...);
}

template <typename H, nanosender... Ss>
constexpr nanooperation auto create_simultaneous_operation(H&& h, Ss&&... ss) noexcept {
    // Deduce the handler from the first argument
    return simultaneous_operation<H, Ss...>(mlib_fwd(h), mlib_fwd(ss)...);
}

}  // namespace amongoc
