#pragma once

#include <mlib/invoke.hpp>

#include <concepts>

namespace amongoc {

template <typename T, typename F>
class event_listener;

/**
 * @brief An object that allows dispatching of events to a linked-list of event listeners
 *
 * @tparam T The type of object that is fired by the event emitter
 */
template <typename T>
class event_emitter {
public:
    event_emitter() = default;

    /**
     * @brief Fire en event to all listeners
     *
     * @param arg The argument. Will be converted to the event type lazily if they
     * are any listeners
     */
    template <std::convertible_to<T> U>
    constexpr void fire(U&& arg) const noexcept {
        if (not _head) {
            // Nothing to fire. Don't convert to the event type
            return;
        }
        // Create the object to be fired by casting now
        T object(static_cast<U&&>(arg));
        for (auto p = _head; p; p = p->_next) {
            p->do_fire(object);
        }
    }

    // Create a listener associated with this event emitter. The listener will be detached
    // automatically when it is destroyed
    template <typename F>
    constexpr event_listener<T, F> listen(F&& fn) {
        return event_listener<T, F>(*this, static_cast<F&&>(fn));
    }

private:
    // Abstract base class for event listeners for this emitter
    class listener_base {
    protected:
        // Listeners are immobile (their address must be stable)
        listener_base(const listener_base&) = delete;

        // Construct a new listener and register it with the emitter
        constexpr explicit listener_base(event_emitter& owner) noexcept {
            // Take the pointer to the next listener
            _next = owner._head;
            // Point to the pervious next-pointer
            _previous_nextptr = &owner._head;
            if (owner._head) {
                // Update the most recent listener's next-pointer to point to our own next-pointer
                owner._head->_previous_nextptr = &_next;
            }
            // We are now the most recent listener
            owner._head = this;
        }

        ~listener_base() {
            // Splice ourselves out of the linked list of listeners
            *_previous_nextptr = _next;
            if (_next) {
                _next->_previous_nextptr = _previous_nextptr;
            }
        }

    private:
        // Pointer to the next listener in the linked list
        listener_base* _next = nullptr;
        // Pointer to the previous `_next` pointer in the list (or the owner's `_head` pointer)
        listener_base** _previous_nextptr;

        // Dispatch the event
        virtual void do_fire(T obj) noexcept = 0;

        // Allow our emitter to access our internal pointers
        friend event_emitter;
    };

    // Allow a listener to get at `listener_base` and `_head`
    template <typename, typename>
    friend class event_listener;

    // Pointer to the most-recently registered event listener
    listener_base* _head = nullptr;
};

/**
 * @brief An object that listens for event on an event emitter and calls the
 * given function when an event is fired
 *
 * @tparam T The type of events that are emitted
 * @tparam F The event handler that will be called
 */
template <typename T, typename F>
class [[nodiscard]] event_listener : event_emitter<T>::listener_base {
public:
    template <std::convertible_to<F> Fu>
    constexpr explicit event_listener(event_emitter<T>& ev, Fu&& fn)
        : event_listener::listener_base::listener_base(ev)
        , _func(static_cast<Fu&&>(fn)) {}

private:
    [[no_unique_address]] F _func;

    void do_fire(T obj) noexcept override { mlib::invoke(_func, static_cast<T&&>(obj)); }
};

template <typename T, typename F>
explicit event_listener(event_emitter<T>& ev, F&& fn) -> event_listener<T, F>;

}  // namespace amongoc
