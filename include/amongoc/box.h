#pragma once

#include <mlib/alloc.h>
#include <mlib/config.h>
#include <mlib/delete.h>

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if mlib_is_cxx()
#include <amongoc/relocation.hpp>

#include <mlib/unique.hpp>

#include <new>
#include <type_traits>
#endif

/**
 * @brief A generic type-erased relocatable value container.
 *
 * To store a new value, use 'amongoc_box_init'. To update/read an existing value,
 * use `amongoc_box_cast`. To release the stored value, use `amongoc_box_destroy`.
 *
 * An amongoc_box is "active for type T" if either:
 *
 * - It was given to an amongoc_box_init() with argument T
 * - OR: It is a copy of an amongoc_box that is active for type T
 *
 * AND:
 *
 * - AND: It has not been passed by-value to any function that "consumes" the box
 *   since it most recently became active for type T.
 *
 * @note About RELOCATION:
 *
 * The amongoc_box should be considered "trivially relocatable". That is, a
 * byte-wise copy of the object *can* be considered a moved-to amongoc_box.
 *
 * If the type stored in an amongoc_box is relocatable, then the amongoc_box is
 * itself relocatable. If any copies are taken of an amongoc_box, then exactly
 * one of the copies may be given to to a consuming function. Calling any consuming
 * functions more than once on independent amongoc_box values that were derived
 * from the same original amongoc_box_init call is undefined behavior.
 *
 * @note About SMALLNESS:
 *
 * amongoc_box considers some objects to be "small". If those objects are small,
 * then it is guaranteed that amongoc_box will not allocate memory for storing
 * those objects, and it is safe to omit calls to amongoc_box_destroy() ONLY if
 * the stored type is "small" AND the type has no associated destructor.
 *
 * The only types guaranteed to be considered "small" are any objects smaller
 * than the platform's pointer type.
 */
typedef struct amongoc_box amongoc_box;

typedef void (*amongoc_box_destructor)(void*);

/// The total size of an amongoc_box object
enum {
    AMONGOC_BOX_SMALL_SIZE           = sizeof(void*) * 3,
    AMONGOC_BOX_FN_PTR_SIZE          = sizeof(amongoc_box_destructor),
    AMONGOC_BOX_SMALL_SIZE_WITH_DTOR = AMONGOC_BOX_SMALL_SIZE - AMONGOC_BOX_FN_PTR_SIZE,
};

typedef struct amongoc_view amongoc_view;

struct _amongoc_trivial_inline {
    char bytes[AMONGOC_BOX_SMALL_SIZE];
};
struct _amongoc_nontrivial_inline {
    char bytes[AMONGOC_BOX_SMALL_SIZE_WITH_DTOR];
    char dtor_bytes[AMONGOC_BOX_FN_PTR_SIZE];
};

struct _amongoc_dynamic_box {
    mlib_allocator         alloc;
    amongoc_box_destructor destroy;
    size_t                 alloc_size;
    mlib_alignas(max_align_t) char object[1];
};

union _amongoc_box_union {
    struct _amongoc_trivial_inline    trivial_inline;
    struct _amongoc_nontrivial_inline nontrivial_inline;
    struct _amongoc_dynamic_box*      dynamic;
};

struct _amongoc_box_storage {
    alignas(max_align_t) union _amongoc_box_union u;
    // Non-zero if the box storage is dynamically allocated
    unsigned char is_dynamic : 1;
    // Non-zero if the box has an associated destructor function
    unsigned char has_dtor : 1;
    unsigned char inline_size : 6;
};

MLIB_IF_CXX(namespace amongoc { struct unique_box; })

typedef struct amongoc_view amongoc_view;
struct amongoc_view {
    struct _amongoc_box_storage _storage;

#if mlib_is_cxx()
    template <typename T>
    T& as() const noexcept;
#endif
};

