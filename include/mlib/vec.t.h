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
 * - Optional: Define `VecInitElement(Ptr, Alloc, ...)` which initializes a new element.
 *   The first macro argument is a pointer to the element. the second is the allocator
 *   of the vector itself, and subsequent arguments are unspecified and reserved for
 *   future use. Elements are zero-initialized before being passed to this macro.
 *
 * Types stored in the vector must be trivially relocatable.
 */
#include <mlib/alloc.h>
#include <mlib/config.h>
#include <mlib/delete.h>

#include <stdbool.h>
#include <stddef.h>
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

#if MLIB_IS_EMPTY(MLIB_PASTE(VecDeclareExternInlines_, VecName))
#define vec_extern_inline extern inline
#else
#define vec_extern_inline inline
#endif

#define fn(M) MLIB_PASTE(VecName, MLIB_PASTE(_, M))

typedef struct VecName {
    T*             data;
    size_t         size;
    mlib_allocator allocator;

#if mlib_is_cxx()
    T* begin() noexcept { return data; }
    T* end() noexcept { return data + size; }
#endif
} VecName;

mlib_extern_c_begin();

vec_extern_inline T*     fn(begin)(VecName v) mlib_noexcept { return v.data; }
vec_extern_inline T*     fn(end)(VecName v) mlib_noexcept { return v.data + v.size; }
vec_extern_inline size_t fn(max_size)(void) mlib_noexcept { return SSIZE_MAX / sizeof(T); }

mlib_nodiscard("Check the returned bool to detect allocation failure")  //
    vec_extern_inline bool fn(resize(VecName* const self, size_t const count)) mlib_noexcept {
    if (count > fn(max_size())) {
        // We cannot allocate this many objects
        return false;
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
        mlib_deallocate(self->allocator, self->data, self->size * sizeof(T));
        self->data = NULL;
        self->size = 0;
        return true;
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
        return false;
    }
    // Update the data pointer
    self->data = pointer;

    if (count > self->size) {
        // Zero-init the tail
        T* iter = self->data + self->size;
        memset(iter, 0, sizeof(T) * (count - self->size));
#ifdef VecInitElement
        // Initialize each element.
        // Pass a const-copy of the allocator to the initializer, in case
        // the initializer wishes to use it.
        const mlib_allocator _vec_allocator = self->allocator;
        (void)_vec_allocator;
        for (; iter != self->data + count; ++iter) {
            // XXX: There is no error handling here. Currently, no vector types
            // in the library can fail to initialize, so we're safe.
            T* const _new_vec_element_ptr = iter;
            (void)(VecInitElement(_new_vec_element_ptr, _vec_allocator, ~));
        }
#endif
    }
    // Update the stored size
    self->size = count;
    return true;
}

/**
 * @brief Append another element, returning a pointer to that element.
 */
mlib_nodiscard("Check the returned pointer for failure")
    vec_extern_inline T* fn(push(VecName* self)) mlib_noexcept {
    size_t count = self->size;
    if (!fn(resize(self, count + 1))) {
        // Failed to push another item
        return NULL;
    }
    return self->data + count;
}

/**
 * @brief Create a new empty vector
 */
vec_extern_inline VecName fn(new(mlib_allocator alloc)) mlib_noexcept {
    VecName ret;
    ret.data      = NULL;
    ret.size      = 0;
    ret.allocator = alloc;
    return ret;
}

vec_extern_inline void fn(delete(VecName v)) mlib_noexcept { (void)fn(resize(&v, 0)); }
mlib_assoc_deleter(VecName, fn(delete));

/**
 * @brief Create a new vector with `n` zero-initialized elements
 */
vec_extern_inline VecName fn(new_n(size_t n, bool* okay, mlib_allocator alloc)) mlib_noexcept {
    VecName ret = fn(new (alloc));
    *okay       = fn(resize)(&ret, n);
    return ret;
}

mlib_extern_c_end();

#undef T
#undef fn
#undef VecName
#undef VecDestroyElement
#undef vec_extern_inline
#ifdef VecInitElement
#undef VecInitElement
#endif
