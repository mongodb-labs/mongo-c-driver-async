#pragma once

#include "./config.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
#include <memory>
#endif

AMONGOC_EXTERN_C_BEGIN

/**
 * @brief An object that provides memory allocation functionality, can be used
 * to customize allocation behavior
 */
typedef struct amongoc_allocator amongoc_allocator;

struct amongoc_allocator {
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
     * @param previous_size The previous size of the allocate region
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
                        size_t  previous_size,
                        size_t* out_new_size)AMONGOC_NOEXCEPT;
};

/**
 * @brief Allocate a new region using an `amongoc_allocator`
 *
 * Returns NULL on allocation failure.
 */
inline void* amongoc_allocate(amongoc_allocator alloc, size_t sz) AMONGOC_NOEXCEPT {
    return alloc.reallocate(alloc.userdata, NULL, sz, 0, &sz);
}

/**
 * @brief Deallocate a region that was obtained from the given `amongoc_allocator`
 */
inline void amongoc_deallocate(amongoc_allocator alloc, void* p, size_t sz) AMONGOC_NOEXCEPT {
    alloc.reallocate(alloc.userdata, p, 0, sz, &sz);
}

/**
 * @brief A default allocator that uses the standard allocation functions (malloc/realloc/free)
 */
extern const amongoc_allocator amongoc_default_allocator;

/**
 * @brief An allocator that immediately terminates the program if an attempt is made to
 * allocate memory.
 *
 * Use this in cases where you wish to assert that an API will not allocate memory.
 */
extern const amongoc_allocator amongoc_terminating_allocator;

AMONGOC_EXTERN_C_END

#ifdef __cplusplus
namespace amongoc {

struct get_allocator_fn {
    constexpr auto operator()(const auto& arg) const noexcept
        requires requires { arg.query(*this); } and (not requires { arg.get_allocator(); })
    {
        return arg.query(*this);
    }

    constexpr auto operator()(const auto& arg) const noexcept
        requires requires { arg.get_allocator(); }
    {
        return arg.get_allocator();
    }
};

/**
 * @brief Obtain the allocator associated with an object, if available
 */
inline constexpr struct get_allocator_fn get_allocator {};

template <typename T>
concept has_allocator = requires(const T& obj) { get_allocator(obj); };

template <typename T>
using get_allocator_t = decltype(get_allocator(*(const T*)nullptr));

/**
 * @brief A C++-style allocator that adapts an `amongoc_allocator`
 *
 * @tparam T
 */
template <typename T = void>
class cxx_allocator {
public:
    using value_type = T;
    using pointer    = value_type*;

    // Construct around an existing amongoc_allocator object
    explicit cxx_allocator(amongoc_allocator a) noexcept
        : _alloc(a) {}

    // Convert-construct from an allocator of another type
    template <typename U>
    cxx_allocator(cxx_allocator<U> o) noexcept
        : _alloc(o.c_allocator()) {}

    // Obtain the C-style allocator managed by this object
    amongoc_allocator c_allocator() const noexcept { return _alloc; }

    // Allocate N objects
    pointer allocate(size_t n) const {
        const size_t max_count = SIZE_MAX / sizeof(T);
        if (n > max_count) {
            // Multiplying would overflow
            return nullptr;
        }
        pointer p = static_cast<pointer>(amongoc_allocate(_alloc, n * sizeof(T)));
        if (p == nullptr) {
            throw std::bad_alloc();
        }
        return p;
    }

    // Deallocate N object
    void deallocate(pointer p, size_t n) const {
        return amongoc_deallocate(_alloc, p, n * sizeof(T));
    }

    // Compare two allocators. They are equivalent if the underlying C allocators are equal
    bool operator==(cxx_allocator other) const noexcept {
        return _alloc.userdata == other.c_allocator().userdata
            and _alloc.reallocate == other.c_allocator().reallocate;
    }

    // Utility to dynamically allocate and construct a single object using this allocator
    template <typename... Args>
    pointer new_(Args&&... args) const {
        pointer p = this->allocate(1);
        return new (p) T(static_cast<Args&&>(args)...);
    }

    // Utility to destroy and deallocate a single object that was allocated with this allocator
    void delete_(pointer p) const noexcept {
        if (p) {
            p->~T();
            this->deallocate(p, 1);
        }
    }

    // Utility to rebind the value type of the allocator
    template <typename U>
    cxx_allocator<U> rebind() const noexcept {
        return *this;
    }

private:
    // The adapted C allocator
    amongoc_allocator _alloc;
};

// C++ wrapper around `::amongoc_terminating_allocator`, and allocator which immediately terminates
inline const cxx_allocator<> terminating_allocator
    = cxx_allocator<>{::amongoc_terminating_allocator};

}  // namespace amongoc
#endif
