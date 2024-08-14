/**
 * @file as_sender.hpp
 * @brief Compat between Asio an nanosender components
 * @date 2024-08-13
 */
#pragma once

#include <amongoc/nano/concepts.hpp>
#include <amongoc/nano/result.hpp>

#include <asio/async_result.hpp>
#include <asio/bind_cancellation_slot.hpp>
#include <asio/error_code.hpp>
#include <neo/unit.hpp>

namespace amongoc {

/**
 * @brief Undefined primary template for the nanosender that will be returned from Asio
 *
 * @tparam Init The underlying initiator invocable (comes from async_result<> below)
 * @tparam Signatures The completion signatures expected by Asio.
 *
 * This template is specialized based on the `Signatures` parameter.
 */
template <typename Init, typename... Signatures>
class asio_nanosender;

// Nanosender adaptor for null completion handlers (no result value)
template <typename Init>
class asio_nanosender<Init, void()> {
public:
    Init _init;
    using sends_type = neo::unit;

    template <nanoreceiver_of<sends_type> R>
    constexpr nanooperation auto connect(R&& recv) && {
        return operation<R>{NEO_MOVE(_init), NEO_FWD(recv)};
    }

    template <typename R>
    struct operation {
        Init _init;
        R    _recv;

        constexpr void start() noexcept {
            auto h = [this] { NEO_INVOKE(static_cast<R&&>(_recv), neo::unit{}); };
            static_cast<Init&&>(_init)(h);
        }
    };
};

/**
 * @brief Handle completion with one result value and an error code
 *
 * @tparam T The success result type
 *
 * @note This will discard the result value in case of error.
 */
template <typename Init, typename T>
class asio_nanosender<Init, void(std::error_code, T)> {
public:
    Init _init;
    using sends_type = result<T, std::error_code>;

    template <nanoreceiver_of<sends_type> R>
    constexpr nanooperation auto connect(R&& recv) && {
        return operation<R>{NEO_MOVE(_init), NEO_FWD(recv)};
    }

    template <typename R>
    struct operation {
        Init _init;
        R    _recv;

        constexpr void start() noexcept {
            auto h = [this](std::error_code ec, T value) {
                if (ec) {
                    NEO_INVOKE(static_cast<R&&>(_recv), sends_type(amongoc::error(ec)));
                } else {
                    NEO_INVOKE(static_cast<R&&>(_recv),
                               sends_type(amongoc::success(NEO_FWD(value))));
                }
            };
            static_cast<Init&&>(_init)(NEO_MOVE(h));
        }
    };
};

// Asio CompletionToken type, specialized with asio::async_result
struct asio_as_nanosender_t {
    template <typename... Signatures, typename Init>
    static constexpr nanosender auto make_nanosender(Init&& initiator) {
        return asio_nanosender<Init, Signatures...>(NEO_FWD(initiator));
    }
};

/**
 * @brief Asio CompletionToken that creates a nanosender for an asynchronous operation
 */
inline constexpr asio_as_nanosender_t asio_as_nanosender;

}  // namespace amongoc

/**
 * @brief Specialization of asio::async_result that allows amongoc::asio_as_nanosender
 * to work as a CompletionToken for Asio operations
 *
 * @tparam Signatures The completion signatures of the requested operation
 */
template <typename... Signatures>
struct asio::async_result<amongoc::asio_as_nanosender_t, Signatures...> {
    /**
     * @brief The initiation function for Asio. Refer to the docs on async_result for info
     *
     * @param init The actual initiation function from Asio
     * @param args Additional arguments for the initiation function
     * @return A new nanosender that encapsulates the operation
     */
    template <typename Initiator, typename... Args>
    constexpr static amongoc::nanosender auto
    initiate(Initiator init, amongoc::asio_as_nanosender_t a, Args&&... args) noexcept {
        return a.make_nanosender<Signatures...>(
            [init = NEO_MOVE(init), ... args = NEO_FWD(args)](auto&& handler) mutable {
                NEO_MOVE(init)(NEO_FWD(handler), static_cast<Args&&>(args)...);
            });
    }
};