#pragma once

#include <mlib/config.h>
#include <mlib/delete.h>

namespace mlib {

/**
 * @brief Creates an move-only wrapper around a C type that defines deletion
 * semantics.
 *
 * @tparam T The C type being owned
 * @tparam Deleter The deletion mechanism
 *
 * - Usage of CTAD is supported and recommended.
 * - Invoking the deleter on a default-constructed instance of `T` must be a no-op.
 * - Requires that the deleter is invocable with a modifiable l-value of type `T`
 */
template <typename T, typename Deleter = unique_deleter<T>>
    requires requires(T& inst) { Deleter{}(inst); }
class unique {
public:
    // Default-construct to a default-constructed instance
    unique() = default;

    // Deletes the held object
    ~unique() { reset(); }

    /**
     * @brief Take ownership of the given object
     */
    constexpr unique(T&& inst) noexcept
        : _instance(inst) {
        inst = T();
    }

    /**
     * @brief Move-construct, taking ownership from the other instance
     */
    constexpr unique(unique&& other) noexcept
        : _instance(static_cast<unique&&>(other).release()) {}

    /**
     * @brief Move-assign, destroying the held value and taking ownership from the other
     */
    constexpr unique& operator=(unique&& other) noexcept {
        reset(static_cast<unique&&>(other).release());
        return *this;
    }

    /**
     * @brief Destroy the held object
     *
     * @return constexpr T& The newly held default-constructed instance
     */
    constexpr T& reset() noexcept(noexcept(T())) { return reset(T()); }
    /**
     * @brief Destroy the held object and replace it with a new value
     *
     * @param value The value to take
     * @return constexpr T& A reference to the stored object.
     */
    constexpr T& reset(T&& value) noexcept(noexcept(T())) {
        Deleter{}(_instance);
        _instance = value;
        value     = T();
        return _instance;
    }

    /**
     * @brief Relinquish ownership of the object and return it to the caller
     */
    [[nodiscard]] constexpr T release() && noexcept {
        T ret     = _instance;
        _instance = T();
        return ret;
    }

    constexpr T*       operator->() noexcept { return &_instance; }
    constexpr const T* operator->() const noexcept { return &_instance; }
    constexpr T&       get() noexcept { return _instance; }
    constexpr const T& get() const noexcept { return _instance; }
    constexpr T&       operator*() noexcept { return _instance; }
    constexpr const T& operator*() const noexcept { return _instance; }

private:
    mlib_no_unique_address T _instance = T();
};

template <typename T>
unique(T) -> unique<T>;

}  // namespace mlib
