#pragma once

#include <mlib/config.h>

#include <stddef.h>
#include <stdint.h>

#if mlib_is_cxx()
#include <mlib/detail/cxx20.hpp>

#include <new>
#endif

mlib_extern_c_begin();

/**
 * @brief An object that provides memory allocation functionality, can be used
 * to customize allocation behavior
 */
typedef struct mlib_allocator mlib_allocator;

struct mlib_allocator_impl {
    /**
     * @brief Pointer to a user-provided object for the allocator context
     */
    void* userdata;

    /**
     * @brief Allocate/free/reallocate memory
     *
     * @param userdata The `userdata` pointer for the allocator
     * @param prev_ptr Pointer to a previous allocated region that should be reclaimed
     * @param requested_size The new size of the allocated region, or zero to request deallocation
     * @param previous_size The previous size of the allocated region
     * @param out_new_size Output parameter: The size of the newly allocated region
     *
     * @return A pointer to the allocated region, or a null pointer if alloation failed.
     *
     * @note After a successful call, the `prev_ptr` is no longer valid
     * @note If allocation fails, the `prev_ptr` region will be unmodified and the pointer remains
     * valid
     */
    void* (*reallocate)(void*   userdata,
                        void*   prev_ptr,
                        size_t  requested_size,
                        size_t  alignment,
                        size_t  previous_size,
                        size_t* out_new_size)mlib_noexcept;
};

struct mlib_allocator {
    struct mlib_allocator_impl const* impl;
};

/**
 * @brief Reallocate an existing region using an allocator
 *
 * @param alloc The allocator to be used
 * @param prev_ptr Pointer to the previously allocated region
 * @param new_size The new requested size of the region. If zero, the region is deallocated.
 * @param prev_size The previously requested size of the region
 * @param out_new_size Output pointer that receives the new region size
 * @return A pointer to the reallocated region, or NULL on failure, or NULL if `new_size` is zero
 */
mlib_constexpr void* mlib_reallocate(mlib_allocator alloc,
                                     void*          prev_ptr,
                                     size_t         new_size,
                                     size_t         alignment,
                                     size_t         prev_size,
                                     size_t*        out_new_size) mlib_noexcept {
    return alloc.impl
        ->reallocate(alloc.impl->userdata, prev_ptr, new_size, alignment, prev_size, out_new_size);
}

/**
 * @brief Allocate a new region using an `mlib_allocator`
 *
 * Returns NULL on allocation failure.
 */
mlib_constexpr void* mlib_allocate(mlib_allocator alloc, size_t sz) mlib_noexcept {
    return mlib_reallocate(alloc, NULL, sz, mlib_alignof(max_align_t), 0, &sz);
}

/**
 * @brief Deallocate a region that was obtained from the given `mlib_allocator`
 */
mlib_constexpr void mlib_deallocate(mlib_allocator alloc, void* p, size_t sz) mlib_noexcept {
    if (!p) {
        return;
    }
    mlib_reallocate(alloc, p, 0, 0, sz, &sz);
}

/**
 * @brief A default allocator that uses the standard allocation functions (malloc/realloc/free)
 */
extern const mlib_allocator mlib_default_allocator;

/**
 * @brief An allocator that immediately terminates the program if an attempt is made to
 * allocate memory.
 *
 * Use this in cases where you wish to assert that an API will not allocate memory.
 */
extern const mlib_allocator mlib_terminating_allocator;

mlib_extern_c_end();

#if mlib_is_cxx()

namespace mlib {

/**
 * @brief A C++-style allocator that adapts an `mlib_allocator`
 *
 * @tparam T The type being managed. IF `void`, this is a proto-allocator.
 */
template <typename T = void>
class allocator {
public:
    using value_type = T;
    using pointer    = value_type*;

    // Construct around an existing mlib_allocator object
    constexpr allocator(mlib_allocator a) noexcept
        : _alloc(a) {}

    // Convert-construct from an allocator of another type
    template <typename U>
    constexpr allocator(allocator<U> o) noexcept
        : _alloc(o.c_allocator()) {}

    // Obtain the C-style allocator managed by this object
    constexpr mlib_allocator c_allocator() const noexcept { return _alloc; }

    // Allocate N objects
    constexpr pointer allocate(size_t n) const {
        const size_t max_count = SSIZE_MAX / sizeof(T);
        if (n > max_count) {
            // Multiplying would overflow
            throw std::bad_alloc();
        }
        pointer p = static_cast<pointer>(
            ::mlib_reallocate(_alloc, nullptr, n * sizeof(T), alignof(T), 0, &n));
        if (p == nullptr) {
            throw std::bad_alloc();
        }
        return p;
    }

    // Deallocate N object
    constexpr void deallocate(pointer p, size_t n) const {
        return ::mlib_deallocate(_alloc, p, n * sizeof(T));
    }

    // Compare two allocators. They are equivalent if the underlying C allocators are equal
    template <typename U>
    constexpr bool operator==(allocator<U> other) const noexcept {
        return _alloc.impl == other.c_allocator().impl;
    }

    // Utility to dynamically allocate and construct a single object using this allocator
    template <typename... Args>
    constexpr pointer new_(Args&&... args) const {
        pointer p = this->allocate(1);
        this->construct(p, mlib_fwd(args)...);
        return p;
    }

