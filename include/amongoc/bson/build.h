#pragma once

#include "./view.h"

#include <mlib/config.h>

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if mlib_is_cxx()
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
inline bson_byte* _bson_write_int32le(bson_byte* bytes, int32_t i32) mlib_noexcept {
    uint32_t v = (uint32_t)i32;
    bytes[0].v = (v >> 0) & 0xff;
    bytes[1].v = (v >> 8) & 0xff;
    bytes[2].v = (v >> 16) & 0xff;
    bytes[3].v = (v >> 24) & 0xff;
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
inline bson_byte* _bson_write_int64le(bson_byte* out, int64_t i64) mlib_noexcept {
    const uint64_t u64 = (uint64_t)i64;
    out                = _bson_write_int32le(out, (int32_t)u64);
    out                = _bson_write_int32le(out, (int32_t)(u64 >> 32));
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
inline bson_byte* _bson_memcpy(bson_byte* out, const void* src, uint32_t len) mlib_noexcept {
    if (src && len) {
        memcpy(out, src, len);
    }
    return out + len;
}

/**
 * @brief Type of the function used to manage memory
 *
 * @param ptr Pointer to the memory that was previously allocated, or NULL if
 * no data has been allocated previously.
 * @param requested_size The requested allocation size, or zero to free the
 * memory.
 * @param previous_size The previous size of the allocated region, or zero.
 * @param out_new_size Upon success, the allocator MUST write the actual size
 * of memory that was allocated to this address.
 * @param userdata Arbitrary pointer that was given when the allocator was
 * created.
 *
 * @returns A pointer to the allocated memory, or NULL on allocation failure.
 *
 * @note The allocation function is responsible for copying data from the
 * prior memory region into the new memory region.
 * @note If this function returns NULL, it is assumed that `ptr` is still vaild
 */
typedef bson_byte* (*bson_mut_allocator_fn)(bson_byte* ptr,
                                            uint32_t   requested_size,
                                            uint32_t   previous_size,
                                            uint32_t*  out_new_size,
                                            void*      userdata);

/**
 * @brief Type used to control memory allocation in a @ref bson_mut, given to
 * @ref bson_mut_new_ex
 */
typedef struct bson_mut_allocator {
    /**
     * @brief The function used to allocate memory for a bson_mut. Refer to
     * @ref bson_mut_allocator_fn for signature information
     */
    bson_mut_allocator_fn reallocate;
    /**
     * @brief An arbitrary pointer. Will be passed to `reallocate()` when it is
     * called.
     */
    void* userdata;
} bson_mut_allocator;

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
    /**
     * @brief The parent mutator, or the allocator context
     *
     * @internal
     *
     * In CHILD MODE, this points to a `bson_mut` that manages the document that
     * contains this document.
     *
     * In ROOT_MODE, this is a pointer to a `bson_mut_allocator`, to be used to
     * manage the `_bson_document_data` pointer.
     */
    void* _parent_mut_or_allocator_context;
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
inline uint32_t bson_capacity(bson_mut d) {
    if (d._capacity_or_negative_offset_within_parent_data < 0) {
        // We are a subdocument, so we need to calculate the capacity of our
        // document in the context of the parent's capacity.
        bson_mut parent = *(bson_mut*)d._parent_mut_or_allocator_context;
        // Number of bytes between the parent start and this element start:
        const mcd_integer bytes_before
            = mcMath(neg(I(d._capacity_or_negative_offset_within_parent_data)));
        // Number of bytes from the element start until the end of the parent:
        const mcd_integer bytes_until_parent_end
            = mcMath(sub(I(bson_ssize(parent)), V(bytes_before)));
        // Num bytes in the parent after this document element:
        const mcd_integer bytes_after = mcMath(sub(V(bytes_until_parent_end), I(bson_ssize(d))));
        // Number of bytes in the parent that are not our own:
        const mcd_integer bytes_other = mcMath(add(V(bytes_before), V(bytes_after)));
        // The number of bytes we can grow to until we will break the capacity of
        // the parent:
        const mcd_integer bytes_remaining
            = mcMath(checkNonNegativeInt32(sub(I(bson_capacity(parent)), V(bytes_other))));
        mcMath(assertNot(MC_INTEGER_ALLBITS, V(bytes_remaining)));
        return (uint32_t)bytes_remaining.i64;
    }
    return (uint32_t)d._capacity_or_negative_offset_within_parent_data;
}

/**
 * @brief The default reallocation function for a bson_mut
 *
 * This function is used to manage the data buffers of a bson_mut as it is
 * manipulated. This is implemented in terms of realloc() and free() from
 * stdlib.h
 *
 * @see bson_mut_allocator_fn
 */
inline bson_byte* bson_mut_default_reallocate(bson_byte* previous,
                                              uint32_t   request_size,
                                              uint32_t   prev_size,
                                              uint32_t*  actual_size,
                                              void*      userdata) {
    // An allocation of zero is a request to free:
    if (request_size == 0) {
        free(previous);
        *actual_size = 0;
        return NULL;
    }
    // Reallocate:
    bson_byte* const p = (bson_byte*)realloc(previous, request_size);
    if (!p) {
        // Failure:
        return NULL;
    }
    // Tell the caller how much we allocated
    *actual_size = request_size;
    return p;
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
inline bool _bson_mut_realloc(bson_mut* m, uint32_t new_size) {
    // We are only called on ROOT MODE bson_muts
    BV_ASSERT(m->_capacity_or_negative_offset_within_parent_data >= 0);
    // Cap allocation size:
    if (new_size > INT32_MAX) {
        return false;
    }
    // Get the allocator:
    bson_mut_allocator* alloc = (bson_mut_allocator*)m->_parent_mut_or_allocator_context;
    // Perform the reallocation:
    uint32_t   got_size = 0;
    bson_byte* newptr
        = alloc->reallocate(m->_bson_document_data,
                            new_size,
                            (uint32_t)m->_capacity_or_negative_offset_within_parent_data,
                            &got_size,
                            alloc->userdata);
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
inline int32_t bson_reserve(bson_mut* d, uint32_t size) {
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
inline bson_mut bson_mut_new_ex(const bson_mut_allocator* allocator, uint32_t reserve) {
    // The default allocator, stored as a static object:
    static const bson_mut_allocator default_vtab = {
        .reallocate = bson_mut_default_reallocate,
    };
    if (allocator == NULL) {
        // They didn't provide an allocator: Use the default one
        allocator = &default_vtab;
    }
    // Create the object:
    bson_mut r                                        = {0};
    r._capacity_or_negative_offset_within_parent_data = 0;
    r._parent_mut_or_allocator_context                = (void*)allocator;
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
    memset(r._bson_document_data, 0, (uint32_t)r._capacity_or_negative_offset_within_parent_data);
    r._bson_document_data[0].v = 5;
    return r;
}

/**
 * @brief Create a new empty document for later manipulation
 *
 * @note The return value must later be deleted using bson_mut_delete
 */
inline bson_mut bson_mut_new(void) { return bson_mut_new_ex(NULL, 512); }

inline bson_mut_allocator* bson_mut_get_allocator(bson_mut m) {
    if (m._capacity_or_negative_offset_within_parent_data < 0) {
        // We are a child document
        return bson_mut_get_allocator(*(bson_mut*)m._parent_mut_or_allocator_context);
    }
    return (bson_mut_allocator*)m._parent_mut_or_allocator_context;
}

inline bson_mut bson_mut_copy(bson_mut other) {
    bson_mut ret = bson_mut_new_ex(bson_mut_get_allocator(other), bson_size(other));
    memcpy(ret._bson_document_data, bson_data(other), bson_size(other));
    return ret;
}

/**
 * @brief Free the resources of the given BSON document
 */
inline void bson_mut_delete(bson_mut d) { _bson_mut_realloc(&d, 0); }

/**
 * @internal
 * @brief Obtain a pointer to the element data at the given iterator position
 *
 * @param doc The document that owns the element
 * @param pos An element iterator
 */
inline bson_byte* _bson_mut_data_at(bson_mut doc, bson_iterator pos) {
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
inline bson_byte* _bson_splice_region(bson_mut* const        doc,
                                      bson_byte*             position,
                                      uint32_t               n_delete,
                                      uint32_t               n_insert,
                                      const bson_byte* const insert_from) {
    mcMathOnFail(err) { return NULL; }
    // The offset of the position. We use this to later recover a pointer upon
    // reallocation
    const ssize_t pos_offset = position - bson_data(*doc);
    //  Check that we aren't splicing within our document header:
    BV_ASSERT(pos_offset >= 4);
    // Check that we aren't splicing at/beyond the doc's null terminator:
    BV_ASSERT(pos_offset < bson_ssize(*doc));

    // Calculate the new document size:
    const int32_t size_diff = mcMathInt32(sub(I(n_insert), I(n_delete)));
    uint32_t      new_doc_size
        = (uint32_t)mcMathNonNegativeInt32(add(I(bson_ssize(*doc)), I(size_diff)));

    if (doc->_capacity_or_negative_offset_within_parent_data < 0) {
        // We are in CHILD MODE, so we need to tell the parent to do the work:
        bson_mut* parent = (bson_mut*)doc->_parent_mut_or_allocator_context;
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
            = (uint32_t)mcMathNonNegativeInt32(I(doc_data_end - position));
        if (n_delete > avail_to_delete) {
            // Not enough data to actual delete, so this splice request is bogus?
            return NULL;
        }
        // We are the root of the document, so we do the actual work:
        if (new_doc_size > bson_capacity(*doc)) {
            // We need to grow larger. Add some extra to prevent repeated
            // allocations:
            const uint32_t new_capacity
                = (uint32_t)mcMathNonNegativeInt32(add(I(new_doc_size), 1024));
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
        const uint32_t data_remain = (uint32_t)mcMathNonNegativeInt32(I(doc_end - move_from));
        // Move the data, and insert a marker in the empty space:
        memmove(move_dest, move_from, data_remain);
        if (insert_from) {
            memmove(position, insert_from, n_insert);
        } else {
            memset(position, 'X', n_insert);
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
inline bson_byte* _bson_prep_element_region(bson_mut* const      d,
                                            bson_iterator* const pos,
                                            const bson_type      type,
                                            bson_utf8_view       key,
                                            const uint32_t       datasize) {
    mcMathOnFail(_) {
        *pos = bson_end(*d);
        return NULL;
    }
    // Prevent embedded nulls within document keys:
    key = bson_utf8_view_chopnulls(key);
    // The total size of the element (add two: for the tag and the key's null):
    const uint32_t elem_size
        = (uint32_t)mcMathNonNegativeInt32(add(U(key.len), add(2, U(datasize))));
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
    outptr = _bson_memcpy(outptr, key.data, key.len);
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

inline bson_iterator _bson_insert_stringlike(bson_mut*      doc,
                                             bson_iterator  pos,
                                             bson_utf8_view key,
                                             bson_type      realtype,
                                             bson_utf8_view string) {
    mcMathOnFail(_) { return bson_end(*doc); }
    const int32_t string_size = mcMathPositiveInt32(add(U(string.len), 1));
    const int32_t elem_size   = mcMathPositiveInt32(add(I(string_size), 4));
    bson_byte*    out = _bson_prep_element_region(doc, &pos, realtype, key, (uint32_t)elem_size);
    if (out) {
        out    = _bson_write_int32le(out, string_size);
        out    = _bson_memcpy(out, string.data, (uint32_t)string.len);
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
inline bson_iterator
bson_insert_double(bson_mut* doc, bson_iterator pos, bson_utf8_view key, double d) {
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
static inline bson_iterator
bson_insert_utf8(bson_mut* doc, bson_iterator pos, bson_utf8_view key, bson_utf8_view utf8) {
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
inline bson_iterator
bson_insert_doc(bson_mut* doc, bson_iterator pos, bson_utf8_view key, bson_view insert_doc) {
    if (!bson_data(insert_doc)) {
        // There was no document given. Re-call ourself with a view of an empty
        // doc:
        bson_byte empty_doc[5] = {0};
        empty_doc[0].v         = 5;
        return bson_insert_doc(doc, pos, key, bson_view_from_data(empty_doc, sizeof doc, NULL));
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
inline bson_iterator bson_insert_array(bson_mut* doc, bson_iterator pos, bson_utf8_view key) {
    bson_byte* out = _bson_prep_element_region(doc, &pos, BSON_TYPE_ARRAY, key, 5);
    if (out) {
        memset(out, 0, 5);
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
inline bson_mut bson_mut_subdocument(bson_mut* parent, bson_iterator subdoc_iter) {
    bson_mut ret = {0};
    if (bson_iterator_type(subdoc_iter) != BSON_TYPE_DOCUMENT
        && bson_iterator_type(subdoc_iter) != BSON_TYPE_ARRAY) {
        // The element is not a document, so return a null mutator.
        return ret;
    }
    // Remember the parent:
    ret._parent_mut_or_allocator_context = parent;
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
inline bson_iterator bson_parent_iterator(bson_mut doc) {
    BV_ASSERT(doc._capacity_or_negative_offset_within_parent_data < 0);
    bson_mut      par = *(bson_mut*)doc._parent_mut_or_allocator_context;
    bson_iterator ret = {0};
    // Recover the address of the element:
    ret._ptr         = bson_mut_data(par) + -doc._capacity_or_negative_offset_within_parent_data;
    ptrdiff_t offset = ret._ptr - bson_mut_data(par);
    ret._keylen      = (int32_t)mcMath(
                      assertNot(MC_INTEGER_ALLBITS, sub(I(doc._bson_document_data - ret._ptr), 2)))
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
inline bson_iterator
bson_insert_binary(bson_mut* doc, bson_iterator pos, bson_utf8_view key, bson_binary bin) {
    mcMathOnFail(_) { return bson_end(*doc); }
    const int32_t bin_size  = mcMathNonNegativeInt32(I(bin.data_len));
    const int32_t elem_size = mcMathPositiveInt32(add(I(bin_size), 5));
    bson_byte*    out
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
inline bson_iterator bson_insert_undefined(bson_mut* doc, bson_iterator pos, bson_utf8_view key) {
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
inline bson_iterator
bson_insert_oid(bson_mut* doc, bson_iterator pos, bson_utf8_view key, bson_oid oid) {
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
inline bson_iterator
bson_insert_bool(bson_mut* doc, bson_iterator pos, bson_utf8_view key, bool b) {
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
inline bson_iterator
bson_insert_datetime(bson_mut* doc, bson_iterator pos, bson_utf8_view key, int64_t dt) {
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
inline bson_iterator bson_insert_null(bson_mut* doc, bson_iterator pos, bson_utf8_view key) {
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
inline bson_iterator
bson_insert_regex(bson_mut* doc, bson_iterator pos, bson_utf8_view key, bson_regex rx) {
    mcMathOnFail(_) { return bson_end(*doc); }
    // Neither regex nor options may contain embedded nulls, so we cannot trust
    // the caller giving us the correct length:
    // Compute regex length:
    const int32_t rx_len = mcMathNonNegativeInt32(strnlen(rx.regex, I(rx.regex_len)));
    // Compute options length:
    const int32_t opts_len
        = rx.options ? mcMathNonNegativeInt32(strnlen32(rx.options, I(rx.options_len))) : 0;
    // The total value size:
    const int32_t size = mcMathNonNegativeInt32(checkMin(2, add(I(rx_len), add(I(opts_len), 5))));
    bson_byte*    out  = _bson_prep_element_region(doc, &pos, BSON_TYPE_REGEX, key, (uint32_t)size);
    if (out) {
        out        = _bson_memcpy(out, rx.regex, (uint32_t)rx_len);
        (out++)->v = 0;
        if (rx.options) {
            out = _bson_memcpy(out, rx.options, (uint32_t)opts_len);
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
inline bson_iterator
bson_insert_dbpointer(bson_mut* doc, bson_iterator pos, bson_utf8_view key, bson_dbpointer dbp) {
    mcMathOnFail(_) { return bson_end(*doc); }
    const int32_t collname_string_size
        = mcMathPositiveInt32(add(strnlen(dbp.collection, I(dbp.collection_len)), 1));
    const int32_t el_size = mcMathNonNegativeInt32(add(I(collname_string_size), I(12 + 4)));
    bson_byte*    out
        = _bson_prep_element_region(doc, &pos, BSON_TYPE_DBPOINTER, key, (uint32_t)el_size);
    if (out) {
        out        = _bson_write_int32le(out, collname_string_size);
        out        = _bson_memcpy(out, dbp.collection, (uint32_t)collname_string_size - 1);
        (out++)->v = 0;
        out        = _bson_memcpy(out, &dbp.object_id, sizeof dbp.object_id);
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
static inline bson_iterator
bson_insert_code(bson_mut* doc, bson_iterator pos, bson_utf8_view key, bson_utf8_view code) {
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
static inline bson_iterator
bson_insert_symbol(bson_mut* doc, bson_iterator pos, bson_utf8_view key, bson_utf8_view sym) {
    return _bson_insert_stringlike(doc, pos, key, BSON_TYPE_SYMBOL, sym);
}

inline bson_iterator bson_insert_code_with_scope(bson_mut*      doc,
                                                 bson_iterator  pos,
                                                 bson_utf8_view key,
                                                 bson_utf8_view code,
                                                 bson_view      scope) {
    mcMathOnFail(err) { return bson_end(*doc); }
    const int32_t code_size = mcMathPositiveInt32(add(1, U(code.len)));
    const int32_t elem_size = mcMathPositiveInt32(add(I(code_size), add(I(bson_size(scope)), 4)));
    bson_byte*    out
        = _bson_prep_element_region(doc, &pos, BSON_TYPE_CODEWSCOPE, key, (uint32_t)elem_size);
    if (out) {
        out = _bson_write_int32le(out, elem_size);
        out = _bson_write_int32le(out, code_size);
        out = _bson_memcpy(out, code.data, (uint32_t)code.len);
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
inline bson_iterator
bson_insert_int32(bson_mut* doc, bson_iterator pos, bson_utf8_view key, int32_t value) {
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
inline bson_iterator
bson_insert_timestamp(bson_mut* doc, bson_iterator pos, bson_utf8_view key, uint64_t ts) {
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
inline bson_iterator
bson_insert_int64(bson_mut* doc, bson_iterator pos, bson_utf8_view key, int64_t value) {
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
inline bson_iterator bson_insert_decimal128(bson_mut*              doc,
                                            bson_iterator          pos,
                                            bson_utf8_view         key,
                                            struct bson_decimal128 value) {
    bson_byte* out = _bson_prep_element_region(doc, &pos, BSON_TYPE_DECIMAL128, key, sizeof value);
    if (out) {
        out = _bson_memcpy(out, &value, sizeof value);
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
inline bson_iterator bson_insert_maxkey(bson_mut* doc, bson_iterator pos, bson_utf8_view key) {
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
inline bson_iterator bson_insert_minkey(bson_mut* doc, bson_iterator pos, bson_utf8_view key) {
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
    mcMathOnFail(_) { return bson_end(*doc); }
    BV_ASSERT(!bson_iterator_done(pos));
    // Truncate the key to not contain an null bytes:
    newkey = bson_utf8_view_chopnulls(newkey);
    // The current key:
    const bson_utf8_view curkey = bson_iterator_key(pos);
    // The number of bytes added for the new key (or possible negative)
    const ptrdiff_t size_diff = newkey.len - curkey.len;
    // The new remaining len following the adjusted key size
    const int32_t new_rlen = mcMathPositiveInt32(add(I(pos._rlen), I(size_diff)));
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

#if mlib_is_cxx()
thread_local
#else
_Thread_local
#endif
    extern char _bson_tmp_integer_key_tl_storage[12];

/// Write the decimal representation of a uint32_t into the given string.
inline char* _bson_write_uint(uint32_t v, char* at) {
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
 * @brief Generate a UTF-8 string that contains the decimal spelling of the
 * given uint32_t value
 *
 * @param val The value to be represented
 * @return bson_utf8_view A view of the generated string. NOTE: This view will
 * remain valid only until a subsequent call to bson_tmp_uint_string within the
 * same thread.
 */
inline bson_utf8_view bson_tmp_uint_string(uint32_t val) mlib_noexcept {
    const char* const end = _bson_write_uint(val, _bson_tmp_integer_key_tl_storage);
    return bson_utf8_view_from_data(_bson_tmp_integer_key_tl_storage,
                                    (size_t)(end - _bson_tmp_integer_key_tl_storage));
}

inline void
bson_relabel_array_elements_at(bson_mut* doc, bson_iterator pos, uint32_t idx) mlib_noexcept {
    for (; !bson_iterator_done(pos); pos = bson_next(pos)) {
        pos = bson_set_key(doc, pos, bson_tmp_uint_string(idx));
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
inline bson_iterator bson_splice_disjoint_ranges(bson_mut*     doc,
                                                 bson_iterator pos,
                                                 bson_iterator delete_end,
                                                 bson_iterator from_begin,
                                                 bson_iterator from_end) mlib_noexcept {
    mcMathOnFail(_) { return bson_end(*doc); }
    // We don't copy individually, since we can just memcpy the entire range:
    const bson_byte* const copy_begin = bson_iterator_data(from_begin);
    const bson_byte* const copy_end   = bson_iterator_data(from_end);
    ptrdiff_t              copy_size  = copy_end - copy_begin;
    BV_ASSERT(copy_size >= 0 && "Invalid insertion range given for bson_splice_disjoint_ranges()");
    // Same with delete, since we can just delete the whole range at once:
    ptrdiff_t delete_size = bson_iterator_data(delete_end) - bson_iterator_data(pos);
    BV_ASSERT(delete_size >= 0 && "Invalid deletion range for bson_splice_disjoint_ranges()");
    // The number of bytes different from when we began:
    const int32_t size_diff = mcMathInt32(sub(I(copy_size), I(delete_size)));
    // The new "bytes remaining" size for the returned iterator:
    const int32_t new_rlen = mcMathPositiveInt32(add(U(pos._rlen), I(size_diff)));

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

inline bson_iterator bson_insert_disjoint_range(bson_mut*     doc,
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
inline bson_iterator
bson_erase_range(bson_mut* const doc, const bson_iterator first, const bson_iterator last) {
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
static inline bson_iterator bson_erase(bson_mut* doc, bson_iterator pos) {
    return bson_erase_range(doc, pos, bson_next(pos));
}

mlib_extern_c_end();

#if mlib_is_cxx()
class bson_doc {
public:
    bson_doc() { _mut = bson_mut_new(); }
    ~bson_doc() { _del(); }

    explicit bson_doc(bson_mut&& o) {
        _mut = o;
        o    = bson_mut{};
    }

    explicit bson_doc(bson_view v)
        : _mut(bson_mut_new_ex(nullptr, bson_size(v))) {
        memcpy(data(), bson_data(v), bson_size(v));
    }

    bson_doc(bson_doc const& other) { _mut = bson_mut_copy(other._mut); }
    bson_doc(bson_doc&& other)
        : _mut(((bson_doc&&)other).release()) {}

    bson_doc& operator=(const bson_doc& other) noexcept {
        _del();
        _mut = bson_mut_copy(other._mut);
        return *this;
    }

    bson_doc& operator=(bson_doc&& other) noexcept {
        _del();
        _mut = ((bson_doc&&)other).release();
        return *this;
    }

    bson_iterator begin() const noexcept { return bson_begin(_mut); }
    bson_iterator end() const noexcept { return bson_end(_mut); }

    bson_byte*       data() noexcept { return bson_mut_data(_mut); }
    const bson_byte* data() const noexcept { return bson_data(_mut); }
    std::size_t      byte_size() const noexcept { return bson_size(_mut); }

    bson_iterator insert(bson_iterator pos, std::string_view key, std::same_as<double> auto d) {
        return bson_insert_double(&_mut, pos, bson_utf8_view::from_str(key), d);
    }

    bson_iterator insert(bson_iterator pos, std::string_view key, std::string_view str) {
        return bson_insert_utf8(&_mut,
                                pos,
                                bson_utf8_view::from_str(key),
                                bson_utf8_view::from_str(key));
    }

    bson_iterator push_back(std::string_view key, auto&& val)
        requires requires { insert(end(), key, val); }
    {
        return insert(end(), key, val);
    }

    void reserve(std::size_t n) {
        if (bson_reserve(&_mut, static_cast<std::uint32_t>(n)) < 0) {
            throw std::bad_alloc();
        }
    }

    operator bson_view() const noexcept { return bson_view::from_data(data(), byte_size()); }

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

    bson_mut release() && noexcept {
        auto m                   = _mut;
        _mut._bson_document_data = nullptr;
        return m;
    }

private:
    void _del() noexcept {
        if (_mut._bson_document_data) {
            // Moving from the object will set the data pointer to null, which prevents a
            // double-free
            bson_mut_delete(_mut);
        }
        _mut._bson_document_data = nullptr;
    }
    bson_mut _mut;
};
#endif  // C++

#undef BV_ASSERT
