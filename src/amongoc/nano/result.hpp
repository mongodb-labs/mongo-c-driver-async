#pragma once

#include "./concepts.hpp"
#include "./nano.hpp"
#include "./util.hpp"

#include <amongoc/status.h>

#include <neo/attrib.hpp>
#include <neo/concepts.hpp>
#include <neo/invoke.hpp>
#include <neo/like.hpp>
#include <neo/object_box.hpp>

#include <cassert>
#include <exception>
#include <new>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <variant>

namespace amongoc {

// Tag type that carries the success() arguments to a result<T>
template <typename... Args>
struct success_tag {
    std::tuple<Args&&...> args;
};

// Tag type that carries the error() arguments to a result<T>
template <typename... Args>
struct error_tag {
    std::tuple<Args&&...> args;
};

// Create a constructor tag for result<T> that imbues it with a success
constexpr auto success
    = []<typename... Args>(Args&&... args) -> success_tag<Args...> { return {{NEO_FWD(args)...}}; };

// Create a constructor tag for result<T> that imbues it with an error
constexpr auto error
    = []<typename... Args>(Args&&... args) -> error_tag<Args...> { return {{NEO_FWD(args)...}}; };

// Define the behavior of `result<T, E>` for handling the error type `E`
template <typename E>
struct error_traits {};

/**
 * @brief An error-or-value sum type. A `result<T, E>` holds either a `T` in its
 * success state, or an `E` in its error state.
 *
 * @tparam T The success-value type
 * @tparam E The error-value type (default is `amongoc_status`)
 *
 * @note Construct using the `success` and `error` utilities in namespace scope
 */
template <typename T, typename E = amongoc_status>
class result {
public:
    using success_type = T;
    using error_type   = E;

    template <typename U>
    using rebind = result<U, E>;

    result() = default;

    template <neo::unalike<result> U>
        requires neo::explicit_convertible_to<U&&, T>
        and (not neo::explicit_convertible_to<U &&, E>)
    explicit(not neo::implicit_convertible_to<U&&, T>) constexpr result(U&& arg)
        : result(success(NEO_FWD(arg))) {}

    /// Construct a success-valued result object
    template <typename... Args>
        requires neo::constructible_from<T, Args&&...>
    constexpr result(success_tag<Args...> success)
        : result(success, NEO_MOVE(success).args, std::index_sequence_for<Args...>{}) {}

    /// Construct an error-valued result object
    template <typename... Args>
        requires neo::constructible_from<E, Args&&...>
    constexpr result(error_tag<Args...> err)
        : result(err, NEO_MOVE(err).args, std::index_sequence_for<Args...>{}) {}

    /// Returns `true` iff the result has a success value
    constexpr bool has_value() const noexcept { return _stored.index() == 0; }
    /// Returns `true` iff the result has an error value
    constexpr bool has_error() const noexcept { return _stored.index() == 1; }

    /**
     * @brief Create an error_tag for the error contained by this result, useful
     * for forwarding an error from one result to another
     */
    constexpr decltype(auto) error_tag() & {
        assert(has_error());
        return amongoc::error(this->error());
    }
    constexpr decltype(auto) error_tag() const& {
        assert(has_error());
        return amongoc::error(this->error());
    }
    constexpr decltype(auto) error_tag() && {
        assert(has_error());
        return amongoc::error(NEO_MOVE(*this).error());
    }
    constexpr decltype(auto) error_tag() const&& {
        assert(has_error());
        return amongoc::error(NEO_MOVE(*this).error());
    }

    /**
     * @brief Obtain the value stored in the object, throwing in case of error
     */
    constexpr decltype(auto) value() & {
        this->_maybe_throw();
        return std::get_if<0>(&_stored)->get();
    }
    constexpr decltype(auto) value() const& {
        this->_maybe_throw();
        return std::get_if<0>(&_stored)->get();
    }
    constexpr decltype(auto) value() && {
        this->_maybe_throw();
        return std::get_if<0>(&_stored)->forward();
    }
    constexpr decltype(auto) value() const&& {
        this->_maybe_throw();
        return std::get_if<0>(&_stored)->forward();
    }

