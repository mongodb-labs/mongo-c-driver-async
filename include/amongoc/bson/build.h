#pragma once

#include "./view.h"

#include <mlib/alloc.h>
#include <mlib/config.h>
#include <mlib/integer.h>

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if mlib_is_cxx()
#include <concepts>
#include <new>
#include <string_view>
#endif

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

mlib_extern_c_begin();

/**
 * @internal
 * @brief Write a 2's complement little-endian 32-bit integer into the given
 * memory location.
 *
 * @param bytes The destination to write into
 * @param v The value to write
 */
mlib_constexpr bson_byte* _bson_write_int32le(bson_byte* bytes, int32_t i32) mlib_noexcept {
    if (mlib_is_consteval() || !mlib_is_little_endian()) {
        uint32_t v = (uint32_t)i32;
        bytes[0].v = (v >> 0) & 0xff;
        bytes[1].v = (v >> 8) & 0xff;
        bytes[2].v = (v >> 16) & 0xff;
        bytes[3].v = (v >> 24) & 0xff;
    } else {
        memcpy(bytes, &i32, sizeof i32);
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
mlib_constexpr bson_byte* _bson_write_int64le(bson_byte* out, int64_t i64) mlib_noexcept {
    if (mlib_is_consteval() || !mlib_is_little_endian()) {
        const uint64_t u64 = (uint64_t)i64;
        out                = _bson_write_int32le(out, (int32_t)u64);
        out                = _bson_write_int32le(out, (int32_t)(u64 >> 32));
    } else {
        memcpy(out, &i64, sizeof i64);
        out += sizeof i64;
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
_bson_memcpy(bson_byte* dst, const bson_byte* src, uint32_t len) mlib_noexcept {
    if (src && mlib_is_consteval()) {
        for (uint32_t n = 0; n < len; ++n) {
            dst[n] = src[n];
        }
    } else if (src && len) {
        memcpy(dst, src, len);
    }
    return dst + len;
}

mlib_constexpr bson_byte*
_bson_memmove(bson_byte* dst, const bson_byte* src, uint32_t len) mlib_noexcept {
    if (src && mlib_is_consteval()) {
        if (src < dst) {
            for (uint32_t n = len; n; --n) {
                dst[n - 1] = src[n - 1];
            }
        } else {
            for (uint32_t n = 0; n < len; ++n) {
                dst[n] = src[n];
            }
        }

    } else if (src && len) {
        memmove(dst, src, len);
    }
    return dst + len;
}

mlib_constexpr bson_byte* _bson_memset(bson_byte* dst, char v, uint32_t len) mlib_noexcept {
    if (mlib_is_consteval()) {
        for (uint32_t n = 0; n < len; ++n) {
            dst[n].v = v;
        }
    } else {
        memset(dst, v, len);
    }
    return dst + len;
}

mlib_constexpr bson_byte*
_bson_memcpy_chr(bson_byte* dst, const char* src, uint32_t len) mlib_noexcept {
    if (src && mlib_is_consteval()) {
        for (uint32_t n = 0; n < len; ++n) {
            dst[n].v = src[n];
        }
    } else if (src && len) {
        memcpy(dst, src, len);
    }
    return dst + len;
}

mlib_constexpr bson_byte*
_bson_memcpy_u8(bson_byte* dst, const uint8_t* src, uint32_t len) mlib_noexcept {
    if (src && mlib_is_consteval()) {
        for (uint32_t n = 0; n < len; ++n) {
            dst[n].v = src[n];
        }
    } else if (src && len) {
        memcpy(dst, src, len);
    }
    return dst + len;
}

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
typedef struct bson_mut {
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
    union {
        /**
         * @internal
         * @brief In CHILD MODE, this points to the bson_mut that manages the parent document
         */
        struct bson_mut* _parent_mut;
        /**
         * @internal
         * @brief In ROOT MODE, this is the allocator for the document data.
         */
        struct mlib_allocator _allocator;
    };
    // void* _parent_mut_or_allocator_context;
    /**
     * @brief The capacity of the data buffer, or the offset of the element
     * within its parent.
     *
     * @internal
     *
     * If non-negative, we are in ROOT MODE. If negative, we are in CHILD MODE.
     *
     * In ROOT MODE, this refers to the number of bytes available in
     * `_bosn_document_data` that are available for writing before we must
     * reallocate the region.
     *
     * In CHILD MODE, this is the negative value of the byte-offset of the
     * document /element/ within the parent document. This is used to quickly
     * recover a bson_iterator that refers to the document element within the
     * parent, since iterators point to the element data. This is also necessary
     * to quickly compute the element key length without a call to strlen(),
     * since we can compute the key length based on the different between the
     * element's address and the address of the document data within the element.
     */
    int32_t _capacity_or_negative_offset_within_parent_data;

#if mlib_is_cxx()
    friend constexpr ::bson_iterator begin(const bson_mut& m) noexcept { return bson_begin(m); }
    friend constexpr ::bson_iterator end(const bson_mut& m) noexcept { return bson_end(m); }
#endif  // C++
} bson_mut;

/**
 * @brief Compute the number of bytes available before we will require
 * reallocating
 *
 * @param d The document to inspect
 * @return uint32_t The capacity of the document `d`
 *
 * @note If `d` is a child document, the return value reflects the number of
 * bytes for the maximum size of `d` until which we will require reallocating
 * the parent, transitively.
 */
mlib_constexpr uint32_t bson_capacity(bson_mut d) mlib_noexcept {
    if (d._capacity_or_negative_offset_within_parent_data < 0) {
        // We are a subdocument, so we need to calculate the capacity of our
        // document in the context of the parent's capacity.
        bson_mut parent = *d._parent_mut;
        // Number of bytes between the parent start and this element start:
        const mlib_integer bytes_before
            = mlibMath(neg(I(d._capacity_or_negative_offset_within_parent_data)));
        // Number of bytes from the element start until the end of the parent:
        const mlib_integer bytes_until_parent_end
            = mlibMath(sub(I(bson_ssize(parent)), V(bytes_before)));
        // Num bytes in the parent after this document element:
        const mlib_integer bytes_after = mlibMath(sub(V(bytes_until_parent_end), I(bson_ssize(d))));
        // Number of bytes in the parent that are not our own:
        const mlib_integer bytes_other = mlibMath(add(V(bytes_before), V(bytes_after)));
        // The number of bytes we can grow to until we will break the capacity of
        // the parent:
        const mlib_integer bytes_remaining
            = mlibMath(checkNonNegativeInt32(sub(I(bson_capacity(parent)), V(bytes_other))));
        mlibMath(assertNot(mlib_integer_allbits, V(bytes_remaining)));
        return (uint32_t)bytes_remaining.i64;
    }
    return (uint32_t)d._capacity_or_negative_offset_within_parent_data;
}

/**
 * @internal
 * @brief Reallocate the data buffer of a bson_mut
 *
 * @param m The mut object that is being resized
 * @param new_size The desired size of the data buffer
 * @retval true Upon success
 * @retval false Otherwise
 *
 * All iterators and pointers to the underlying data are invalidated
 */
mlib_constexpr bool _bson_mut_realloc(bson_mut* m, uint32_t new_size) mlib_noexcept {
    // We are only called on ROOT MODE bson_muts
    BV_ASSERT(m->_capacity_or_negative_offset_within_parent_data >= 0);
    // Cap allocation size:
    if (new_size > INT32_MAX) {
        return false;
    }
    // Get the allocator:
    mlib_allocator alloc = m->_allocator;
    // Perform the reallocation:
    size_t     got_size = 0;
    bson_byte* newptr
        = (bson_byte*)mlib_reallocate(alloc,
                                      m->_bson_document_data,
                                      new_size,
                                      1,
                                      (uint32_t)m->_capacity_or_negative_offset_within_parent_data,
                                      &got_size);
    if (!newptr) {
        // The allocatore reports failure
        return false;
    }
    // Assert that the allocator actually gave us the space we requested:
    BV_ASSERT(got_size >= new_size);
    BV_ASSERT(got_size <= INT32_MAX);
    // Save the new pointer
    m->_bson_document_data = newptr;
    // Remember the buffer size that the allocator gave us:
    m->_capacity_or_negative_offset_within_parent_data = (int32_t)got_size;
    return true;
}

/**
 * @brief Adjust the capacity of the given bson_mut
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
mlib_constexpr int32_t bson_reserve(bson_mut* d, uint32_t size) mlib_noexcept {
    BV_ASSERT(d->_capacity_or_negative_offset_within_parent_data >= 0
              && "Called bson_reserve() on child bson_mut");
    if (bson_capacity(*d) >= size) {
        return (int32_t)bson_capacity(*d);
    }
    if (!_bson_mut_realloc(d, size)) {
        return -1;
    }
    return d->_capacity_or_negative_offset_within_parent_data;
}

/**
 * @brief Create a new bson_mut with an allocator and reserved size
 *
 * @param allocator A custom allocator, or NULL for the default allocator
 * @param reserve The size to reserve within the new document
 * @return bson_mut A new mutator. Must be deleted with bson_mut_delete()
 */
mlib_constexpr bson_mut bson_mut_new_ex(mlib_allocator allocator, uint32_t reserve) mlib_noexcept {
    // Create the object:
    bson_mut r                                        = {0};
    r._capacity_or_negative_offset_within_parent_data = 0;
    r._allocator                                      = allocator;
    if (reserve < 5) {
        // An empty document requires *at least* five bytes:
        reserve = 5;
    }
    // Create the initial storage:
    if (!bson_reserve(&r, reserve)) {
        return r;
    }
    // Set an initial empty document:
    BV_ASSERT(r._capacity_or_negative_offset_within_parent_data >= 0);
    _bson_memset(r._bson_document_data,
                 0,
                 (uint32_t)r._capacity_or_negative_offset_within_parent_data);
    r._bson_document_data[0].v = 5;
    return r;
}

/**
 * @brief Create a new empty document for later manipulation
 *
 * @note The return value must later be deleted using bson_mut_delete
 *
 * @note Not `constexpr` because the default allocator is not `constexpr`
 */
inline bson_mut bson_mut_new(void) mlib_noexcept {
    return bson_mut_new_ex(mlib_default_allocator, 512);
}

/**
 * @brief Obtain the `mlib_allocator` used with the given BSON mutator object
 */
mlib_constexpr mlib_allocator bson_mut_get_allocator(bson_mut m) mlib_noexcept {
    if (m._capacity_or_negative_offset_within_parent_data < 0) {
        // We are a child document
        return bson_mut_get_allocator(*m._parent_mut);
    }
    return m._allocator;
}

mlib_constexpr bson_mut bson_mut_copy(bson_mut other) mlib_noexcept {
    bson_mut ret = bson_mut_new_ex(bson_mut_get_allocator(other), bson_size(other));
    _bson_memcpy(ret._bson_document_data, bson_data(other), bson_size(other));
    return ret;
}

/**
 * @brief Free the resources of the given BSON document
 */
mlib_constexpr void bson_mut_delete(bson_mut d) mlib_noexcept {
    if (d._bson_document_data == NULL) {
        // Object is null
        return;
    }
    _bson_mut_realloc(&d, 0);
}

/**
 * @internal
 * @brief Obtain a pointer to the element data at the given iterator position
 *
 * @param doc The document that owns the element
 * @param pos An element iterator
 */
mlib_constexpr bson_byte* _bson_mut_data_at(bson_mut doc, bson_iterator pos) mlib_noexcept {
    const ssize_t off = bson_iterator_data(pos) - bson_data(doc);
    return bson_mut_data(doc) + off;
}

/**
 * @internal
 * @brief Delete and/or insert a region of bytes within document data.
 *
 * This function will resize a region of memory within a document, and update
 * the document's size header to reflect the changed size. If this is a child
 * document, all parent documents' headers will also be updated.
 *
 * @param doc The document to update
 * @param position The position at which we will delete/add bytes
 * @param n_delete The number of bytes to "delete" at `position`
 * @param n_insert The number of bytes to "insert" at `position`
 * @param insert_from Pointer to a different memory region that willbe inserted
 * into the new region. If NULL, a fill byte is used. This MUST NOT point into
 * the data of `doc` or any of its parents/siblings.
 * @return bson_byte* A pointer within the document that corresponds to the
 * beginning of the modified area.
 *
 * @note Any pointers or iterators into the document will be invalidated if
 * the splice results in growing the document beyond its capacity. Care must
 * be taken to restore iterators and pointers following this operation.
 */
mlib_constexpr bson_byte* _bson_splice_region(bson_mut* const        doc,
                                              bson_byte*             position,
                                              uint32_t               n_delete,
                                              uint32_t               n_insert,
                                              const bson_byte* const insert_from) mlib_noexcept {
    // The offset of the position. We use this to later recover a pointer upon
    // reallocation
    const ssize_t pos_offset = position - bson_data(*doc);
    //  Check that we aren't splicing within our document header:
    BV_ASSERT(pos_offset >= 4);
    // Check that we aren't splicing at/beyond the doc's null terminator:
    BV_ASSERT(pos_offset < bson_ssize(*doc));

    // Calculate the new document size:
    mlib_math_try();
    const int32_t size_diff = mlibMathInt32(sub(I(n_insert), I(n_delete)));
    uint32_t      new_doc_size
        = (uint32_t)mlibMathNonNegativeInt32(add(I(bson_ssize(*doc)), I(size_diff)));
    mlib_math_catch (_) {
        return NULL;
    }

    if (doc->_capacity_or_negative_offset_within_parent_data < 0) {
        // We are in CHILD MODE, so we need to tell the parent to do the work:
        bson_mut* parent = doc->_parent_mut;
        // Our document offset within the parent, to adjust after reallocation:
        const ptrdiff_t my_doc_offset = bson_data(*doc) - bson_data(*parent);
        // Resize, and get the new position:
        position = _bson_splice_region(parent, position, n_delete, n_insert, insert_from);
        if (!position) {
            // Allocation failed
            return NULL;
        }
        // Adjust our document begin pointer, since we may have reallocated:
        doc->_bson_document_data = bson_mut_data(*parent) + my_doc_offset;
    } else {
        const bson_byte* const doc_data_end = bson_data(*doc) + bson_ssize(*doc);
        const uint32_t         avail_to_delete
            = (uint32_t)mlibMathNonNegativeInt32(I(doc_data_end - position));
        mlib_math_catch (_) {
            return NULL;
        }
        if (n_delete > avail_to_delete) {
            // Not enough data to actual delete, so this splice request is bogus?
            return NULL;
        }
        // We are the root of the document, so we do the actual work:
        if (new_doc_size > bson_capacity(*doc)) {
            // We need to grow larger. Add some extra to prevent repeated
            // allocations:
            const uint32_t new_capacity
                = (uint32_t)mlibMathNonNegativeInt32(add(I(new_doc_size), 1024));
            mlib_math_catch (_) {
                return NULL;
            }
            // Resize:
            if (bson_reserve(doc, (uint32_t)new_capacity) < 0) {
                // Allocation failed...
                return NULL;
            }
            // Recalc the splice position, since we reallocated
            position = bson_mut_data(*doc) + pos_offset;
        }
        // Data beginning:
        bson_byte* const doc_begin = bson_mut_data(*doc);
        // Data past-then-end:
        bson_byte* const doc_end = doc_begin + bson_size(*doc);
        // The beginning of the tail that needs to be shifted
        bson_byte* const move_dest = position + n_insert;
        // The destination of the shifted buffer tail:
        bson_byte* const move_from = position + n_delete;
        // The size of the tail that is being moved:
        const uint32_t data_remain = (uint32_t)mlibMathNonNegativeInt32(I(doc_end - move_from));
        mlib_math_catch (_) {
            return NULL;
        }
        // Move the data, and insert a marker in the empty space:
        _bson_memmove(move_dest, move_from, data_remain);
        if (insert_from) {
            _bson_memmove(position, insert_from, n_insert);
        } else {
            _bson_memset(position, 'X', n_insert);
        }
    }
    // Update our document header to match the new size:
    _bson_write_int32le(bson_mut_data(*doc), (int32_t)new_doc_size);
    return position;
}

/**
 * @internal
 * @brief Prepare a region within the document for a new element
 *
 * @param d The document to update
 * @param pos The iterator position at which the element will be inserted. This
 * will be updated to point to the inserted element data, or the end on failure
 * @param type The element type tag
 * @param key The element key string
 * @param datasize The amount of data we need to prepare for the element value
 * @return bson_byte* A pointer to the beginning of the element's value region
 * (after the key), or NULL in case of failure.
 *
 * @note In case of failure, returns NULL, and `pos` will be updated to refer to
 * the end position within the document, since that is never otherwise a valid
 * result of an insert.
 *
 * @note In case of success, `pos` will be updated to point to the newly
 * inserted element.
 */
mlib_constexpr bson_byte* _bson_prep_element_region(bson_mut* const      d,
                                                    bson_iterator* const pos,
                                                    const bson_type      type,
                                                    bson_utf8_view       key,
                                                    const uint32_t       datasize) mlib_noexcept {
    // Prevent embedded nulls within document keys:
    key = bson_utf8_view_chopnulls(key);
    // The total size of the element (add two: for the tag and the key's null):
    mlib_math_try();
    const uint32_t elem_size
        = (uint32_t)mlibMathNonNegativeInt32(add(U(key.len), add(2, U(datasize))));
    mlib_math_catch (_) {
        *pos = bson_end(*d);
        return NULL;
    }
    // The offset of the element within the doc:
    const ptrdiff_t pos_offset = bson_iterator_data(*pos) - bson_mut_data(*d);
    // Insert a new set of bytes for the data:
    bson_byte* outptr = _bson_splice_region(d, _bson_mut_data_at(*d, *pos), 0, elem_size, NULL);
    if (!outptr) {
        // Allocation failed:
        *pos = bson_end(*d);
        return NULL;
    }
    // Wriet the type tag:
    (*outptr++).v = (uint8_t)type;
    // Write the key
    outptr = _bson_memcpy_chr(outptr, key.data, key.len);
    // Write the null terminator:
    (*outptr++).v = 0;  // null terminator

    // Create a new iterator at the inserted data position
    pos->_ptr    = bson_mut_data(*d) + pos_offset;
    pos->_keylen = (int32_t)key.len;
    pos->_rlen   = (int32_t)(bson_ssize(*d) - pos_offset);
    return outptr;
}

/**
 * @internal
 * @brief Insert a string-like data type (UTF-8, code, or symbol)
 *
 * @param doc The document to update
 * @param pos The position at which to insert
 * @param key The new element key
 * @param realtype The bson type tag to insert
 * @param string The string value that will be inserted
 * @return bson_iterator Iterator to the inserted element, or the end iterator
 * on failure
 */
mlib_constexpr bson_iterator _bson_insert_stringlike(bson_mut*      doc,
                                                     bson_iterator  pos,
                                                     bson_utf8_view key,
                                                     bson_type      realtype,
                                                     bson_utf8_view string) mlib_noexcept {
    mlib_math_try();
    const int32_t string_size = mlibMathPositiveInt32(add(U(string.len), 1));
    const int32_t elem_size   = mlibMathPositiveInt32(add(I(string_size), 4));
    mlib_math_catch (_) {
        return bson_end(*doc);
    }
    bson_byte* out = _bson_prep_element_region(doc, &pos, realtype, key, (uint32_t)elem_size);
    if (out) {
        out    = _bson_write_int32le(out, string_size);
        out    = _bson_memcpy_chr(out, string.data, (uint32_t)string.len);
        out->v = 0;
    }
    return pos;
}

/**
 * @brief Insert a double value into the document
 *
 * @param doc The document to update
 * @param pos The insertion position
 * @param key The new element key
 * @param d The value to be inserted
 * @return bson_iterator An iterator for the inserted element, or the end
 * iterator
 */
mlib_constexpr bson_iterator bson_insert_double(bson_mut*      doc,
                                                bson_iterator  pos,
                                                bson_utf8_view key,
                                                double         d) mlib_noexcept {
    bson_byte* out = _bson_prep_element_region(doc, &pos, BSON_TYPE_DOUBLE, key, sizeof(double));
    if (out) {
        int64_t tmp;
        memcpy(&tmp, &d, sizeof d);
        _bson_write_int64le(out, tmp);
    }
    return pos;
}

/**
 * @brief Insert a UTF-8 element into a document
 *
 * @param doc The document to be updated
 * @param pos The insertion position
 * @param key The new element key
 * @param utf8 The string to be inserted
 * @return bson_iterator An iterator to the inserted element, or the end
 * iterator
 */
static mlib_constexpr bson_iterator bson_insert_utf8(bson_mut*      doc,
                                                     bson_iterator  pos,
                                                     bson_utf8_view key,
                                                     bson_utf8_view utf8) mlib_noexcept {
    return _bson_insert_stringlike(doc, pos, key, BSON_TYPE_UTF8, utf8);
}

/**
 * @brief Insert a new document element
 *
 * @param doc The document that is being updated
 * @param pos The position at which to insert
 * @param key The key for the new element
 * @param insert_doc A document to insert, or BSON_VIEW_NULL to create a new
 * empty document
 * @return bson_iterator An iterator to the inserted element, or the end
 * iterator
 *
 * @note To modify the child document, use @ref bson_mut_subdocument with the
 * returned iterator to obtain a bson_mut that modifies the subdocument.
 */
mlib_constexpr bson_iterator bson_insert_doc(bson_mut*      doc,
                                             bson_iterator  pos,
                                             bson_utf8_view key,
                                             bson_view      insert_doc) mlib_noexcept {
    if (!bson_data(insert_doc)) {
        // There was no document given. Re-call ourself with a view of an empty
        // doc:
        const bson_byte empty_doc[5] = {5};
        return bson_insert_doc(doc,
                               pos,
                               key,
                               bson_view_from_data(empty_doc, sizeof empty_doc, NULL));
    }
    // We have a document to insert:
    const uint32_t insert_size = bson_size(insert_doc);
    bson_byte*     out = _bson_prep_element_region(doc, &pos, BSON_TYPE_DOCUMENT, key, insert_size);
    if (out) {
        // Copy the document into place:
        _bson_memcpy(out, bson_data(insert_doc), bson_size(insert_doc));
    }
    return pos;
}

/**
 * @brief Insert a new array element
 *
 * @param doc The document that is being updated
 * @param pos The position at which to insert
 * @param key The key for the new element
 * @return bson_iterator An iterator to the inserted element, or the end
 * iterator
 *
 * @note To modify the child array, use @ref bson_mut_subdocument with the
 * returned iterator to obtain a bson_mut that modifies to the array document.
 *
 * @note When inserting/erasing within an array document, a standard array
 * document will have element keys that spell monotonically increasing decimal
 * integers starting from zero "0". It is up to the caller to use such keys for
 * array elements. Use @ref bson_tmp_uint_string() to easily create array keys.
 */
mlib_constexpr bson_iterator bson_insert_array(bson_mut*      doc,
                                               bson_iterator  pos,
                                               bson_utf8_view key) mlib_noexcept {
    bson_byte* out = _bson_prep_element_region(doc, &pos, BSON_TYPE_ARRAY, key, 5);
    if (out) {
        _bson_memset(out, 0, 5);
        out[0].v = 5;
    }
    return pos;
}

/**
 * @brief Obtain a mutator for the subdocument at the given position within
 * another document mutator
 *
 * @param parent A document being mutated.
 * @param subdoc_iter An iterator referring to a document/array element witihn
 * `parent`
 * @return bson_mut A new child mutator.
 *
 * @note The returned mutator MUST NOT be deleted.
 * @note The `parent` is passed by-pointer, because the child mutator may need
 * to update the parent in case of element insertion/deletion.
 */
mlib_constexpr bson_mut bson_mut_subdocument(bson_mut*     parent,
                                             bson_iterator subdoc_iter) mlib_noexcept {
    bson_mut ret = {0};
    if (bson_iterator_type(subdoc_iter) != BSON_TYPE_DOCUMENT
        && bson_iterator_type(subdoc_iter) != BSON_TYPE_ARRAY) {
        // The element is not a document, so return a null mutator.
        return ret;
    }
    // Remember the parent:
    ret._parent_mut = parent;
    // The offset of the element referred-to by the iterator
    const ptrdiff_t elem_offset = bson_iterator_data(subdoc_iter) - bson_data(*parent);
    // Point to the value data:
    ret._bson_document_data = bson_mut_data(*parent) + elem_offset + subdoc_iter._keylen + 2;
    // Save the element offset as a negative value. This triggers the mutator to
    // use CHILD MODE
    ret._capacity_or_negative_offset_within_parent_data = (int32_t)-elem_offset;
    return ret;
}

/**
 * @brief Given a bson_mut that is a child of another bson_mut, obtain an
 * iterator within the parent that refers to the child document.
 *
 * @param doc A child document mutator
 * @return bson_iterator An iterator within the parent that refers to the child
 */
mlib_constexpr bson_iterator bson_parent_iterator(bson_mut doc) mlib_noexcept {
    BV_ASSERT(doc._capacity_or_negative_offset_within_parent_data < 0);
    bson_mut      par = *doc._parent_mut;
    bson_iterator ret = {0};
    // Recover the address of the element:
    ret._ptr         = bson_mut_data(par) + -doc._capacity_or_negative_offset_within_parent_data;
    ptrdiff_t offset = ret._ptr - bson_mut_data(par);
    ret._keylen      = (int32_t)mlibMath(assertNot(mlib_integer_allbits,
                                              sub(I(doc._bson_document_data - ret._ptr), 2)))
                      .i64;
    ret._rlen = (int32_t)(bson_size(par) - offset);
    return ret;
}

/**
 * @brief Insert a binary object into the document
 *
 * @param doc The document being updated
 * @param pos The position at which to insert
 * @param key The new element's key
 * @param bin The binary data that will be inserted
 * @return bson_iterator An iterator to the inserted element, or the end
 * iterator
 */
mlib_constexpr bson_iterator bson_insert_binary(bson_mut*      doc,
                                                bson_iterator  pos,
                                                bson_utf8_view key,
                                                bson_binary    bin) mlib_noexcept {
    mlib_math_try();
    const int32_t bin_size  = mlibMathNonNegativeInt32(I(bin.data_len));
    const int32_t elem_size = mlibMathPositiveInt32(add(I(bin_size), 5));
    mlib_math_catch (_) {
        return bson_end(*doc);
    }
    bson_byte* out
        = _bson_prep_element_region(doc, &pos, BSON_TYPE_BINARY, key, (uint32_t)elem_size);
    if (out) {
        out      = _bson_write_int32le(out, bin_size);
        out[0].v = bin.subtype;
        ++out;
        out = _bson_memcpy(out, bin.data, (uint32_t)bin_size);
    }
    return pos;
}

/**
 * @brief Insert an "undefined" value into the document
 *
 * @param doc The document to be updated
 * @param pos The position at which to insert
 * @param key The new element's key
 * @return bson_iterator An iterator to the inserted element, or the end
 * iterator
 */
mlib_constexpr bson_iterator bson_insert_undefined(bson_mut*      doc,
                                                   bson_iterator  pos,
                                                   bson_utf8_view key) mlib_noexcept {
    _bson_prep_element_region(doc, &pos, BSON_TYPE_UNDEFINED, key, 0);
    return pos;
}

/**
 * @brief Insert an object ID into the document
 *
 * @param doc The document to be updated
 * @param pos The position at which to insert
 * @param key The new element's key
 * @param oid The object ID to insert
 * @return bson_iterator An iterator to the inserted element, or the end
 * iterator
 */
mlib_constexpr bson_iterator bson_insert_oid(bson_mut*      doc,
                                             bson_iterator  pos,
                                             bson_utf8_view key,
                                             bson_oid       oid) mlib_noexcept {
    bson_byte* out = _bson_prep_element_region(doc, &pos, BSON_TYPE_OID, key, sizeof oid);
    if (out) {
        memcpy(out, &oid, sizeof oid);
    }
    return pos;
}

/**
 * @brief Insert a boolean true/false value into the document
 *
 * @param doc The document being updated
 * @param pos The position at which to insert
 * @param key The new element's key
 * @param b The boolean value to insert
 * @return bson_iterator An iterator to the inserted element, or the end
 * iterator
 */
mlib_constexpr bson_iterator bson_insert_bool(bson_mut*      doc,
                                              bson_iterator  pos,
                                              bson_utf8_view key,
                                              bool           b) mlib_noexcept {
    bson_byte* out = _bson_prep_element_region(doc, &pos, BSON_TYPE_BOOL, key, 1);
    if (out) {
        out[0].v = b ? 1 : 0;
    }
    return pos;
}

/**
 * @brief Insert a datetime value into the document
 *
 * @param doc The document being updated
 * @param pos The position at which to insert
 * @param key The new element's key
 * @param dt The datetime to insert. This is encoded as a 64-bit integer count
 * of miliseconds since the Unix Epoch.
 * @return bson_iterator An iterator to the inserted element, or the end
 * iterator
 */
mlib_constexpr bson_iterator bson_insert_datetime(bson_mut*      doc,
                                                  bson_iterator  pos,
                                                  bson_utf8_view key,
                                                  int64_t        dt) mlib_noexcept {
    bson_byte* out = _bson_prep_element_region(doc, &pos, BSON_TYPE_DATE_TIME, key, sizeof dt);
    if (out) {
        _bson_write_int64le(out, dt);
    }
    return pos;
}

/**
 * @brief Insert a null into the document
 *
 * @param doc The document being updated
 * @param pos The position at which to insert
 * @param key The new element's key
 * @return bson_iterator An iterator to the inserted element, or the end
 * iterator
 */
mlib_constexpr bson_iterator bson_insert_null(bson_mut*      doc,
                                              bson_iterator  pos,
                                              bson_utf8_view key) mlib_noexcept {
    _bson_prep_element_region(doc, &pos, BSON_TYPE_NULL, key, 0);
    return pos;
}

/**
 * @brief Insert a regular expression value into the document
 *
 * @param doc The document being updated
 * @param pos The position at which to insert
 * @param key The new element's key
 * @param rx The regular expression to be inserted
 * @return bson_iterator An iterator to the inserted element, or the end
 * iterator
 */
mlib_constexpr bson_iterator bson_insert_regex(bson_mut*      doc,
                                               bson_iterator  pos,
                                               bson_utf8_view key,
                                               bson_regex     rx) mlib_noexcept {
    mlib_math_try();
    // Neither regex nor options may contain embedded nulls, so we cannot trust
    // the caller giving us the correct length:
    // Compute regex length:
    const int32_t rx_len = mlibMathNonNegativeInt32(strnlen(rx.regex, I(rx.regex_len)));
    // Compute options length:
    const int32_t opts_len
        = rx.options ? mlibMathNonNegativeInt32(strnlen32(rx.options, I(rx.options_len))) : 0;
    // The total value size:
    const int32_t size = mlibMathNonNegativeInt32(checkMin(2, add(I(rx_len), add(I(opts_len), 5))));
    mlib_math_catch (_) {
        return bson_end(*doc);
    }
    bson_byte* out = _bson_prep_element_region(doc, &pos, BSON_TYPE_REGEX, key, (uint32_t)size);
    if (out) {
        out        = _bson_memcpy_chr(out, rx.regex, (uint32_t)rx_len);
        (out++)->v = 0;
        if (rx.options) {
            out = _bson_memcpy_chr(out, rx.options, (uint32_t)opts_len);
        }
        (out++)->v = 0;
    }
    return pos;
}

/**
 * @brief Insert a DBPointer  into the document
 *
 * @param doc The document being updated
 * @param pos The position at which to insert
 * @param key The new element's key
 * @param dbp The DBPointer to be inserted
 * @return bson_iterator An iterator to the inserted element, or the end
 * iterator
 */
mlib_constexpr bson_iterator bson_insert_dbpointer(bson_mut*      doc,
                                                   bson_iterator  pos,
                                                   bson_utf8_view key,
                                                   bson_dbpointer dbp) mlib_noexcept {
    mlib_math_try();
    const int32_t collname_string_size
        = mlibMathPositiveInt32(add(strnlen(dbp.collection, I(dbp.collection_len)), 1));
    const int32_t el_size = mlibMathNonNegativeInt32(add(I(collname_string_size), I(12 + 4)));
    mlib_math_catch (_) {
        return bson_end(*doc);
    }
    bson_byte* out
        = _bson_prep_element_region(doc, &pos, BSON_TYPE_DBPOINTER, key, (uint32_t)el_size);
    if (out) {
        out        = _bson_write_int32le(out, collname_string_size);
        out        = _bson_memcpy_chr(out, dbp.collection, (uint32_t)collname_string_size - 1);
        (out++)->v = 0;
        out        = _bson_memcpy_u8(out, dbp.object_id.bytes, sizeof dbp.object_id);
    }
    return pos;
}

/**
 * @brief Insert a code string into the document
 *
 * @param doc The document being updated
 * @param pos The position at which to insert
 * @param key The new element's key
 * @param code A string of the code to be inserted
 * @return bson_iterator An iterator to the inserted element, or the end
 * iterator
 */
static mlib_constexpr bson_iterator bson_insert_code(bson_mut*      doc,
                                                     bson_iterator  pos,
                                                     bson_utf8_view key,
                                                     bson_utf8_view code) mlib_noexcept {
    return _bson_insert_stringlike(doc, pos, key, BSON_TYPE_CODE, code);
}

/**
 * @brief Insert a symbol string into the document
 *
 * @param doc The document being updated
 * @param pos The position at which to insert
 * @param key The new element's key
 * @param sym The symbol string to be inserted
 * @return bson_iterator An iterator to the inserted element, or the end
 * iterator
 */
static mlib_constexpr bson_iterator bson_insert_symbol(bson_mut*      doc,
                                                       bson_iterator  pos,
                                                       bson_utf8_view key,
                                                       bson_utf8_view sym) mlib_noexcept {
    return _bson_insert_stringlike(doc, pos, key, BSON_TYPE_SYMBOL, sym);
}

mlib_constexpr bson_iterator bson_insert_code_with_scope(bson_mut*      doc,
                                                         bson_iterator  pos,
                                                         bson_utf8_view key,
                                                         bson_utf8_view code,
                                                         bson_view      scope) mlib_noexcept {
    mlib_math_try();
    const int32_t code_size = mlibMathPositiveInt32(add(1, U(code.len)));
    const int32_t elem_size = mlibMathPositiveInt32(add(I(code_size), add(I(bson_size(scope)), 4)));
    mlib_math_catch (_) {
        return bson_end(*doc);
    }
    bson_byte* out
        = _bson_prep_element_region(doc, &pos, BSON_TYPE_CODEWSCOPE, key, (uint32_t)elem_size);
    if (out) {
        out = _bson_write_int32le(out, elem_size);
        out = _bson_write_int32le(out, code_size);
        out = _bson_memcpy_chr(out, code.data, (uint32_t)code.len);
        out = _bson_memcpy(out, bson_data(scope), bson_size(scope));
    }
    return pos;
}

/**
 * @brief Insert an Int32 value into the document
 *
 * @param doc The document being updated
 * @param pos The position at which to insert
 * @param key The new element's key
 * @param value The integer value for the new element
 * @return bson_iterator An iterator to the inserted element, or the end
 * iterator
 */
mlib_constexpr bson_iterator bson_insert_int32(bson_mut*      doc,
                                               bson_iterator  pos,
                                               bson_utf8_view key,
                                               int32_t        value) mlib_noexcept {
    bson_byte* out = _bson_prep_element_region(doc, &pos, BSON_TYPE_INT32, key, sizeof(int32_t));
    if (out) {
        out = _bson_write_int32le(out, value);
    }
    return pos;
}

/**
 * @brief Insert a timestamp value into the document
 *
 * @param doc The document being updated
 * @param pos The position at which to insert
 * @param key The new element's key
 * @param ts The timestamp for the new element
 * @return bson_iterator An iterator to the inserted element, or the end
 * iterator
 */
mlib_constexpr bson_iterator bson_insert_timestamp(bson_mut*      doc,
                                                   bson_iterator  pos,
                                                   bson_utf8_view key,
                                                   uint64_t       ts) mlib_noexcept {
    bson_byte* out = _bson_prep_element_region(doc, &pos, BSON_TYPE_TIMESTAMP, key, sizeof ts);
    if (out) {
        out = _bson_write_int64le(out, (uint64_t)ts);
    }
    return pos;
}

/**
 * @brief Insert a Int64 value into the document
 *
 * @param doc The document being updated
 * @param pos The position at which to insert
 * @param key The new element's key
 * @param value The integer value for the new element
 * @return bson_iterator An iterator to the inserted element, or the end
 * iterator
 */
mlib_constexpr bson_iterator bson_insert_int64(bson_mut*      doc,
                                               bson_iterator  pos,
                                               bson_utf8_view key,
                                               int64_t        value) mlib_noexcept {
    bson_byte* out = _bson_prep_element_region(doc, &pos, BSON_TYPE_INT64, key, sizeof value);
    if (out) {
        out = _bson_write_int64le(out, value);
    }
    return pos;
}

struct bson_decimal128 {
    uint8_t bytes[16];
};

/**
 * @brief Insert a Decimal128 value into the document
 *
 * @param doc The document being updated
 * @param pos The position at which to insert
 * @param key The new element's key
 * @param value The Decimal128 to insert
 * @return bson_iterator An iterator to the inserted element, or the end
 * iterator
 */
mlib_constexpr bson_iterator bson_insert_decimal128(bson_mut*              doc,
                                                    bson_iterator          pos,
                                                    bson_utf8_view         key,
                                                    struct bson_decimal128 value) mlib_noexcept {
    bson_byte* out = _bson_prep_element_region(doc, &pos, BSON_TYPE_DECIMAL128, key, sizeof value);
    if (out) {
        out = _bson_memcpy_u8(out, value.bytes, sizeof value);
    }
    return pos;
}

/**
 * @brief Insert a "Max key" into the document
 *
 * @param doc The document being updated
 * @param pos The position at which to insert
 * @param key The new element's key
 * @return bson_iterator An iterator to the inserted element, or the end
 * iterator
 */
mlib_constexpr bson_iterator bson_insert_maxkey(bson_mut*      doc,
                                                bson_iterator  pos,
                                                bson_utf8_view key) mlib_noexcept {
    _bson_prep_element_region(doc, &pos, BSON_TYPE_MAXKEY, key, 0);
    return pos;
}

/**
 * @brief Insert a "Min key" into the document
 *
 * @param doc The document being updated
 * @param pos The position at which to insert
 * @param key The new element's key
 * @return bson_iterator An iterator to the inserted element, or the end
 * iterator
 */
mlib_constexpr bson_iterator bson_insert_minkey(bson_mut*      doc,
                                                bson_iterator  pos,
                                                bson_utf8_view key) mlib_noexcept {
    _bson_prep_element_region(doc, &pos, BSON_TYPE_MINKEY, key, 0);
    return pos;
}

/**
 * @brief Replace the key string of an element within a document
 *
 * @param doc The document to update
 * @param pos The element that will be updated
 * @param newkey The new key string for the element.
 * @return bson_iterator The iterator referring to the element after being
 * updated.
 */
inline bson_iterator
bson_set_key(bson_mut* doc, bson_iterator pos, bson_utf8_view newkey) mlib_noexcept {
    mlib_math_try();
    BV_ASSERT(!bson_iterator_done(pos));
    // Truncate the key to not contain an null bytes:
    newkey = bson_utf8_view_chopnulls(newkey);
    // The current key:
    const bson_utf8_view curkey = bson_iterator_key(pos);
    // The number of bytes added for the new key (or possible negative)
    const ptrdiff_t size_diff = newkey.len - curkey.len;
    // The new remaining len following the adjusted key size
    const int32_t new_rlen = mlibMathPositiveInt32(add(I(pos._rlen), I(size_diff)));
    mlib_math_catch (_) {
        return bson_end(*doc);
    }
    // The current iterator's offset within the document:
    const ptrdiff_t iter_off = bson_iterator_data(pos) - bson_data(*doc);
    BV_ASSERT(iter_off > 0);
    bson_byte* out = _bson_splice_region(doc,
                                         _bson_mut_data_at(*doc, pos) + 1,
                                         curkey.len,
                                         newkey.len,
                                         (const bson_byte*)newkey.data);
    if (!out) {
        return bson_end(*doc);
    }
    // Adjust the iterator:
    pos._ptr    = bson_data(*doc) + iter_off;
    pos._keylen = newkey.len;
    pos._rlen   = new_rlen;
    return pos;
}

mlib_thread_local extern char _bson_tmp_integer_key_tl_storage[12];

/// Write the decimal representation of a uint32_t into the given string.
mlib_constexpr char* _bson_write_uint(uint32_t v, char* at) mlib_noexcept {
    if (v == 0) {
        *at++ = '0';
    } else if (v >= 10) {
        if (v < 100) {
            *at++ = '0' + (v / 10ull);
        } else {
            at = _bson_write_uint(v / 10ull, at);
        }
        *at++ = '0' + (v % 10ull);
    } else {
        *at++ = '0' + v;
    }
    *at = 0;
    return at;
}

/**
 * @brief A simple type used to store a small string representing the the
 * integer keys of array elements.
 */
struct bson_array_element_integer_keybuf {
    char buf[12];
};

/**
 * @brief Generate a UTF-8 string that contains the decimal spelling of the
 * given uint32_t value
 *
 * @param val The value to be represented
 * @return bson_utf8_view A view of the generated string. NOTE: This view will
 * remain valid only until a subsequent call to bson_tmp_uint_string within the
 * same thread.
 */
mlib_constexpr struct bson_array_element_integer_keybuf
bson_tmp_uint_string(uint32_t val) mlib_noexcept {
    struct bson_array_element_integer_keybuf arr = {0};
    const char* const                        end = _bson_write_uint(val, arr.buf);
    return arr;
}

inline void
bson_relabel_array_elements_at(bson_mut* doc, bson_iterator pos, uint32_t idx) mlib_noexcept {
    for (; !bson_iterator_done(pos); pos = bson_next(pos)) {
        struct bson_array_element_integer_keybuf key = bson_tmp_uint_string(idx);
        pos = bson_set_key(doc, pos, bson_utf8_view_from_cstring(key.buf));
    }
}

/**
 * @brief Given a BSON document, update the element keys such that they are
 * monotonically increasing decimal integers beginning from zero
 *
 * @param doc The document that will be modified.
 */
inline void bson_relabel_array_elements(bson_mut* doc) mlib_noexcept {
    bson_relabel_array_elements_at(doc, bson_begin(*doc), 0);
}

/**
 * @brief Insert and delete elements within a document in a single pass.
 *
 * @param doc The document that is being updated
 * @param pos The position at which to do the splice
 * @param delete_end The position at which to stop deleting elements
 * @param from_begin The beginning of the range of elements to insert at 'pos'
 * @param from_end The position at which to stop copying into 'pos'
 * @return bson_iterator An iterator referring to the beginning of the spliced
 * region, or the end iterator.
 *
 * @note The DISJOINT requirement applies to `from_begin` and `from_end`: If
 * `from_begin` and `from_end` are NOT equivalent, then `from_begin` and
 * `from_end` MUST NOT be iterators within `doc`.
 *
 * @note `delete_end` MUST be reachable from `pos`, and `from_end` MUST be
 * reachable from `from_begin`.
 */
mlib_constexpr bson_iterator bson_splice_disjoint_ranges(bson_mut*     doc,
                                                         bson_iterator pos,
                                                         bson_iterator delete_end,
                                                         bson_iterator from_begin,
                                                         bson_iterator from_end) mlib_noexcept {
    mlib_math_try();
    // We don't copy individually, since we can just memcpy the entire range:
    const bson_byte* const copy_begin = bson_iterator_data(from_begin);
    const bson_byte* const copy_end   = bson_iterator_data(from_end);
    ptrdiff_t              copy_size  = copy_end - copy_begin;
    BV_ASSERT(copy_size >= 0 && "Invalid insertion range given for bson_splice_disjoint_ranges()");
    // Same with delete, since we can just delete the whole range at once:
    ptrdiff_t delete_size = bson_iterator_data(delete_end) - bson_iterator_data(pos);
    BV_ASSERT(delete_size >= 0 && "Invalid deletion range for bson_splice_disjoint_ranges()");
    // The number of bytes different from when we began:
    const int32_t size_diff = mlibMathInt32(sub(I(copy_size), I(delete_size)));
    // The new "bytes remaining" size for the returned iterator:
    const int32_t new_rlen = mlibMathPositiveInt32(add(U(pos._rlen), I(size_diff)));
    mlib_math_catch (_) {
        return bson_end(*doc);
    }

    // Do the splice:
    bson_byte* new_posptr = _bson_splice_region(doc,
                                                _bson_mut_data_at(*doc, pos),
                                                (uint32_t)delete_size,
                                                (uint32_t)copy_size,
                                                copy_begin);
    if (!new_posptr) {
        // Failure...
        return bson_end(*doc);
    }
    // Update the iterator to point to the new region:
    pos._ptr  = new_posptr;
    pos._rlen = new_rlen;
    if (copy_size) {
        // We inserted an element from another range, and that one knows its key
        // length:
        pos._keylen = from_begin._keylen;
    } else {
        // We only delete elements, so the deletion end is now the beginning of
        // the range:
        pos._keylen = delete_end._keylen;
    }
    return pos;
}

mlib_constexpr bson_iterator bson_insert_disjoint_range(bson_mut*     doc,
                                                        bson_iterator pos,
                                                        bson_iterator from_begin,
                                                        bson_iterator from_end) mlib_noexcept {
    return bson_splice_disjoint_ranges(doc, pos, pos, from_begin, from_end);
}

/**
 * @brief Remove a range of elements from a document
 *
 * Delete elements from the document, starting with `first`, and continuing
 * until (but not including) `last`.
 *
 * @param doc The document to update
 * @param first The first element to remove from `doc`
 * @param last The element at which to stop removing elements.
 * @return bson_iterator An iterator that refers to the new location of `last`
 *
 * @note If `first` and `last` are equal, no elements are removed.
 * @note If `last` is not reachable from `first`, the behavior is undefined.
 */
mlib_constexpr bson_iterator bson_erase_range(bson_mut* const     doc,
                                              const bson_iterator first,
                                              const bson_iterator last) mlib_noexcept {
    return bson_splice_disjoint_ranges(doc, first, last, last, last);
}

/**
 * @brief Remove a single element from the given bson document
 *
 * @param doc The document to modify
 * @param pos An iterator pointing to the single element to remove. Must not be
 * the end position
 * @return bson_iterator An iterator referring to the element after the removed
 * element.
 */
static mlib_constexpr bson_iterator bson_erase(bson_mut* doc, bson_iterator pos) mlib_noexcept {
    return bson_erase_range(doc, pos, bson_next(pos));
}

mlib_extern_c_end();

#if mlib_is_cxx()

namespace bson {

using view      = ::bson_view;
using utf8_view = ::bson_utf8_view;

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
        _mut = bson_mut_new_ex(alloc.c_allocator(), reserve_size);
        if (data() == nullptr) {
            throw std::bad_alloc();
        }
    }

    /**
     * @brief Take ownership of a C-style `::bson_mut` object
     */
    constexpr explicit document(bson_mut&& o) noexcept {
        _mut = o;
        o    = bson_mut{};
    }

    /**
     * @brief Copy from the given BSON view
     *
     * @param v The document to be copied
     * @param alloc An allocator for the operation
     */
    constexpr explicit document(bson_view v, allocator_type alloc)
        : _mut(bson_mut_new_ex(alloc.c_allocator(), bson_size(v))) {
        if (data() == nullptr)
            throw std::bad_alloc();
        memcpy(data(), v.data(), v.byte_size());
    }

    constexpr document(document const& other) { _mut = bson_mut_copy(other._mut); }
    constexpr document(document&& other) noexcept
        : _mut(((document&&)other).release()) {}

    constexpr document& operator=(const document& other) {
        _del();
        _mut = bson_mut_copy(other._mut);
        if (data() == nullptr) {
            throw std::bad_alloc();
        }
        return *this;
    }

    constexpr document& operator=(document&& other) noexcept {
        _del();
        _mut = ((document&&)other).release();
        return *this;
    }

    constexpr iterator begin() const noexcept { return bson_begin(_mut); }
    constexpr iterator end() const noexcept { return bson_end(_mut); }

    constexpr iterator find(auto&& key) const noexcept
        requires requires(view v) { v.find(key); }
    {
        return view(*this).find(key);
    }

    constexpr bson_byte*       data() noexcept { return bson_mut_data(_mut); }
    constexpr const bson_byte* data() const noexcept { return bson_data(_mut); }
    constexpr std::size_t      byte_size() const noexcept { return bson_size(_mut); }

    constexpr bool empty() const noexcept { return byte_size() == 5; }

    constexpr void reserve(std::size_t n) {
        if (bson_reserve(&_mut, static_cast<std::uint32_t>(n)) < 0) {
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

    constexpr bson_mut release() && noexcept {
        auto m                   = _mut;
        _mut._bson_document_data = nullptr;
        return m;
    }

    constexpr allocator_type get_allocator() const noexcept {
        return allocator_type(bson_mut_get_allocator(_mut));
    }

private:
    constexpr void _del() noexcept {
        if (_mut._bson_document_data
            and _mut._capacity_or_negative_offset_within_parent_data >= 0) {
            // Moving from the object will set the data pointer to null, which prevents a
            // double-free
            bson_mut_delete(_mut);
        }
        _mut._bson_document_data = nullptr;
    }
    bson_mut _mut;

    constexpr iterator _do_emplace(iterator pos, utf8_view key, bson_view d) noexcept {
        return bson_insert_doc(&_mut, pos, key, d);
    }
    constexpr iterator _do_emplace(iterator pos, utf8_view key, double d) noexcept {
        return bson_insert_double(&_mut, pos, key, d);
    }
    constexpr iterator _do_emplace(iterator pos, utf8_view key, float d) noexcept {
        return bson_insert_double(&_mut, pos, key, d);
    }
    constexpr iterator _do_emplace(iterator pos, utf8_view key, std::int32_t d) noexcept {
        return bson_insert_int32(&_mut, pos, key, d);
    }
    constexpr iterator _do_emplace(iterator pos, utf8_view key, std::int64_t d) noexcept {
        return bson_insert_int64(&_mut, pos, key, d);
    }
    constexpr iterator _do_emplace(iterator pos, utf8_view key, std::string_view d) noexcept {
        return bson_insert_utf8(&_mut, pos, key, utf8_view::from_str(d));
    }
    constexpr iterator _do_emplace(iterator pos, utf8_view key, null_t) noexcept {
        return bson_insert_null(&_mut, pos, key);
    }
    constexpr iterator _do_emplace(iterator pos, utf8_view key, undefined_t) noexcept {
        return bson_insert_undefined(&_mut, pos, key);
    }
    // Constrain to exactly handle `bool` (no conversions)
    constexpr iterator
    _do_emplace(iterator pos, utf8_view key, std::same_as<bool> auto b) noexcept {
        return bson_insert_bool(&_mut, pos, key, b);
    }

    constexpr iterator _emplace(iterator pos, std::string_view key_, auto const& val)
        requires requires(utf8_view key) { _do_emplace(pos, key, val); }
    {
        const utf8_view key = utf8_view::from_str(key_);
        const iterator  ret = _do_emplace(pos, key, val);
        if (ret == end()) {
            // This will only occur if there was an allocation failure
            throw std::bad_alloc();
        }
        return ret;
    }

    struct subdoc_tag {
        bson_mut m;
    };

    constexpr document(subdoc_tag s) noexcept
        : _mut(s.m) {}

public:
    struct inserted_subdocument;
    constexpr iterator insert(iterator pos, auto const& pair)
        requires requires { _emplace(pos, std::get<0>(pair), std::get<1>(pair)); }
    {
        return _emplace(pos, std::get<0>(pair), std::get<1>(pair));
    }

    constexpr iterator emplace(iterator pos, std::string_view key, auto const& val)
        requires requires { _emplace(pos, key, val); }
    {
        return _emplace(pos, key, val);
    }

    constexpr iterator push_back(auto const& pair)
        requires requires { insert(end(), pair); }
    {
        return insert(end(), pair);
    }

    constexpr iterator emplace_back(std::string_view key, auto const& val)
        requires requires { emplace(end(), key, val); }
    {
        return emplace(end(), key, val);
    }

    inline inserted_subdocument insert_subdoc(iterator pos, std::string_view key);
    inline inserted_subdocument insert_array(iterator pos, std::string_view key);

    inline document push_subdoc(std::string_view key);
    inline document push_array(std::string_view key);

    [[nodiscard]] document child(iterator pos) noexcept {
        return document(subdoc_tag{::bson_mut_subdocument(&_mut, pos)});
    }

    [[nodiscard]] iterator position_in_parent() const noexcept {
        return ::bson_parent_iterator(_mut);
    }
};

struct document::inserted_subdocument {
    iterator position;
    document mutator;
};

document::inserted_subdocument document::insert_subdoc(iterator pos, std::string_view key) {
    auto it = ::bson_insert_doc(&_mut, pos, utf8_view::from_str(key), BSON_VIEW_NULL);
    return {it, child(it)};
}

document::inserted_subdocument document::insert_array(iterator pos, std::string_view key) {
    auto it = ::bson_insert_array(&_mut, pos, utf8_view::from_str(key));
    return {it, child(it)};
}

document document::push_subdoc(std::string_view key) { return insert_subdoc(end(), key).mutator; }
document document::push_array(std::string_view key) { return insert_array(end(), key).mutator; }

}  // namespace bson
#endif  // C++

#undef BV_ASSERT
