#pragma once

#include "./concepts.hpp"

#include <mlib/config.h>
#include <mlib/object_t.hpp>

#include <concepts>

namespace amongoc {

/**
 * @brief A sender that yields a value immediately.
 *
 * The receiver for the sender is invoked inline within the start() for the
 * generated operation state
 *
 * The value is perfect-forwarded along.
 */
template <typename T>
class just {
public:
    // Default-construct is only available if the underlying value is default-constructbile
    just() = default;

    // Forward-construct the value in-place
    template <typename U>
        requires std::constructible_from<T, U&&>
    constexpr explicit(not std::convertible_to<U&&, T>) just(U&& arg)
        : _value(mlib_fwd(arg)) {}

    // We send the exact type that we are given (including references)
    using sends_type = T;

    /**
     * @brief Move-connect to a new receiver and return the bound operation
     */
    template <nanoreceiver_of<sends_type> R>
    constexpr nanooperation auto connect(R&& recv) && noexcept {
        return op<R>{static_cast<T&&>(_value), mlib_fwd(recv)};
    }

    /**
     * @brief Copy-connect to a new receiver and return the bound operation
     *
     * This overload will copy-construct the bound value into a new operation,
     * allowing additional connect() to be invoked later on for the same value.
     */
    template <nanoreceiver_of<sends_type> R>
    constexpr nanooperation auto connect(R&& recv) const& noexcept
        requires std::copy_constructible<sends_type>
    {
        return op<R>{static_cast<const T&>(_value), mlib_fwd(recv)};
    }

    constexpr static bool is_immediate() noexcept { return true; }

private:
    template <typename R>
    struct [[nodiscard]] op {
        mlib_no_unique_address T _value;
        mlib_no_unique_address R _receiver;

        constexpr void start() noexcept {
            mlib::invoke(static_cast<R&&>(_receiver), static_cast<T&&>(_value));
        }
    };

    mlib_no_unique_address mlib::object_t<T> _value;
};

template <typename T>
explicit just(T&&) -> just<T>;

}  // namespace amongoc