struct amongoc_box {
    union {
        amongoc_view                view;
        struct _amongoc_box_storage _storage;
    };

#if mlib_is_cxx()
    /**
     * @brief Low-level: Prepare storage for an object of type `T` with the given
     * destructor function
     *
     * @tparam T The type for which storage will be prepared
     * @param dtor A destructor function to imbue in the box
     * @return T* A pointer to the reserved storage.
     *
     * @note This does not construct the object: It only zero-initializes enough
     * storage for that object.
     *
     * This function understands the trivial-relocatability of `T` and acts appropriately
     * when allocating the storage
     */
    template <typename T>
    T* prepare_storage(mlib::allocator<>, amongoc_box_destructor dtor);

    /**
     * @brief Convert the C-style `amongoc_box` to a C++ `unique_box` for lifetime safety
     */
    inline amongoc::unique_box as_unique() && noexcept;
#endif
};

mlib_extern_c_begin();

/**
 * @internal
 * @brief Initialize the given amongoc_box for storage of a type with the given size and destructor
 *
 * @param box The box to initialize
 * @param size The size of the object being stored.
 * @param dtor The destructor function to imbue upon the box
 * @return void* A pointer to the storage for the stored object, or NULL if memory allocation failed
 *
 * This function is intentionally static-inline, as any inliner, dead-code-eliminator, and constant
 * propagator worth its salt will replace these with trivial memset/calloc/memcpy calls.
 */
inline void* _amongocBoxInitStorage(amongoc_box*           box,
                                    bool                   allow_inline,
                                    size_t                 size,
                                    amongoc_box_destructor dtor,
                                    mlib_allocator         alloc) mlib_noexcept {
    if (allow_inline && !dtor && size <= AMONGOC_BOX_SMALL_SIZE) {
        // Store as a trivial object with no destructor
        box->_storage.is_dynamic  = 0;
        box->_storage.has_dtor    = 0;
        box->_storage.inline_size = (uint8_t)(size & 0x3f);
        memset(&box->_storage.u.trivial_inline, 0, sizeof box->_storage.u.trivial_inline);
        return &box->_storage.u.trivial_inline;
    } else if (allow_inline && dtor && size <= AMONGOC_BOX_SMALL_SIZE_WITH_DTOR) {
        // Store as a non-trivial inline object with a destructor
        box->_storage.is_dynamic  = 0;
        box->_storage.has_dtor    = 1;
        box->_storage.inline_size = (uint8_t)(size & 0x3f);
        memset(&box->_storage.u.nontrivial_inline.bytes,
               0,
               sizeof box->_storage.u.nontrivial_inline.bytes);
        memcpy(&box->_storage.u.nontrivial_inline.dtor_bytes, &dtor, sizeof dtor);
        return &box->_storage.u.nontrivial_inline.bytes;
    } else {
        // Value is too large for a small-object optimization, or inlining has been disabled
        box->_storage.is_dynamic                = 1;
        box->_storage.has_dtor                  = dtor != NULL;
        size_t                       alloc_size = sizeof(struct _amongoc_dynamic_box) + size;
        struct _amongoc_dynamic_box* dyn        = box->_storage.u.dynamic
            = (struct _amongoc_dynamic_box*)mlib_allocate(alloc, alloc_size);
        if (dyn == NULL) {
            // Allocation failed
            return NULL;
        }
        memset(dyn, 0, alloc_size);
        dyn->alloc      = alloc;
        dyn->alloc_size = alloc_size;
        dyn->destroy    = dtor;
        return dyn->object;
    }
}

/**
 * @brief Release any dynamic storage required by the given box without running
 * any associated destructor functions.
 *
 * @note This function is only necessary if the object contained by the box has
 * already been destroyed or moved-from, or is not yet properly initialized.
 * Use `amongoc_box_destroy` to destroy a box that is no longer in use.
 *
 * @param box The box to be freed
 */
inline void amongoc_box_free_storage(amongoc_box box) mlib_noexcept {
    if (box._storage.is_dynamic) {
        mlib_deallocate(box._storage.u.dynamic->alloc,
                        box._storage.u.dynamic,
                        box._storage.u.dynamic->alloc_size);
    }
}

/**
 * @internal
 * @brief Obtain a pointer to the value stored within a box
 */
