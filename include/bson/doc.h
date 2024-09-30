#pragma once

#include <bson/view.h>

#include <mlib/alloc.h>
#include <mlib/config.h>

/**
 * @brief Assert the truth of the given expression. In checked mode, this is a
 * runtime assertion. In unchecked mode, this becomes an optimization hint only.
 */
#define BV_ASSERT(Cond)                                                                            \
    if (BSON_VIEW_CHECKED && !(Cond)) {                                                            \
        _bson_assert_fail(#Cond, __FILE__, __LINE__);                                              \
        abort();                                                                                   \
    } else if (!(Cond)) {                                                                          \
        __builtin_unreachable();                                                                   \
    } else                                                                                         \
        ((void)0)

/**
 * @brief A mutable BSON document.
 *
 * This type is trivially relocatable.
 *
 * @internal
 *
 * The sign bit of `_capacity_or_negative_offset_within_parent_data` is used as
 * a binary flag to control the interpretation of the data members, since the
 * sign bit would otherwise be unused.
 *
 * If _capacity_or_negative_offset_within_parent_data is less than zero, the
 * whole object is in CHILD MODE, otherwise it is in ROOT MODE
 */
typedef struct bson_doc {
    /**
     * @brief Points to the beginning of the document data.
     *
     * @internal
     *
     * In CHILD MODE, this is a non-owning pointer. It should not be freed or
     * reallocated.
     *
     * In ROOT MODE, this is an owning pointer and can be freed or reallocated.
     */
    bson_byte* _bson_document_data;
    /**
     * @brief The allocator used with this document
     */
    struct mlib_allocator _allocator;
    /**
     * @internal
     * @brief The capacity of the data buffer.
     *
     * In ROOT MODE, this refers to the number of bytes in `_bson_document` that
     * are available for writing before we must reallocate the region.
     */
    uint32_t _capacity;

#if mlib_is_cxx()
    friend constexpr ::bson_iterator begin(const bson_doc& m) noexcept { return bson_begin(m); }
    friend constexpr ::bson_iterator end(const bson_doc& m) noexcept { return bson_end(m); }
#endif  // C++
} bson_doc;

#define BSON_DOC_NULL                                                                              \
    mlib_init(bson_doc) { NULL, {NULL}, 0 }

mlib_extern_c_begin();

/**
 * @internal
 * @brief Write a 2's complement little-endian 32-bit integer into the given
 * memory location.
 *
 * @param bytes The destination to write into
 * @param v The value to write
 */
mlib_constexpr bson_byte* _bson_write_u32le(bson_byte* bytes, uint32_t u32) mlib_noexcept {
    if (mlib_is_consteval() || !mlib_is_little_endian()) {
        bytes[0].v = (u32 >> 0) & 0xff;
        bytes[1].v = (u32 >> 8) & 0xff;
        bytes[2].v = (u32 >> 16) & 0xff;
        bytes[3].v = (u32 >> 24) & 0xff;
    } else {
        memcpy(bytes, &u32, sizeof u32);
    }
    return bytes + 4;
}

/**
 * @internal
 * @brief Write a 2's complement little-endian 64-bit integer into the given
 * memory location.
 *
 * @param bytes The destination of the write
 * @param v  The value to write
 */
mlib_constexpr bson_byte* _bson_write_u64le(bson_byte* out, uint64_t u64) mlib_noexcept {
    if (mlib_is_consteval() || !mlib_is_little_endian()) {
        out = _bson_write_u32le(out, (uint32_t)(u64));
        out = _bson_write_u32le(out, (uint32_t)(u64 >> 32));
    } else {
        memcpy(out, &u64, sizeof u64);
        out += sizeof u64;
    }
    return out;
}

/**
 * @brief Perform a memcpy into the destination, returning that address
 * immediately following the copied data
 *
 * @param out The destination at which to write
 * @param src The source of the copy
 * @param len The number of bytes to copy
 * @return bson_byte* The value of (out + len)
 */
mlib_constexpr bson_byte*
_bson_memcpy(bson_byte* dst, const bson_byte* src, size_t len) mlib_noexcept {
    if (src && mlib_is_consteval()) {
        for (size_t n = 0; n < len; ++n) {
            dst[n] = src[n];
        }
    } else if (src && len) {
        memcpy(dst, src, len);
    }
    return dst + len;
}

mlib_constexpr bson_byte*
_bson_memmove(bson_byte* dst, const bson_byte* src, size_t len) mlib_noexcept {
    if (src && mlib_is_consteval()) {
        if (src < dst) {
            for (size_t n = len; n; --n) {
                dst[n - 1] = src[n - 1];
            }
        } else {
            for (size_t n = 0; n < len; ++n) {
                dst[n] = src[n];
            }
        }

    } else if (src && len) {
        memmove(dst, src, len);
    }
    return dst + len;
}

mlib_constexpr bson_byte* _bson_memset(bson_byte* dst, char v, size_t len) mlib_noexcept {
    if (mlib_is_consteval()) {
        for (size_t n = 0; n < len; ++n) {
            dst[n].v = (uint8_t)v;
        }
    } else {
        memset(dst, v, len);
    }
    return dst + len;
}

mlib_constexpr bson_byte*
_bson_memcpy_chr(bson_byte* dst, const char* src, size_t len) mlib_noexcept {
    if (src && mlib_is_consteval()) {
        for (size_t n = 0; n < len; ++n) {
            dst[n].v = (uint8_t)src[n];
        }
    } else if (src && len) {
        memcpy(dst, src, len);
    }
    return dst + len;
}

mlib_constexpr bson_byte*
_bson_memcpy_u8(bson_byte* dst, const uint8_t* src, size_t len) mlib_noexcept {
    if (src && mlib_is_consteval()) {
        for (uint32_t n = 0; n < len; ++n) {
            dst[n].v = src[n];
        }
    } else if (src && len) {
        memcpy(dst, src, len);
    }
    return dst + len;
}

mlib_constexpr uint32_t bson_doc_capacity(bson_doc d) mlib_noexcept { return d._capacity; }

/**
 * @brief Obtain the `mlib_allocator` used with the given BSON mutator object
 */
mlib_constexpr mlib_allocator bson_doc_get_allocator(bson_doc m) mlib_noexcept {
    return m._allocator;
}

mlib_constexpr bool _bson_doc_realloc(bson_doc* doc, uint32_t new_size) mlib_noexcept {
    // Cap allocation size:
    if (new_size > INT32_MAX) {
        return false;
    }
    // Get the allocator:
    mlib_allocator alloc = bson_doc_get_allocator(*doc);
    // Perform the reallocation:
    size_t     got_size = 0;
    bson_byte* newptr   = (bson_byte*)mlib_reallocate(alloc,
                                                    bson_mut_data(*doc),
                                                    new_size,
                                                    1,
                                                    bson_doc_capacity(*doc),
                                                    &got_size);
    if (!newptr) {
        // The allocatore reports failure
        return false;
    }
    // Assert that the allocator actually gave us the space we requested:
    BV_ASSERT(got_size >= new_size);
    BV_ASSERT(got_size <= INT32_MAX);
    // Save the new pointer
    doc->_bson_document_data = newptr;
    // Remember the buffer size that the allocator gave us:
    doc->_capacity = got_size;
    return true;
}

/**
 * @brief Adjust the capacity of the given bson_doc
 *
 * @param d The document to update
 * @param size The requested capacity
 * @return int32_t Returns the new capacity upon success, otherwise a negative
 * value
 *
 * @note If `size` is less than our current capacity, the capacity will remain
 * unchanged.
 *
 * All outstanding iterators and pointers are invalidated if the requested size
 * is greater than our current capacity.
 */
mlib_constexpr int32_t bson_reserve(bson_doc* d, uint32_t size) mlib_noexcept {
    if (bson_doc_capacity(*d) >= size) {
        return (int32_t)bson_doc_capacity(*d);
    }
    if (!_bson_doc_realloc(d, size)) {
        return -1;
    }
    return (int32_t)bson_doc_capacity(*d);
}

/**
 * @brief Create a new bson_doc with an allocator and reserved size
 *
 * @param allocator A custom allocator, or NULL for the default allocator
 * @param reserve The size to reserve within the new document
 * @return bson_doc A new mutator. Must be deleted with bson_mut_delete()
 */
mlib_constexpr bson_doc bson_new_ex(mlib_allocator allocator, uint32_t reserve) mlib_noexcept {
    // Create the object:
    bson_doc ret            = BSON_DOC_NULL;
    ret._capacity           = 0;
    ret._allocator          = allocator;
    ret._bson_document_data = NULL;
    if (reserve < 5) {
        // An empty document requires *at least* five bytes:
        reserve = 5;
    }
    // Create the initial storage:
    if (!bson_reserve(&ret, reserve)) {
        return ret;
    }
    // Set an initial empty document:
    _bson_memset(bson_mut_data(ret), 0, bson_doc_capacity(ret));
    ret._bson_document_data[0].v = 5;
    return ret;
}

/**
 * @brief Create a new empty document for later manipulation
 *
 * @note The return value must later be deleted using bson_doc_delete
 *
 * @note Not `constexpr` because the default allocator is not `constexpr`
 */
inline bson_doc bson_new(void) mlib_noexcept { return bson_new_ex(mlib_default_allocator, 512); }

mlib_constexpr bson_doc bson_copy_view(bson_view view, mlib_allocator alloc) mlib_noexcept {
    bson_doc ret = bson_new_ex(alloc, bson_size(view));
    _bson_memcpy(bson_mut_data(ret), bson_data(view), bson_size(view));
    return ret;
}

mlib_constexpr bson_doc bson_doc_copy(bson_doc other) mlib_noexcept {
    return bson_copy_view(bson_view_of(other), bson_doc_get_allocator(other));
}

/**
 * @brief Free the resources of the given BSON document
 */
mlib_constexpr void bson_delete(bson_doc d) mlib_noexcept {
    if (d._bson_document_data == NULL) {
        // Object is null
        return;
    }
    _bson_doc_realloc(&d, 0);
}

mlib_extern_c_end();

#if mlib_is_cxx()

namespace bson {

/**
 * @brief Presents a mutable BSON document STL-like interface
 *
 * This type provides insert/emplace/push_back/emplace_back APIs based on those
 * found on std::map.
 *
 * The type *does not* provide a `size()`, because such a method cannot be implemented
 * as a constant-time algorithm. To get the number of elements, use `std::ranges::distance`
 */
class document {
public:
    using allocator_type = mlib::allocator<bson_byte>;

    using iterator = bson_iterator;

#if !mlib_audit_allocator_passing()
    document()
        : document(allocator_type(mlib_default_allocator)) {}
    explicit document(bson_view v)
        : document(v, allocator_type(mlib_default_allocator)) {}
#endif

    ~document() { _del(); }

    /**
     * @brief Construct a document object using the given allocator
     */
    constexpr explicit document(allocator_type alloc)
        : document(alloc, 512) {}

    /**
     * @brief Construct a new document with the given allocator, and reserving
     * the given capacity in the new document.
     */
    constexpr explicit document(allocator_type alloc, std::size_t reserve_size) {
        _doc = bson_new_ex(alloc.c_allocator(), static_cast<std::uint32_t>(reserve_size));
        if (data() == nullptr) {
            throw std::bad_alloc();
        }
    }

    /**
     * @brief Take ownership of a C-style `::bson_doc` object
     */
    constexpr explicit document(bson_doc&& o) noexcept {
        _doc = o;
        o    = bson_doc{};
    }

    /**
     * @brief Copy from the given BSON view
     *
     * @param v The document to be copied
     * @param alloc An allocator for the operation
     */
    constexpr explicit document(bson_view v, allocator_type alloc)
        : _doc(bson_copy_view(v, alloc.c_allocator())) {
        if (data() == nullptr)
            throw std::bad_alloc();
    }

    constexpr document(document const& other) { _doc = bson_doc_copy(other._doc); }
    constexpr document(document&& other) noexcept
        : _doc(((document&&)other).release()) {}

    constexpr document& operator=(const document& other) {
        _del();
        _doc = bson_doc_copy(other._doc);
        if (data() == nullptr) {
            throw std::bad_alloc();
        }
        return *this;
    }

    constexpr document& operator=(document&& other) noexcept {
        _del();
        _doc = ((document&&)other).release();
        return *this;
    }

    constexpr iterator begin() const noexcept { return bson_begin(_doc); }
    constexpr iterator end() const noexcept { return bson_end(_doc); }

    constexpr iterator find(auto&& key) const noexcept
        requires requires(view v) { v.find(key); }
    {
        return view(*this).find(key);
    }

    constexpr bson_byte*       data() noexcept { return bson_mut_data(_doc); }
    constexpr const bson_byte* data() const noexcept { return bson_data(_doc); }
    constexpr std::size_t      byte_size() const noexcept { return bson_size(_doc); }

    constexpr bool empty() const noexcept { return byte_size() == 5; }

    constexpr void reserve(std::size_t n) {
        if (::bson_reserve(&_doc, static_cast<std::uint32_t>(n)) < 0) {
            throw std::bad_alloc();
        }
    }

    constexpr operator bson_view() const noexcept {
        return bson_view::from_data(data(), byte_size());
    }

    /**
     * @brief Prepare the internal buffer to be overwritten via invoking the given
     * operation
     *
     * @param len The number of bytes to reserve for the operation
     * @param oper The operation that will fill the document. Must be invocable
     * with a `bson_byte*` pointing to the beginning of the reserved buffer. If
     * the operation throws, then `*this` will be left in a valid-but-unspecified
     * state.
     */
    template <class Operation>
    constexpr void resize_and_overwrite(std::size_t len, Operation oper) {
        BV_ASSERT(len >= 5);
        reserve(len);
        try {
            oper(data());
        } catch (...) {
            _del();
            throw;
        }
    }

    // Obtain a reference to the wrapped C API struct
    constexpr bson_doc&       get() noexcept { return _doc; }
    constexpr const bson_doc& get() const noexcept { return _doc; }

    constexpr bson_doc release() && noexcept {
        auto m                   = _doc;
        _doc._bson_document_data = nullptr;
        return m;
    }

    constexpr allocator_type get_allocator() const noexcept {
        return allocator_type(::bson_doc_get_allocator(_doc));
    }

private:
    constexpr void _del() noexcept {
        if (_doc._bson_document_data) {
            // Moving from the object will set the data pointer to null, which prevents a
            // double-free
            ::bson_delete(_doc);
        }
        _doc._bson_document_data = nullptr;
    }
    ::bson_doc _doc;
};

}  // namespace bson
#endif  // C++

#undef BV_ASSERT
