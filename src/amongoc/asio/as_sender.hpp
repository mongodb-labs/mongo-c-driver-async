/**
 * @file as_sender.hpp
 * @brief Compat between Asio an nanosender components
 * @date 2024-08-13
 */
#pragma once

#include <amongoc/nano/concepts.hpp>
#include <amongoc/nano/result.hpp>

#include <mlib/config.h>

#include <asio/async_result.hpp>
#include <asio/error_code.hpp>

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
    mlib_no_unique_address Init _init;
    using sends_type = mlib::unit;

    template <nanoreceiver_of<sends_type> R>
    constexpr nanooperation auto connect(R&& recv) && {
        return operation<R>{std::move(_init), mlib_fwd(recv)};
    }

    template <typename R>
    struct operation {
        mlib_no_unique_address Init _init;
        mlib_no_unique_address R    _recv;

        constexpr void start() noexcept {
            auto h = [this] { mlib::invoke(static_cast<R&&>(_recv), sends_type()); };
            static_cast<Init&&>(_init)(h);
        }
    };
};

/**
 * @brief Handle completion with either success or an error code
 */
template <typename Init>
class asio_nanosender<Init, void(std::error_code)> {
public:
    mlib_no_unique_address Init _init;
    using sends_type = result<mlib::unit, std::error_code>;

    template <nanoreceiver_of<sends_type> R>
    constexpr nanooperation auto connect(R&& recv) && {
        return operation<R>{std::move(_init), mlib_fwd(recv)};
    }

    template <typename R>
    struct operation {
        mlib_no_unique_address Init _init;
        mlib_no_unique_address R    _recv;

        constexpr void start() noexcept {
            auto h = [this](std::error_code ec) {
                if (ec) {
                    mlib::invoke(static_cast<R&&>(_recv), sends_type(amongoc::error(ec)));
                } else {
                    mlib::invoke(static_cast<R&&>(_recv), sends_type(amongoc::success()));
                }
            };
            static_cast<Init&&>(_init)(std::move(h));
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
    mlib_no_unique_address Init _init;
    using sends_type = result<T, std::error_code>;

    template <nanoreceiver_of<sends_type> R>
    constexpr nanooperation auto connect(R&& recv) && {
        return operation<R>{std::move(_init), mlib_fwd(recv)};
    }

    template <typename R>
    struct operation {
        mlib_no_unique_address Init _init;
        mlib_no_unique_address R    _recv;

        constexpr void start() noexcept {
            auto h = [this](std::error_code ec, T value) {
                if (ec) {
                    mlib::invoke(static_cast<R&&>(_recv), sends_type(amongoc::error(ec)));
                } else {
                    mlib::invoke(static_cast<R&&>(_recv),
                                 sends_type(amongoc::success(mlib_fwd(value))));
                }
            };
            static_cast<Init&&>(_init)(std::move(h));
        }
    };
};

// Asio CompletionToken type, specialized with asio::async_result
struct asio_as_nanosender_t {
    template <typename... Signatures, typename Init>
    static constexpr nanosender auto make_nanosender(Init&& initiator) {
        return asio_nanosender<Init, Signatures...>(mlib_fwd(initiator));
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
            [init = std::move(init), ... args = mlib_fwd(args)](auto&& handler) mutable {
                std::move(init)(mlib_fwd(handler), static_cast<Args&&>(args)...);
            });
    }
};