inline void* _amongocBoxDataPtr(struct _amongoc_box_storage* stor) mlib_noexcept {
    mlib_diagnostic_push();
    mlib_gnu_warning_disable("-Warray-bounds");
    if (stor->is_dynamic) {
        return stor->u.dynamic->object;
    } else {
        if (stor->has_dtor) {
            return stor->u.nontrivial_inline.bytes;
        } else {
            return stor->u.trivial_inline.bytes;
        }
    }
    mlib_diagnostic_pop();
}

/**
 * @brief Destroy a value contained in an amongoc_box
 *
 * This function will both run the destructor and release any allocated storage
 *
 * @param box The box to destroy
 */
inline void amongoc_box_destroy(amongoc_box box) mlib_noexcept {
    if (box._storage.has_dtor) {
        // Box has a destructor function
        if (box._storage.is_dynamic) {
            // Box value is stored in dynamic storage
            box._storage.u.dynamic->destroy(_amongocBoxDataPtr(&(box)._storage));
        } else {
            // Box value is stored inline.
            amongoc_box_destructor dtor;
            // bit-cast the destructor from the inline storage
            memcpy(&dtor, box._storage.u.nontrivial_inline.dtor_bytes, sizeof dtor);
            dtor(_amongocBoxDataPtr(&(box)._storage));
        }
    }
    amongoc_box_free_storage(box);
}

mlib_assoc_deleter(amongoc_box, amongoc_box_destroy);

mlib_extern_c_end();

/**
 * @brief Initialize an amongoc_box to contain storage for a new T.
 *
 * The given type must be zero-initializable! The default value is zero-initialized.
 *
 * Expands to an l-value expression of type T.
 *
 * @param Box an l-value expression of type amongoc_box
 * @param T A complete type specifier
 * @param Destroy (optional) If given, specify the destructor function to be used
 * when invoking amongoc_box_destroy on the resulting box value.
 */
#define amongoc_box_init(Box, T, ...) _amongoc_box_init_impl(Box, T, true, __VA_ARGS__)

/**
 * @brief Initialize an amongoc_box to contain storage for a new T, never using
 * inline storage.
 *
 * This is like amongoc_box_init() but will force-disable the small-object optimization.
 * Use this for types that are not trivially relocatable.
 *
 * @param Box an l-value of type amongoc_box
 * @param T A complete type specifier
 * @param Destroy (optional) If given, the destructor function that will be used
 * to destroy the contained value in amongoc_box_destroy
 */
#define amongoc_box_init_noinline(Box, T, ...) _amongoc_box_init_impl(Box, T, false, __VA_ARGS__)

#define _amongoc_box_init_impl(Box, T, AllowInline, ...)                                           \
    MLIB_PASTE(_amongoc_box_init_impl_argc_, MLIB_ARG_COUNT(~, ~, ~__VA_OPT__(, ) __VA_ARGS__))    \
    (Box, T, AllowInline __VA_OPT__(, ) __VA_ARGS__)

#define _amongoc_box_init_impl_argc_3(Box, T, AllowInline)                                         \
    (T*)_amongocBoxInitStorage(&(Box), AllowInline, sizeof(T), NULL, mlib_default_allocator)

#define _amongoc_box_init_impl_argc_4(Box, T, AllowInline, Dtor)                                   \
    (T*)_amongocBoxInitStorage(&(Box), AllowInline, sizeof(T), Dtor, mlib_default_allocator)

#define _amongoc_box_init_impl_argc_5(Box, T, AllowInline, Dtor, Alloc)                            \
    (T*)_amongocBoxInitStorage(&(Box), AllowInline, sizeof(T), Dtor, Alloc)

/**
 * @brief Obtain a pointer to the stored data within an amongoc_box or amongoc_view
 */
#define amongoc_box_data(Box) _amongocBoxData((Box))
#define _amongocBoxData(B)                                                                         \
    mlib_generic(_amongocBoxDataCxx,                                                               \
                 _amongocBoxConstDataPtr,                                                          \
                 &(B),                                                                             \
                 const amongoc_box* : _amongocBoxConstDataPtr,                                     \
                 amongoc_box* : _amongocBoxMutDataPtr,                                             \
                 amongoc_view* : _amongocViewDataPtr)(&(B))