    constexpr decltype(auto) operator*() & {
        this->_maybe_throw();
        return this->value();
    }
    constexpr decltype(auto) operator*() const& {
        this->_maybe_throw();
        return this->value();
    }
    constexpr decltype(auto) operator*() && {
        this->_maybe_throw();
        return NEO_MOVE(*this).value();
    }
    constexpr decltype(auto) operator*() const&& {
        this->_maybe_throw();
        return NEO_MOVE(*this).value();
    }

    constexpr decltype(auto) error() & {
        assert(has_error());
        return std::get_if<1>(&_stored)->get();
    }
    constexpr decltype(auto) error() const& {
        assert(has_error());
        return std::get_if<1>(&_stored)->get();
    }
    constexpr decltype(auto) error() && {
        assert(has_error());
        return std::get_if<1>(&_stored)->forward();
    }
    constexpr decltype(auto) error() const&& {
        assert(has_error());
        return std::get_if<1>(&_stored)->forward();
    }

    template <typename F>
    constexpr auto transform(F&& fn) const& -> result<std::invoke_result_t<F, T const&>> {
        if (has_value()) {
            return success(NEO_FWD(fn)(this->value()));
        } else {
            return this->error_tag();
        }
    }

    template <typename F>
    constexpr auto transform(F&& fn) && -> result<std::invoke_result_t<F, T&&>> {
        if (has_value()) {
            return success(NEO_FWD(fn)(NEO_MOVE(*this).value()));
        } else {
            return this->error_tag();
        }
    }

private:
    template <typename... Args, std::size_t... Ns>
    constexpr explicit result(success_tag<Args...> const&, auto&& tpl, std::index_sequence<Ns...>)
        : _stored(std::in_place_index<0>, std::in_place, std::get<Ns>(NEO_FWD(tpl))...) {}

    template <typename... Args, std::size_t... Ns>
    constexpr explicit result(amongoc::error_tag<Args...> const&,
                              auto&& tpl,
                              std::index_sequence<Ns...>)
        : _stored(std::in_place_index<1>, std::in_place, std::get<Ns>(NEO_FWD(tpl))...) {}

    std::variant<neo::object_box<T>, neo::object_box<E>> _stored;

    constexpr void _maybe_throw() const {
        if (this->has_error()) {
            error_traits<E>::throw_exception(this->error());
        }
    }
};

template <>
struct error_traits<std::exception_ptr> {
    [[noreturn]] static void throw_exception(std::exception_ptr e) { std::rethrow_exception(e); }
};

template <>
struct error_traits<std::error_code> {
    [[noreturn]] static void throw_exception(std::error_code const& e) {
        throw std::system_error(e);
    }
};

template <neo::derived_from<std::exception> E>
struct error_traits<E> {
    [[noreturn]] static void throw_exception(const E& e) { throw e; }
};

template <>
struct error_traits<amongoc_status> {
    [[noreturn]] static void throw_exception(amongoc_status s) { throw amongoc::exception(s); }
};

template <typename T>
constexpr bool is_result_v = false;

template <typename T, typename E>
constexpr bool is_result_v<result<T, E>> = true;

/**
 * @brief Transform a function (T -> U) to a function (result<T> -> result<U>)
 *
 * @tparam F The wrapped function type. (Can be deduced via CTAD)
 *
 * If the wrapped function is invoked with a successful result, then the underlying
 * function is invoked with the result's success value. Otherwise, the result's error
 * value is rebound into a new result<U>
 */
template <typename F>
struct result_fmap {
    // The mapped function
    F _fn;

