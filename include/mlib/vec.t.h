/**
 * @file vec.t.h
 * @brief Declare a new vector container data type
 * @date 2024-10-02
 *
 * To use this file:
 *
 * - #define a type `T` immediately before including this file.
 * - Optional: Define a type `VecName` to the name of the vector.
 * - Optional: Define a `VecDestroyElement` macro to specify how the vector
 *   should destroy elements.
 *
 * Types stored in the vector must be trivially relocatable.
 */
#include <mlib/alloc.h>
#include <mlib/config.h>

#include <string.h>

#ifndef T
#error A type `T` should be defined before including this file
#define T int  // Define a type for IDE diagnostics
#endif

#ifndef VecName
#define VecName MLIB_PASTE(T, _vec)
#endif

#ifndef VecDestroyElement
#define VecDestroyElement(N) ((void)(N))
#endif

#define fn(M) MLIB_PASTE(VecName, MLIB_PASTE(_, M))

typedef struct VecName {
    T*             data;
    size_t         size;
    mlib_allocator allocator;

#if mlib_is_cxx()
    T* begin() const noexcept { return data; }
    T* end() const noexcept { return data + size; }
#endif
} VecName;

static inline T*     fn(begin)(VecName v) mlib_noexcept { return v.data; }
static inline T*     fn(end)(VecName v) mlib_noexcept { return v.data + v.size; }
static inline size_t fn(max_size)(void) mlib_noexcept { return SSIZE_MAX / sizeof(T); }

mlib_nodiscard("Check the returned pointer to detect allocation failure")  //
    static inline T* fn(resize(VecName* self, size_t count)) mlib_noexcept {
    if (count > fn(max_size())) {
        // We cannot allocate this many objects
        return NULL;
    }
    if (count < self->size) {
        // We need to destroy elements at the tail
        T* iter = self->data + count;
        for (; iter != fn(end)(*self); ++iter) {
            VecDestroyElement(*iter);
        }
    }
    T* pointer;
    if (count == 0) {
        pointer = NULL;
        if (self->size) {
            mlib_deallocate(self->allocator, self->data, self->size * sizeof(T));
        }
    } else if (count != self->size) {
        pointer = (T*)mlib_reallocate(self->allocator,
                                      self->data,
                                      count * sizeof(T),
                                      mlib_alignof(T),
                                      self->size * sizeof(T),
                                      NULL);
    } else {
        // We are resizing to the same size. Do nothing.
        pointer = self->data;
    }
    if (!pointer) {
        // We assume that shrinking cannot fail. Otherwise, we'll have destroyed
        // elements at the tail and now we have a vec that contains dead elements.
        return NULL;
    }
    // Update the data pointer
    self->data = pointer;

    T* ret_ptr;
    if (count > self->size) {
        // Zero-init the tail
        T* iter = self->data + self->size;
        memset(iter, 0, sizeof(T) * (count - self->size));
        // We grew the vector. Return the pointer to the first new element
        ret_ptr = self->data + self->size;
    } else {
        // We shrunk the vector. Return the pointr to the end
        ret_ptr = self->data + count;
    }
    // Update the stored size
    self->size = count;
    return ret_ptr;
}

/**
 * @brief Create a new empty vector
 */
static inline VecName fn(new(mlib_allocator alloc)) mlib_noexcept {
    VecName ret;
    ret.data      = NULL;
    ret.size      = 0;
    ret.allocator = alloc;
    return ret;
}

static inline void fn(delete(VecName v)) mlib_noexcept { (void)fn(resize(&v, 0)); }

/**
 * @brief Create a new vector with `n` zero-initialized elements
 */
static inline VecName fn(new_n(size_t n, mlib_allocator alloc)) mlib_noexcept {
    VecName ret = fn(new (alloc));
    (void)fn(resize)(&ret, n);
    return ret;
}

#undef T
#undef fn
#undef VecName
#undef VecDestroyElement
