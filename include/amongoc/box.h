#pragma once

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
#include <memory>
#include <type_traits>
#endif

#include "./alloc.h"
#include "./config.h"

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

typedef void (*amongoc_box_destructor)(void*) AMONGOC_NOEXCEPT;

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
    amongoc_allocator      alloc;
    amongoc_box_destructor destroy;
    size_t                 size;
    AMONGOC_ALIGNAS(max_align_t) char object[1];
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
};

#ifdef __cplusplus
namespace amongoc {

class unique_box;

/**
 * @brief Trait variable that tags types which are trivially-relocatable
 */
template <typename T>
constexpr bool enable_trivially_relocatable_v
    = (std::is_trivially_destructible_v<T> and std::is_trivially_move_constructible_v<T>)
    or requires { requires static_cast<bool>(T::enable_trivially_relocatable); };

template <typename T, typename D>
constexpr bool enable_trivially_relocatable_v<std::unique_ptr<T, D>>
    = enable_trivially_relocatable_v<D>;

template <typename T>
constexpr bool enable_trivially_relocatable_v<std::shared_ptr<T>> = true;

/**
 * @brief Match types which can be stored inline within an amongoc_box.
 *
 * The type must meet the following properties:
 *
 * 1. Must qualify as trivially-relocatable. This is the default for any type that
 *    is trivially destructible and trivially move-constructible.
 * 2. Must be either:
 *    a. Trivially destructible AND no larger than AMONGOC_BOX_SMALL_SIZE
 *    b. OR no larger than AMONGOC_BOX_SMALL_SIZE_WITH_DTOR
 */
template <typename T>
concept box_inlinable_type = enable_trivially_relocatable_v<T>
    and ((std::is_trivially_destructible_v<T> and sizeof(T) <= AMONGOC_BOX_SMALL_SIZE)
         or (sizeof(T) <= AMONGOC_BOX_SMALL_SIZE_WITH_DTOR));

// Use within the public section of a class body to declare whether a type is trivially relocatable
#define AMONGOC_TRIVIALLY_RELOCATABLE_THIS(B) enum { enable_trivially_relocatable = B }

}  // namespace amongoc
#endif

typedef struct amongoc_view amongoc_view;
struct amongoc_view {
    struct _amongoc_box_storage _storage;

#ifdef __cplusplus
    template <typename T>
    T& as() const noexcept;
#endif
};

struct amongoc_box {
    union {
        amongoc_view                view;
        struct _amongoc_box_storage _storage;
    };

#ifdef __cplusplus
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
    T* prepare_storage(amongoc::cxx_allocator<>, amongoc_box_destructor dtor) noexcept;

    /**
     * @brief Convert the C-style `amongoc_box` to a C++ `unique_box` for lifetime safety
     */
    inline amongoc::unique_box as_unique() && noexcept;
#endif
};

AMONGOC_EXTERN_C_BEGIN
/**
 * @internal
 * @brief Initialize the given amongoc_box for storage of a type with the given size and destructor
 *
 * @param box The box to initialize
 * @param size The size of the object being stored.
 * @param dtor The destructor function to imbue upon the box
 * @return void* A pointer to the storage for the stored object.
 *
 * This function is intentionally static-inline, as any inliner, dead-code-eliminator, and constant
 * propagator worth its salt will replace these with trivial memset/calloc/memcpy calls.
 */
