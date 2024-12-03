#pragma once

#include <mlib/config.h>
#include <mlib/object_t.hpp>

#include <concepts>
#include <memory>
#include <mutex>
#include <optional>

namespace mlib {

/**
 * @brief A lazy-initialized object storage
 *
 * @tparam T The type being stored
 * @tparam Init The initializer function object
 */
template <typename T, typename Init>
    requires std::convertible_to<mlib::invoke_result_t<Init>, T>
class lazy_threadsafe {
public:
    lazy_threadsafe() = default;
    explicit lazy_threadsafe(Init&& init)
        : _init(mlib_fwd(init)) {}

    /**
     * @brief Obtain the stored value, initializing it if necessary
     *
     * @return An l-value reference to the stored value
     */
    T& operator*() {
        std::call_once(this->_init_flag, [&] { _value.emplace(mlib::invoke(mlib_fwd(_init))); });
        return static_cast<T&>(*_value);
    }

    const T& operator*() const {
        std::call_once(this->_init_flag, [&] { _value.emplace(mlib::invoke(mlib_fwd(_init))); });
        return static_cast<const T&>(*_value);
    }

    std::add_pointer_t<T>       operator->() { return std::addressof(**this); }
    std::add_pointer_t<T const> operator->() const { return std::addressof(**this); }

private:
    /// Thread-safe call-once flag.
    mutable std::once_flag _init_flag;
    /// The initializer function object
    mlib_no_unique_address mutable Init _init;
    /// Storage for the object
    mutable std::optional<mlib::object_t<T>> _value;
};

}  // namespace mlib
