#pragma once

#include <mlib/config.h>

#include <exception>
#include <utility>

namespace mlib {

/**
 * @brief A scope-exit utility.
 *
 * When the scope-guard object is destroyed, it will execute the associated function.
 *
 * Execution of the associated guard function can be cancelled using the `release()` method.
 */
template <typename F>
class [[nodiscard]] scope_exit {
public:
    // Default-construct if the function is default-constructible
    scope_exit() = default;
    // Prevent copying. That would almost certainly be not what the user intended
    scope_exit(scope_exit&&) = delete;

    constexpr scope_exit(F&& fn) noexcept(noexcept(static_cast<F>(static_cast<F&&>(fn))))
        : _fn(static_cast<F&&>(fn)) {}

    ~scope_exit() {
        if (_alive) {
            _fn();
        }
    }

    /**
     * @brief Cancel execution of the associated function.
     *
     * Cancellation is idempotent.
     */
    constexpr void release() noexcept { _alive = false; }

private:
    // A flag that determines whether the function should be invoked during destruction
    bool _alive = true;
    // The wrapped function object
    mlib_no_unique_address F _fn;
};

template <typename F>
scope_exit(F&&) -> scope_exit<F>;

/**
 * @brief A scope-exit utility that only executes the attached function if the scope exits via
 * exception
 */
template <typename F>
class [[nodiscard]] scope_fail : public scope_exit<F> {
public:
    using scope_fail::scope_exit::scope_exit;

    ~scope_fail() {
        // Check whether there are more exceptions in-flight than when we began
        if (std::uncaught_exceptions() <= _n_exc) {
            // There are the same or fewer exceptions in-flight. Cancel execution
            this->release();
        }
    }

private:
    // Track the number of in-flight exceptions at the moment the guard is consrtucted
    int _n_exc = std::uncaught_exceptions();
};

template <typename F>
scope_fail(F&&) -> scope_fail<F>;

/**
 * @brief A scope-exit utility that only executes the attached function if the scope exits without
 * throwing an exception
 */
template <typename F>
class [[nodiscard]] scope_success : public scope_exit<F> {
public:
    using scope_success::scope_exit::scope_exit;

    ~scope_success() {
        // Check whether there are more exceptions in-flight than when we began
        if (std::uncaught_exceptions() > _n_exc) {
            // There are the same or more exceptions exceptions in-flight. Cancel execution
            this->release();
        }
    }

private:
    // Track the number of in-flight exceptions at the moment the guard is consrtucted
    int _n_exc = std::uncaught_exceptions();
};

template <typename F>
scope_success(F&&) -> scope_success<F>;

/**
 * @brief Steal the content of the given object, replacing it with a default-constructed value
 * and returning the previous value
 */
template <typename T>
constexpr T take(T& object) noexcept(noexcept(T(static_cast<T&&>(object))) and noexcept(T())) {
    return std::exchange(object, T());
}

}  // namespace mlib
