#pragma once

#include <neo/fwd.hpp>
#include <neo/invoke.hpp>
#include <neo/type_traits.hpp>

#include <concepts>
#include <type_traits>

namespace amongoc {

/**
 * @brief Traits class the allows customization of the behavior of a nanosender
 *
 * The primary definition is empty. If `T` contains a `sends_type`, this activates
 * a more specialized version
 */
template <typename T>
struct nanosender_traits {};

/**
 * @brief Request the sends_type from teh nanosender_traits on the given type
 */
template <typename T>
using sends_t = nanosender_traits<std::remove_cvref_t<T>>::sends_type;

/**
 * @brief Match a receiver that can receive the given type
 */
template <typename Recv, typename Result>
concept nanoreceiver_of = neo::invocable2<Recv, Result>;

/**
 * @brief Archetype for operation types
 */
struct archetype_nanooperation {
    void start() noexcept;
};

/**
 * @brief Archetype of senders that send type T
 */
template <typename T>
struct archetype_nanosender {
    // Empty: Content is in the specialization of nanosender_traits
};

template <typename T>
struct nanosender_traits<archetype_nanosender<T>> {
    using sends_type = T;

    static archetype_nanooperation connect(const archetype_nanosender<T>&,
                                           nanoreceiver_of<sends_type> auto&& recv) noexcept;
};

/**
 * @brief Archetype for receivers for type `T`
 */
template <typename T>
struct archetype_nanoreceiver {
    void operator()(T&& arg) {}
};

/**
 * @brief A type which is a nano operation, providing a noexcept(true) start() method
 */
template <typename T>
concept nanooperation = requires(T& operation) {
    { operation.start() } noexcept;
};

/**
 * @brief Match a type which is a nanosender
 *
 * A sender must expose a `sends_type` that informs what type will be emitted
 * from the sender. The sender must be connect-able to a receiver that accepts
 * `sends_type` as its sole argument. The sender must be connectible via r-value
 * to *this
 */
template <typename Sender>
concept nanosender = requires {
    typename sends_t<Sender>;
} and requires(std::remove_cvref_t<Sender>&& sender, archetype_nanoreceiver<sends_t<Sender>> recv) {
    {
        nanosender_traits<std::remove_cvref_t<Sender>>::connect(NEO_FWD(sender), NEO_FWD(recv))
    } -> nanooperation;
};

/**
 * @brief Match a sender that can be connected to multiple receivers.
 *
 * This requires that the type be a nanosender, but it must be valid to call `connect()`
 * via `const S&` in addition to `S&&`
 */
template <typename S>
concept multishot_nanosender = nanosender<S>
    and requires(std::remove_cvref_t<S> const& sender, archetype_nanoreceiver<sends_t<S>> recv) {
            {
                nanosender_traits<std::remove_cvref_t<S>>::connect(sender, NEO_FWD(recv))
            } -> nanooperation;
        };

/**
 * @brief Match a sender that sends a value convertible to the given type
 */
template <typename Sender, typename Result>
concept nanosender_of = nanosender<Sender> and std::convertible_to<sends_t<Sender>, Result>;

/**
 * @brief Match a receiver that can receive the type send by the given sender
 */
template <typename Recv, typename Sender>
concept nanoreceiver_for = nanoreceiver_of<Recv, sends_t<Sender>>;

/**
 * @brief Default impl of nanosender_traits for types that expose a sends_type
 */
template <typename T>
    requires requires { typename T::sends_type; }
struct nanosender_traits<T> {
    using sends_type = T::sends_type;

    template <neo::alike<T> S, nanoreceiver_of<sends_type> R>
    static constexpr decltype(auto) connect(S&& sender, R&& recv) noexcept
        requires requires {
            { NEO_FWD(sender).connect(NEO_FWD(recv)) } -> nanooperation;
        }
    {
        return NEO_FWD(sender).connect(NEO_FWD(recv));
    }

    // Check for is_immediate() as a member function
    static constexpr bool is_immediate(const T& sender) noexcept
        requires requires { sender.is_immediate(); }
    {
        return sender.is_immediate();
    }

    // If no is_immediate() is present, assume it is not immediate
    static constexpr bool is_immediate(const T&) noexcept { return false; }

    // Statically-checked is_immediate()
    static constexpr bool is_immediate() noexcept
        requires requires { requires T::is_immediate(); }
    {
        return true;
    }
    static constexpr bool is_immediate() noexcept { return false; }
};

/**
 * @brief Evaluates to `true` if the given nanosender has statically immediate (i.e. its immediacy
 * is guaranteed)
 */
template <typename S>
concept statically_immediate
    = nanosender<S> and requires { requires nanosender_traits<S>::is_immediate(); };

/**
 * @brief Returns `true` if the given nanosender will send its value immediately
 */
template <nanosender S>
constexpr bool is_immediate(const S& s) {
    return statically_immediate<S> or nanosender_traits<S>::is_immediate(s);
}

}  // namespace amongoc