static inline void* _amongocBoxInitStorage(amongoc_box*           box,
                                           bool                   allow_inline,
                                           size_t                 size,
                                           amongoc_box_destructor dtor,
                                           amongoc_allocator      alloc) AMONGOC_NOEXCEPT {
    if (allow_inline && !dtor && size <= AMONGOC_BOX_SMALL_SIZE) {
        // Store as a trivial object with no destructor
        box->_storage.is_dynamic = 0;
        box->_storage.has_dtor   = 0;
        memset(&box->_storage.u.trivial_inline, 0, sizeof box->_storage.u.trivial_inline);
        return &box->_storage.u.trivial_inline;
    } else if (allow_inline && dtor && size <= AMONGOC_BOX_SMALL_SIZE_WITH_DTOR) {
        // Store as a non-trivial inline object with a destructor
        box->_storage.is_dynamic = 0;
        box->_storage.has_dtor   = 1;
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
            = (struct _amongoc_dynamic_box*)amongoc_allocate(alloc, alloc_size);
        memset(dyn, 0, alloc_size);
        dyn->alloc   = alloc;
        dyn->size    = alloc_size;
        dyn->destroy = dtor;
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
static inline void amongoc_box_free_storage(amongoc_box box) AMONGOC_NOEXCEPT {
    if (box._storage.is_dynamic) {
        amongoc_deallocate(box._storage.u.dynamic->alloc,
                           box._storage.u.dynamic,
                           box._storage.u.dynamic->size);
    }
}

/**
 * @internal
 * @brief Obtain a pointer to the value stored within a box
 */
static inline void* _amongocBoxDataPtr(struct _amongoc_box_storage* stor) AMONGOC_NOEXCEPT {
    if (stor->is_dynamic) {
        return stor->u.dynamic->object;
    } else {
        if (stor->has_dtor) {
            return stor->u.nontrivial_inline.bytes;
        } else {
            return stor->u.trivial_inline.bytes;
        }
    }
}

/**
 * @brief Destroy a value contained in an amongoc_box
 *
 * This function will both run the destructor and release any allocated storage
 *
 * @param box The box to destroy
 */
static inline void amongoc_box_destroy(amongoc_box box) AMONGOC_NOEXCEPT {
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

AMONGOC_EXTERN_C_END

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
    _amongoc_paste(_amongoc_box_init_impl_argc_,                                                   \
                   _amongocArgCount(~, ~, ~__VA_OPT__(, ) __VA_ARGS__))(Box,                       \
                                                                        T,                         \
                                                                        AllowInline __VA_OPT__(, ) \
                                                                            __VA_ARGS__)

#define _amongoc_box_init_impl_argc_3(Box, T, AllowInline)                                         \
    *(T*)_amongocBoxInitStorage(&(Box), AllowInline, sizeof(T), NULL, amongoc_default_allocator)

#define _amongoc_box_init_impl_argc_4(Box, T, AllowInline, Dtor)                                   \
    *(T*)_amongocBoxInitStorage(&(Box), AllowInline, sizeof(T), Dtor, amongoc_default_allocator)

#define _amongoc_box_init_impl_argc_5(Box, T, AllowInline, Dtor, Alloc)                            \
    *(T*)_amongocBoxInitStorage(&(Box), AllowInline, sizeof(T), Dtor, Alloc)

/**
 * @brief Cast an amongoc_box expression to a contained type T
 *
 * Syntax:
 *
 * ```
 * amongoc_box_cast(T)(some_box)
 * ```
 *
 * Expands to an l-value expression of type T
 */
#define amongoc_box_cast(T) (*(T*) _amongocBoxCastOpen
#define _amongocBoxCastOpen(Box) (_amongocBoxDataPtr(&(Box)._storage)))

/**
 * @brief A special amongoc_box value that represents no value. It is safe to
 * discard this value.
 */
#define amongoc_nil (AMONGOC_INIT(amongoc_box){})

static inline void _amongoc_box_take_impl(void* dst, size_t sz, amongoc_box* box) AMONGOC_NOEXCEPT {
    memcpy(dst, _amongocBoxDataPtr(&box->_storage), sz);
    amongoc_box_free_storage(*box);
    *box = amongoc_nil;
}

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
#define amongoc_box_take(Dest, Box) (_amongoc_box_take_impl(&(Dest), (sizeof(Dest)), &(Box)))

/**
 * @brief Declare an amongoc_box destructor function that invokes an underlying destructor function
 * on the value owned by the amongoc_box
 */
#define amongoc_box_declare_dtor_shim(FuncName, Type, RealName)                                    \
    static inline void FuncName(amongoc_box a) { RealName(amongoc_box_cast(Type)(a)); }            \
    static_assert(1, "");

#define DECLARE_BOX_EZ(Name, Type)                                                                 \
    static inline amongoc_box amongoc_box_##Name(Type val) AMONGOC_NOEXCEPT {                      \
        amongoc_box b;                                                                             \
        amongoc_box_init(b, Type) = val;                                                           \
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

#ifdef __cplusplus

namespace amongoc {

using box = amongoc_box;

class unique_box {
public:
    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true);
    // Move-construct from a box
    explicit unique_box(amongoc_box&& b) noexcept
        : _box(b) {
        b = amongoc_nil;
    }

    unique_box(unique_box&& other) noexcept
        : _box(other._box) {
        other._box = amongoc_nil;
    }

    unique_box& operator=(unique_box&& other) noexcept {
        amongoc_box_destroy(_box);
        _box       = other._box;
        other._box = amongoc_nil;
        return *this;
    }

    ~unique_box() { amongoc_box_destroy(_box); }

    template <typename T>
    T& as() noexcept {
        return amongoc_box_cast(T)(_box);
    }

    operator amongoc_view() const noexcept { return _box.view; }

    /**
     * @brief Release ownership of the box, returning the C-style `amongoc_box`
     */
    [[nodiscard]] amongoc_box release() noexcept {
        box r = _box;
        _box  = amongoc_nil;
        return r;
    }

    template <typename T>
    T take() noexcept {
        T ret = (T&&)as<T>();
        amongoc_box_free_storage(release());
        return ret;
    }

    /**
     * @brief Construct a new box value by decay-copying the given value
     *
     * @param value The value to be decay-copied into a new box
     * @return amongoc_box The newly constructed box that contains the value
     */
    template <typename T>
    static unique_box from(cxx_allocator<> alloc, T&& value) {
        return make<std::decay_t<T>>(alloc, AM_FWD(value));
    }

    // Prevent users from accidentally box-ing a box
    static void from(cxx_allocator<>, box&)               = delete;
    static void from(cxx_allocator<>, box&&)              = delete;
    static void from(cxx_allocator<>, box const&)         = delete;
    static void from(cxx_allocator<>, box const&&)        = delete;
    static void from(cxx_allocator<>, unique_box&)        = delete;
    static void from(cxx_allocator<>, unique_box&&)       = delete;
    static void from(cxx_allocator<>, unique_box const&)  = delete;
    static void from(cxx_allocator<>, unique_box const&&) = delete;

    /**
     * @brief Create a box that contains a `T` with an explicit destructor object type
     *
     * @tparam T The type to be boxed. MUST be trivially-destructible
     * @tparam D A stateless default-constructible destructor object
     * @param obj The object to be copied into the box
     */
    template <typename T, typename D>
    static unique_box from(cxx_allocator<> alloc, T obj, D) noexcept {
        static_assert(std::is_trivially_destructible_v<T>,
                      "Creating a box with an explicit destructor requires that the object be "
                      "trivially destructible itself");
        // The destructor function that will be imbued in the box
        auto dtor = [](void* p) noexcept -> void {
            D d{};
            d(*static_cast<T*>(p));
        };
        amongoc_box ret;
        // Make storage
        T* ptr = ret.prepare_storage<T>(alloc, dtor);
        // Placement-new the object
        new (ptr) T(AM_FWD(obj));
        return AM_FWD(ret).as_unique();
    }

    /**
     * @brief Create a new box that contains a `T`, constructed from the given
     * arguments.
     *
     * @tparam T The type to be constructed into a box
     * @param args The constructor arguments
     */
    template <typename T, typename... Args>
    static unique_box make(cxx_allocator<> alloc,
                           Args&&... args) noexcept(noexcept(T(AM_FWD(args)...))) {
        amongoc_box ret;
        T*          ptr;
        // Conditionally add a destructor to the box based on whether the type
        // actually has a destructor.
        if constexpr (std::is_trivially_destructible_v<T>) {
            // No destructor needed. This gives more space within the box for the small-object
            // optimization
            ptr = ret.prepare_storage<T>(alloc, nullptr);
        } else {
            // Add a destructor function that just invokes the destructor on the object
            ptr = ret.prepare_storage<T>(alloc, indirect_destroy<T>);
        }
        // Placement-new the object into storage
        if constexpr (amongoc::box_inlinable_type<T> or noexcept(T(AM_FWD(args)...))) {
            // No exception handling required: The constructor cannot throw OR we didn't allocate
            // any dynamic storage and there is nothing that would need to be freed
            new (ptr)(T)(AM_FWD(args)...);
        } else {
            // We will need to free the storage if the constructor throws
            try {
                new (ptr)(T)((Args&&)args...);
            } catch (...) {
                amongoc_box_free_storage(ret);
                throw;
            }
        }
        return AM_FWD(ret).as_unique();
    }

private:
    // The managed box
    amongoc_box _box;

    template <typename T>
    static void indirect_destroy(void* p) noexcept {
        static_cast<T*>(p)->~T();
    }
};

}  // namespace amongoc

template <typename T>
T& amongoc_view::as() const noexcept {
    return amongoc_box_cast(T)((amongoc_view&)*this);
}

template <typename T>
T* amongoc_box::prepare_storage(amongoc::cxx_allocator<> alloc,
                                amongoc_box_destructor   dtor) noexcept {
    if constexpr (amongoc::enable_trivially_relocatable_v<T>) {
        // The box can be safely stored inline, since moving the box will relocate the corresponding
        // object
        return &amongoc_box_init(*this, T, dtor, alloc.c_allocator());
    } else {
        // Box type cannot be safely stored inline, so dynamically allocate it:
        return &amongoc_box_init_noinline(*this, T, dtor, alloc.c_allocator());
    }
}

amongoc::unique_box amongoc_box::as_unique() && noexcept {
    return amongoc::unique_box((amongoc_box&&)(*this));
}

#endif
