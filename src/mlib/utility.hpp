#pragma once

#include <exception>
#include <type_traits>
#include <utility>

namespace mlib {

/**
 * @brief A scope-exit utility.
 *
 * When the declared object is destroyed, it will execute the associated function
 */
template <typename F>
class [[nodiscard]] scope_exit {
public:
    scope_exit() = default;
    constexpr scope_exit(F&& fn)
        : _fn(static_cast<F&&>(fn)) {}

    ~scope_exit() {
        if (_alive) {
            _fn();
        }
    }

    constexpr void release() noexcept { _alive = false; }

private:
    bool _alive = true;
    F    _fn;
};

template <typename F>
scope_exit(F&&) -> scope_exit<F>;

/**
 * @brief A scope-exit utility that only executes the attached function if the scope exits via
 * exception
 */
template <typename F>
class [[nodiscard]] scope_fail : scope_exit<F> {
public:
    using scope_fail::scope_exit::scope_exit;

    ~scope_fail() {
        if (std::uncaught_exceptions() <= _n_exc) {
            this->release();
        }
    }

private:
    int _n_exc = std::uncaught_exceptions();
};

template <typename F>
scope_fail(F&&) -> scope_fail<F>;

/**
 * @brief Steal the content of the given object, replacing it with a default-constructed value
 * and returning the previous value
 */
template <typename T>
constexpr T take(T& object) noexcept(noexcept(T(static_cast<T&&>(object))) and noexcept(T())) {
    return std::exchange(object, T());
}

}  // namespace mlib
