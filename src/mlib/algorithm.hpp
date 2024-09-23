#pragma once

#include <ranges>

namespace mlib {

namespace sr = std::ranges;

inline constexpr struct copy_to_capacity_fn {
    /**
     * @brief Copy objects from the input range into the container until the container's
     * capacity is reached or the input range is exhausted
     */
    template <typename Container, sr::input_range R>
    constexpr sr::iterator_t<R>
    operator()(R&& in, Container& out, sr::iterator_t<Container> pos) const
        requires requires(sr::iterator_t<R> it) {
            pos = out.insert(pos, *it);
            out.capacity();
            out.size();
        }
    {
        // Input iterator
        auto iter = sr::begin(in);
        // Input sentinel
        const auto stop = sr::end(in);
        // The capacity of the target container
        const auto capacity = out.capacity();
        // Copy until the container is at capacity
        for (; capacity > out.size() and iter != stop; ++iter) {
            pos = out.insert(pos, *iter);
            // `insert` returns an iterator to the inserted element. Move to the
            // next position after the element we just inserted
            ++pos;
        }
        return iter;
    }

    /**
     * @brief Copy objects from the input range into the container until the container's
     * capacity is reached or the input range is exhausted
     */
    template <typename Container, sr::input_range R>
    constexpr sr::iterator_t<R> operator()(R&& in, Container& out) const
        requires requires(sr::iterator_t<R> it) {
            out.insert(sr::end(out), *it);
            out.capacity();
            out.size();
        }
    {
        return (*this)(in, out, sr::end(out));
    }
} copy_to_capacity;

}  // namespace mlib