    /**
     * @brief Invoke the fmapped function
     *
     * @param res The result value of type `R`
     * @return A new `result<invoke_result_t<F, T>, E>`
     *
     * requires that the wrapped invocable is invocable with the success type of
     * the given result object.
     */
    template <typename R,
              typename Rd          = std::remove_cvref_t<R>,
              typename SuccessType = Rd::success_type,
              typename InvokeResult
              = neo::invoke_result_t<F&&, neo::forward_like_t<R, SuccessType>>,
              typename FinalResult = Rd::template rebind<InvokeResult>>
        requires is_result_v<Rd>
    constexpr FinalResult operator()(R&& res) {
        if (res.has_value()) {
            // The result has a success value. Forward that to the wrapped invocable and
            // wrap it in a new result:
            return success(NEO_INVOKE(NEO_FWD(_fn), NEO_FWD(res).value()));
        } else {
            // The result is errant. Just construct a new errant result
            return NEO_FWD(res).error_tag();
        }
    }
};

template <typename F>
explicit result_fmap(F&&) -> result_fmap<F>;

/**
 * @brief Flatten a `result<result<T>>` to a `result<T>`
 */
inline constexpr struct result_join_fn {
    template <typename T, typename E>
    constexpr result<T, E> operator()(result<result<T, E>, E>&& r) const noexcept {
        if (r.has_value()) {
            if (r.value().has_value()) {
                return success(NEO_MOVE(r).value().value());
            } else {
                return error(NEO_MOVE(r).value().error());
            }
        } else {
            return error(NEO_MOVE(r).error());
        }
    }

    template <typename T, typename E>
    constexpr result<T, E> operator()(result<result<T, E>, E> const& r) const noexcept {
        if (r.has_value()) {
            if (r.value().has_value()) {
                return success(r.value().value());
            } else {
                return error(r.value().error());
            }
        } else {
            return error(r.error());
        }
    }
} result_join;

/**
 * @brief Define nanosender semantics of a result type whose success type is itself
 * a nanosender
 *
 * @tparam T The result's value type, which must itself be a nanosender
 * @tparam E The result's error type, which can be anything
 *
 * The receiver for a nanosender of this kind will be invoked with a `result<sends_t<T>, E>`
 *
 * If the result contains a nanosender value, then the adapted operation will create an operation
 * from that nanosender, and will start it as part of its `start()` method.
 *
 * If the result contains an error, then the adapted operation will not construct an operation
 * state, and its `start()` method will immediately invoke the receiver with the error of the
 * result.
 */
template <nanosender T, typename E>
struct nanosender_traits<result<T, E>> {
    using inner_sends = sends_t<T>;
    using sends_type  = result<inner_sends, E>;

    template <nanoreceiver_of<sends_type> R>
    static constexpr nanooperation auto connect(result<T, E>&& r, R&& recv) {
        return op<R>{NEO_FWD(r), NEO_FWD(recv)};
    }

    template <nanoreceiver_of<sends_type> R>
    static constexpr nanooperation auto connect(result<T, E> const& r, R&& recv) {
        return op<R>{r, NEO_FWD(recv)};
    }

    template <nanoreceiver_of<sends_type> R>
    struct op {
        // Wrap the receiver with an invocable that transforms the success value from
        // the input sender into a result
        struct wrapped_recv {
            NEO_NO_UNIQUE_ADDRESS R _wrapped;

            template <valid_query_for<R> Q>
            constexpr query_t<Q, R> query(Q q) const {
                return q(_wrapped);
            }

            constexpr void operator()(inner_sends value) {
                // Invoke the underlying receiver with a new result created from the success of
                // the wrapped nanosender
                NEO_INVOKE(static_cast<R&&>(_wrapped), sends_type(success(NEO_FWD(value))));
            }
        };

        constexpr explicit op(result<T, E>&& res, R recv)
            : _result(NEO_MOVE(res))
            , _recv(NEO_FWD(recv)) {
            if (_result.has_value()) {
                // The input result has a value, which is the nanosender that we
                // will connect immediately here:
                _real_oper.emplace(defer_convert([&] -> connect_t<T, wrapped_recv> {
                    return amongoc::connect(NEO_MOVE(_result).value(),
                                            wrapped_recv{NEO_FWD(_recv)});
                }));
            }
        }

        // The result from which this operation was connected. Will be moved-from in
        // the constructor iff the result has a success value
        result<T, E> _result;
        // The operation receiver. Will be moved-from in the constructor if the result
        // has a success value.
        NEO_NO_UNIQUE_ADDRESS R _recv;

        // The continued operation created by connecting the value from _result to
        // the receiver _recv. Will be non-null iff the input result has a success value
        std::optional<connect_t<T, wrapped_recv>> _real_oper;

        constexpr void start() noexcept {
            if (_real_oper.has_value()) {
                // The underlying operation was created in the constructor, so just
                // defer to that one immediately.
                _real_oper->start();
            } else {
                // The real operation was not constructed, which means that the result
                // value holds an error. Invoke the receiver immediately with the same
                // error that is contained in the result:
                NEO_INVOKE(NEO_MOVE(_recv), sends_type(NEO_MOVE(_result).error_tag()));
            }
        }
    };
};

}  // namespace amongoc