    // Utility to destroy and deallocate a single object that was allocated with this allocator
    constexpr void delete_(pointer p) const noexcept {
        if (p) {
            p->~T();
            this->deallocate(p, 1);
        }
    }

    // Utility to rebind the value type of the allocator
    template <typename U>
    constexpr allocator<U> rebind() const noexcept {
        return *this;
    }

    // Construct the object, injecting this allocator if appropriate
    template <typename... Args>
    constexpr void construct(pointer p, Args&&... args) const {
        mlib::detail::uninitialized_construct_using_allocator(p, *this, mlib_fwd(args)...);
    }

private:
    // The adapted C allocator
    mlib_allocator _alloc;
};

// C++ wrapper around `::amongoc_terminating_allocator`, and allocator which immediately terminates
inline const allocator<> terminating_allocator = allocator<>{::mlib_terminating_allocator};

/**
 * @brief Bind a memory allocator to an invocable object
 *
 * @tparam Alloc The type of the allocator to be bound
 * @tparam T The object being wrapped
 */
template <typename Alloc, typename T>
class bind_allocator {
public:
    bind_allocator() = default;

    constexpr explicit bind_allocator(Alloc alloc, T&& obj)
        : _object(mlib_fwd(obj))
        , _alloc(alloc) {}

    // This type alias is required for some external libraries that expect a declared
    // allocator_type in addition to get_allocator() (e.g. Asio)
    using allocator_type = Alloc;
    constexpr allocator_type get_allocator() const noexcept { return _alloc; }

private:
    [[no_unique_address]] T              _object;
    [[no_unique_address]] allocator_type _alloc;

public:
    template <typename... Args>
    constexpr auto operator()(Args&&... args) & -> decltype(_object(mlib_fwd(args)...)) {
        return _object(mlib_fwd(args)...);
    }

    template <typename... Args>
    constexpr auto operator()(Args&&... args) const& -> decltype(_object(mlib_fwd(args)...)) {
        return _object(mlib_fwd(args)...);
    }

    template <typename... Args>
    constexpr auto
    operator()(Args&&... args) && -> decltype(std::move(*this)._object(mlib_fwd(args)...)) {
        return std::move(*this)._object(mlib_fwd(args)...);
    }

    template <typename... Args>
    constexpr auto
    operator()(Args&&... args) const&& -> decltype(std::move(*this)._object(mlib_fwd(args)...)) {
        return std::move(*this)._object(mlib_fwd(args)...);
    }

    // Forward other queries to the underlying type
    template <typename Q>
    constexpr auto query(Q q) const noexcept -> decltype(q(_object)) {
        return q(_object);
    }
};

template <typename Alloc, typename T>
explicit bind_allocator(Alloc, T&&) -> bind_allocator<Alloc, T>;

/**
 * @brief Query function object type that obtains the allocator associated with an object
 */
struct get_allocator_fn {
    // Base case: No associated allocator
    template <typename O, typename A>
    constexpr static A impl(detail::rank<0>, const O&, const A dflt) {
        return dflt;
    }

    // Case: Has a query() for get_allocator_fn
    template <typename O, typename A>
    constexpr static auto impl(detail::rank<1>, const O& obj, const A /* dflt */)
        -> decltype(obj.query(std::declval<get_allocator_fn const&>())) {
        return obj.query(get_allocator_fn{});
    }
    template <typename O>
    constexpr static auto impl(detail::rank<1>, const O& obj)
        -> decltype(obj.query(std::declval<get_allocator_fn const&>())) {
        return obj.query(get_allocator_fn{});
    }

    // Case: Has a get_allocator() member
    template <typename O, typename A>
    constexpr static auto impl(detail::rank<2>, const O& obj, const A /* dflt */)
        MLIB_RETURNS(obj.get_allocator());
    template <typename O>
    constexpr static auto impl(detail::rank<2>, const O& obj)  //
        MLIB_RETURNS(obj.get_allocator());

    /**
     * @brief Obtain the associated allocator. Is not invocable if there is no associated allocator
     */
    template <typename O>
    constexpr auto operator()(const O& obj) const  //
        MLIB_RETURNS(impl(detail::rank<5>{}, obj));

    /**
     * @brief Obtain the associated allocator, or a default allocator if there is no associated
     * allocator
     */
    template <typename O, typename A>
    constexpr auto operator()(const O& obj, A dflt) const
        MLIB_RETURNS(impl(detail::rank<5>{}, obj, dflt));
};

/**
 * @brief Obtain the allocator associated with an object, if available
 */
inline constexpr struct get_allocator_fn get_allocator{};

/**
 * @brief Obtain the type of allocator associated with an object
 */
template <typename T>
using get_allocator_t = decltype(get_allocator(std::declval<const T&>()));

#if mlib_have_cxx20()
/**
 * @brief Match a type for which `mlib::get_allocator` will return an object
 *
 * @tparam T
 */
template <typename T>
concept has_allocator = requires(const T& obj) { get_allocator(obj); };

/**
 * @brief Match a type that provides an associated allocator that is convertible to
 * an `mlib::allocator<>`
 */
template <typename T>
concept has_mlib_allocator
    = has_allocator<T> and requires(allocator<> a, const T obj) { a = get_allocator(obj); };
#endif  // ≥C++20

}  // namespace mlib

#endif  // C++
