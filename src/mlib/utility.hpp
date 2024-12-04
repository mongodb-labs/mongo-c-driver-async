#pragma once

#include <mlib/config.h>

#include <exception>
#include <memory>
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
 * @brief Create a defered execution block in the current lexical scope
 */
#define mlib_defer ::mlib::scope_exit MLIB_PASTE(_mlibScopeExit, __LINE__) = [&]
/**
 * @brief Create a deferred execution block in the current scope, which will only
 * run if we exit via exception.
 */
#define mlib_defer_fail ::mlib::scope_fail MLIB_PASTE(_mlibScopeExit, __LINE__) = [&]

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

/**
 * @brief Delete a pointed-to object using an allocator that is associated with
 * that object.
 *
 * @param inst A (possibly null) pointer to an object which must have an associated
 * allocator. If the pointer is null, this function does nothing.
 */
template <typename T>
void delete_via_associated_allocator(T* inst) {
    if (inst == nullptr) {
        return;
    }
    auto a = inst->get_allocator().template rebind<T>();
    std::allocator_traits<decltype(a)>::destroy(a, inst);
    std::allocator_traits<decltype(a)>::deallocate(a, inst, 1);
}

}  // namespace mlib
