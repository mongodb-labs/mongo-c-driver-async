#pragma once

#include "./nano/util.hpp"

#include <forward_list>

namespace amongoc {

/**
 * @brief Defines an object pool, with the following guarantees:
 *
 * 1. The objets within the pool have stable addresses.
 * 2. A checked out ticket will keep the object from the pool until it is destroyed.
 *
 * @tparam T The type of objects stored within the pool. It is permitted to be immobile.
 */
template <typename T>
class object_pool {
public:
    object_pool() = default;

    /**
     * @brief Construct a new object pool with a constrained maximum count of objects
     *
     * @param max_size The maximum number of objects allowed in the pool. Excess
     * objects will be destroyed when they attempt to return to the pool.
     */
    explicit object_pool(std::size_t max_size) noexcept
        : _max_size(max_size) {}

    /**
     * @brief A move-only handle to an object stored in the pool
     *
     * This object provides a pointer-like interface to the stored object.
     *
     * When the ticket is destroyed, the object will automatically return to
     * the pool.
     */
    class ticket {
    public:
        // Move-only
        ticket(ticket&&)            = default;
        ticket& operator=(ticket&&) = default;

        // Get the underlying object
        T&       operator*() noexcept { return _one.front(); }
        T*       operator->() noexcept { return &**this; }
        const T& operator*() const noexcept { return _one.front(); }
        const T* operator->() const noexcept { return &**this; }

        // Get a pointer to the object
        T*       get() noexcept { return &_one.front(); }
        const T* get() const noexcept { return &_one.front(); }

        // Destroy the object immediately without returning it to the pool
        void discard() && noexcept { _one.clear(); }

        ~ticket() {
            // The _one will be empty if we are moved-from or discarded
            if (_one.empty()) {
                // Give it back
                _parent->return_(_one);
            }
        }

    private:
        friend object_pool<T>;

        constexpr explicit ticket(object_pool& p, std::forward_list<T>&& l) noexcept
            : _parent(&p)
            , _one(std::move(l)) {}

        object_pool*         _parent;
        std::forward_list<T> _one;
    };

    /**
     * @brief Obtain a new object from the pool, default-constructing if needed
     */
    constexpr ticket checkout() {
        return checkout([] { return T(); });
    }

    /**
     * @brief Obtain an object from the pool
     *
     * @param factory A factory function that will be invoked IF a new object is needed.
     */
    template <typename F>
    constexpr ticket checkout(F&& factory) {
        std::forward_list<T> one;
        if (_objects.empty()) {
            // Nothing in the pool. Construct a new object
            one.emplace_front(defer_convert([&] { return std::forward<F>(factory)(); }));
        } else {
            // Splice the front object to the one-item list to be returned.
            one.splice_after(one.before_begin(), _objects, _objects.before_begin());
            --_count;
        }
        return ticket(*this, std::move(one));
    }

private:
    // The actual objects
    std::forward_list<T> _objects;
    // The current number of objects we hold (forward_list does not keep count)
    std::size_t _count = 0;
    // The maximum number of objects to retain in the pool
    std::size_t _max_size = 1024;

    friend ticket;

    // Return an object to the pool, stored as a single item in a forward-list
    void return_(std::forward_list<T>& one) {
        if (_count < _max_size) {
            // We are allowed to add to the pool. Splice to the front:
            _objects.splice_after(_objects.before_begin(), one, one.cbefore_begin());
            ++_count;
        }
    }
};

}  // namespace amongoc