inline const void* _amongocBoxConstDataPtr(const amongoc_box* b) mlib_noexcept {
    return _amongocBoxDataPtr((struct _amongoc_box_storage*)&b->_storage);
}

inline const void* _amongocViewDataPtr(const amongoc_view* b) mlib_noexcept {
    return _amongocBoxDataPtr((struct _amongoc_box_storage*)&b->_storage);
}

inline void* _amongocBoxMutDataPtr(amongoc_box* b) mlib_noexcept {
    return _amongocBoxDataPtr(&b->_storage);
}

#if mlib_is_cxx()
inline void* _amongocBoxDataCxx(amongoc_box* b) noexcept {
    return _amongocBoxDataPtr(&b->_storage);
}
inline const void* _amongocBoxDataCxx(const amongoc_box* b) noexcept {
    return _amongocBoxDataPtr(const_cast<_amongoc_box_storage*>(&b->_storage));
}
inline const void* _amongocBoxDataCxx(const amongoc_view* b) noexcept {
    return _amongocBoxDataPtr(const_cast<_amongoc_box_storage*>(&b->_storage));
}
#endif  // C++

/**
 * @brief Cast an amongoc_box expression to a contained type T
 *
 * Syntax:
 *
 * ```
 * amongoc_box_cast(T, some_box)
 * ```
 *
 * Expands to an l-value expression of type T
 */
#define amongoc_box_cast(T, Box) mlib_parenthesized_expression(*(T*)(amongoc_box_data(Box)))

mlib_extern_c mlib_always_inline const amongoc_box* _amongocBoxGetNil() mlib_noexcept {
    static const amongoc_box box = {MLIB_IF_NOT_CXX(0)};
    return &box;
}

/**
 * @brief A special amongoc_box value that represents no value. It is safe to
 * discard this value.
 */
#define amongoc_nil mlib_parenthesized_expression(*_amongocBoxGetNil())

/**
 * @brief "Take" the object from a box, storing it in the destination target, and
 * relinquishing the box's ownership of that object.
 *
 * @param Dest An l-value expression of the type matching the contents of the box
 * @param Box The box to be moved-from.
 *
 * After this call, the box will contain `amongoc_nil`
 *
 * @note DO NOT use this for C++ types! Use `unique_box::take`
 */
#define amongoc_box_take(Dest, Box) _amongoc_box_take_impl(&(Dest), (sizeof(Dest)), &(Box))
inline void _amongoc_box_take_impl(void* dst, size_t sz, amongoc_box* box) mlib_noexcept {
    memcpy(dst, amongoc_box_data(*box), sz);
    amongoc_box_free_storage(*box);
    *box = amongoc_nil;
}

#define DECLARE_BOX_EZ(Name, Type)                                                                 \
    static inline amongoc_box amongoc_box_##Name(Type val) mlib_noexcept {                         \
        amongoc_box b;                                                                             \
        *amongoc_box_init(b, Type) = val;                                                          \
        return b;                                                                                  \
    }

DECLARE_BOX_EZ(pointer, const void*)
DECLARE_BOX_EZ(float, float)
DECLARE_BOX_EZ(double, double)
DECLARE_BOX_EZ(char, char)
DECLARE_BOX_EZ(short, short)
DECLARE_BOX_EZ(int, int)
DECLARE_BOX_EZ(unsigned, unsigned int)
DECLARE_BOX_EZ(long, long)
DECLARE_BOX_EZ(ulong, unsigned long)
DECLARE_BOX_EZ(longlong, long long)
DECLARE_BOX_EZ(ulonglong, unsigned long long)
DECLARE_BOX_EZ(ptrdiff, ptrdiff_t)
DECLARE_BOX_EZ(size, size_t)
DECLARE_BOX_EZ(int8, int8_t)
DECLARE_BOX_EZ(uint8, uint8_t)
DECLARE_BOX_EZ(int16, int16_t)
DECLARE_BOX_EZ(uint16, uint16_t)
DECLARE_BOX_EZ(int32, int32_t)
DECLARE_BOX_EZ(uint32, uint32_t)
DECLARE_BOX_EZ(int64, int64_t)
DECLARE_BOX_EZ(uint64, uint64_t)
#undef DECLARE_BOX_EZ
