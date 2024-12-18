#pragma once

#include <bson/detail/assert.h>
#include <bson/detail/mem.h>
#include <bson/doc.h>
#include <bson/iterator.h>
#include <bson/value.h>
#include <bson/value_ref.h>
#include <bson/view.h>

#include <mlib/config.h>
#include <mlib/str.h>

mlib_extern_c_begin();

/**
 * @brief Obtain a mutator for the given BSON document
 */
inline bson_mut bson_mutate(bson_doc* doc) mlib_noexcept {
    bson_mut ret;
    ret._bson_document_data        = bson_mut_data(*doc);
    ret._offset_within_parent_data = 0;
    ret._doc                       = doc;
    return ret;
}

/**
 * @brief Compute the number of bytes available before we will require
 * reallocating
 *
 * @note If `d` is a child document, the return value reflects the number of
 * bytes for the maximum size of `d` until which we will require reallocating
 * the parent, transitively.
 */
inline uint32_t bson_mut_capacity(bson_mut d) mlib_noexcept {
    if (d._offset_within_parent_data > 0) {
        // We are a subdocument, so we need to calculate the capacity of our
        // document in the context of the parent's capacity.
        bson_mut parent = *d._parent_mut;
        // Number of bytes between the parent start and this element start:
        const mlib_integer bytes_before = mlibMath(neg(I(d._offset_within_parent_data)));
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
            = mlibMath(checkNonNegativeInt32(sub(I(bson_mut_capacity(parent)), V(bytes_other))));
        mlibMath(assertNot(mlib_integer_allbits, V(bytes_remaining)));
        return (uint32_t)bytes_remaining.i64;
    }
    return bson_doc_capacity(*d._doc);
}

/**
 * @internal
 * @brief Obtain a pointer to the element data at the given iterator position
 *
 * @param doc The document that owns the element
 * @param pos An element iterator
 */
inline bson_byte* _bson_mut_data_at(bson_mut doc, bson_iterator pos) mlib_noexcept {
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
inline bson_byte* _bson_splice_region(bson_mut* const        mut,
                                      bson_byte*             position,
                                      size_t                 n_delete,
                                      size_t                 n_insert,
                                      const bson_byte* const insert_from) mlib_noexcept {
    // The offset of the position. We use this to later recover a pointer upon
    // reallocation
    const ssize_t pos_offset = position - bson_data(*mut);
    //  Check that we aren't splicing within our document header:
    BV_ASSERT(pos_offset >= 4);
    // Check that we aren't splicing at/beyond the doc's null terminator:
    BV_ASSERT(pos_offset < bson_ssize(*mut));

    // Calculate the new document size:
    mlib_math_try();
    const int32_t size_diff = mlibMathInt32(sub(U(n_insert), U(n_delete)));
    uint32_t      new_doc_size
        = (uint32_t)mlibMathNonNegativeInt32(add(I(bson_ssize(*mut)), I(size_diff)));
    mlib_math_catch (_unused) {
        (void)_unused;
        return NULL;
    }

    if (mut->_offset_within_parent_data > 0) {
        // We are in CHILD MODE, so we need to tell the parent to do the work:
        bson_mut* parent = mut->_parent_mut;
        // Our document offset within the parent, to adjust after reallocation:
        const ptrdiff_t my_doc_offset = bson_data(*mut) - bson_data(*parent);
        // Resize, and get the new position:
        position = _bson_splice_region(parent, position, n_delete, n_insert, insert_from);
        if (!position) {
            // Allocation failed
            return NULL;
        }
        // Adjust our document begin pointer, since we may have reallocated:
        mut->_bson_document_data = bson_mut_data(*parent) + my_doc_offset;
    } else {
        bson_doc*              doc          = mut->_doc;
        const bson_byte* const doc_data_end = bson_data(*doc) + bson_ssize(*doc);
        const uint32_t         avail_to_delete
            = (uint32_t)mlibMathNonNegativeInt32(I(doc_data_end - position));
        mlib_math_catch (_unused) {
            (void)_unused;
            return NULL;
        }
        if (n_delete > avail_to_delete) {
            // Not enough data to actual delete, so this splice request is bogus?
            return NULL;
        }
        // We are the root of the document, so we do the actual work:
        if (new_doc_size > bson_mut_capacity(*mut)) {
            // We need to grow larger. Add some extra to prevent repeated
            // allocations:
            const uint32_t new_capacity
                = (uint32_t)mlibMathNonNegativeInt32(add(I(new_doc_size), 512));
            mlib_math_catch (_unused) {
                (void)_unused;
                return NULL;
            }
            // Resize:
            if (bson_doc_reserve(doc, (uint32_t)new_capacity) < 0) {
                // Allocation failed...
                return NULL;
            }
            mut->_bson_document_data = bson_mut_data(*doc);
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
        mlib_math_catch (_unused) {
            (void)_unused;
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
    _bson_write_u32le(bson_mut_data(*mut), new_doc_size);
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
                                            mlib_str_view        key,
                                            const uint32_t       datasize) mlib_noexcept {
    // Prevent embedded nulls within document keys:
    key = mlib_str_view_chopnulls(key);
    // The total size of the element (add two: for the tag and the key's null):
    mlib_math_try();
    const uint32_t elem_size
        = (uint32_t)mlibMathNonNegativeInt32(add(U(key.len), add(2, U(datasize))));
    mlib_math_catch (_unused) {
        (void)_unused;
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
inline bson_iterator _bson_insert_stringlike(bson_mut*     doc,
                                             bson_iterator pos,
                                             mlib_str_view key,
                                             bson_type     realtype,
                                             mlib_str_view string) mlib_noexcept {
    mlib_math_try();
    const uint32_t string_size = (uint32_t)mlibMathPositiveInt32(add(U(string.len), 1));
    const uint32_t elem_size   = (uint32_t)mlibMathPositiveInt32(add(I(string_size), 4));
    mlib_math_catch (_unused) {
        (void)_unused;
        return bson_end(*doc);
    }
    bson_byte* out = _bson_prep_element_region(doc, &pos, realtype, key, elem_size);
    if (out) {
        out    = _bson_write_u32le(out, string_size);
        out    = _bson_memcpy_chr(out, string.data, string.len);
        out->v = 0;
    }
    return pos;
}

// d888888b d8b   db .d8888. d88888b d8888b. d888888b d888888b d8b   db  d888b
//   `88'   888o  88 88'  YP 88'     88  `8D `~~88~~'   `88'   888o  88 88' Y8b
//    88    88V8o 88 `8bo.   88ooooo 88oobY'    88       88    88V8o 88 88
//    88    88 V8o88   `Y8b. 88~~~~~ 88`8b      88       88    88 V8o88 88  ooo
//   .88.   88  V888 db   8D 88.     88 `88.    88      .88.   88  V888 88. ~8~
// Y888888P VP   V8P `8888Y' Y88888P 88   YD    YP    Y888888P VP   V8P  Y888P

inline bson_iterator _bson_insert_value(bson_mut*      doc,
                                        bson_iterator  pos,
                                        mlib_str_view  key,
                                        bson_value_ref val) mlib_noexcept;

inline bson_iterator bson_insert_code_with_scope(bson_mut*      doc,
                                                 bson_iterator  pos,
                                                 mlib_str_view  key,
                                                 bson_code_view code,
                                                 bson_view      scope) mlib_noexcept {
    mlib_math_try();
    const uint32_t code_size = (uint32_t)mlibMathPositiveInt32(add(1, U(code.utf8.len)));
    const uint32_t elem_size
        = (uint32_t)mlibMathPositiveInt32(add(I(code_size), add(I(bson_size(scope)), 4)));
    mlib_math_catch (_unused) {
        (void)_unused;
        return bson_end(*doc);
    }
    bson_byte* out
        = _bson_prep_element_region(doc, &pos, bson_type_codewscope, key, (uint32_t)elem_size);
    if (out) {
        out = _bson_write_u32le(out, elem_size);
        out = _bson_write_u32le(out, code_size);
        out = _bson_memcpy_chr(out, code.utf8.data, code.utf8.len);
        out = _bson_memcpy(out, bson_data(scope), bson_size(scope));
    }
    return pos;
}

/**
 * @brief Insert a value into a mutable document.
 *
 * Callable as:
 *
 * - `bson_insert(bson_mut, string-like, value-like)`
 * - `bson_insert(bson_mut, bson_iterator, string-like, value-like)`
 */
#define bson_insert(...) MLIB_ARGC_PICK(_bsonInsert, __VA_ARGS__)
#define _bsonInsert_argc_3(Mut, Key, Value) _bsonInsertAt(Mut, bson_end(*(Mut)), (Key), Value)
#define _bsonInsert_argc_4(Mut, Pos, Key, Value) _bsonInsertAt(Mut, (Pos), (Key), Value)
#define _bsonInsertAt(Mut, Position, Key, Value)                                                   \
    _bson_insert_value((Mut), (Position), mlib_str_view_from(Key), bson_value_ref_from(Value))

/**
 * @brief Replace the key string of an element within a document
 *
 * @param doc The document to update
 * @param pos The element that will be updated
 * @param newkey The new key string for the element.
 * @return bson_iterator The iterator referring to the element after being
 * updated, or the end iterator in case of allocation failure
 */
#define bson_set_key(Doc, Pos, Key) _bson_set_key((Doc), (Pos), mlib_str_view_from(Key))
inline bson_iterator
_bson_set_key(bson_mut* doc, bson_iterator pos, mlib_str_view newkey) mlib_noexcept {
    mlib_math_try();
    BV_ASSERT(!bson_stop(pos));
    // Truncate the key to not contain any null bytes:
    newkey = mlib_str_view_chopnulls(newkey);
    // The current key:
    const mlib_str_view curkey = bson_key(pos);
    // The number of bytes added for the new key (or possible negative)
    const ptrdiff_t size_diff = (ptrdiff_t)newkey.len - (ptrdiff_t)curkey.len;
    // The new remaining len following the adjusted key size
    const int32_t new_rlen = mlibMathPositiveInt32(add(I(pos._rlen), I(size_diff)));
    mlib_math_catch (_unused) {
        (void)_unused;
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
    pos._keylen = (int32_t)newkey.len;
    pos._rlen   = new_rlen;
    return pos;
}

/// Write the decimal representation of a uint32_t into the given string.
mlib_constexpr char* _bson_write_uint(uint32_t v, char* at) mlib_noexcept {
    if (v == 0) {
        *at++ = '0';
    } else if (v >= 10) {
        if (v < 100) {
            *at++ = (char)('0' + (v / 10ull));
        } else {
            at = _bson_write_uint(v / 10ull, at);
        }
        *at++ = (char)('0' + (v % 10ull));
    } else {
        *at++ = (char)('0' + v);
    }
    *at = 0;
    return at;
}

/**
 * @brief A simple type used to store a small string representing the the
 * integer keys of array elements.
 */
struct bson_u32_string {
    char buf[11];
};

/**
 * @brief Generate a small string that contains the decimal spelling of the
 * given uint32_t value
 */
mlib_constexpr struct bson_u32_string bson_u32_string_create(uint32_t val) mlib_noexcept {
    struct bson_u32_string arr = {0};
    _bson_write_uint(val, arr.buf);
    return arr;
}

/**
 * @brief Relabel the element keys in a document beginning at `pos` with index `idx`
 */
inline bson_iterator
bson_relabel_array_elements_at(bson_mut* doc, bson_iterator pos, uint32_t idx) mlib_noexcept {
    ptrdiff_t it_offset = bson_iterator_data(pos) - bson_data(*doc);
    for (; !bson_stop(pos); pos = bson_next(pos)) {
        struct bson_u32_string key = bson_u32_string_create(idx);
        pos                        = bson_set_key(doc, pos, mlib_str_view_from(key.buf));
    }
    return _bson_recover_iterator(bson_data(*doc), it_offset);
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
    const int32_t new_rlen = mlibMathPositiveInt32(add(I(pos._rlen), I(size_diff)));
    mlib_math_catch (_unused) {
        (void)_unused;
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
inline bson_iterator bson_erase_range(bson_mut* const     doc,
                                      const bson_iterator first,
                                      const bson_iterator last) mlib_noexcept {
    return bson_splice_disjoint_ranges(doc, first, last, last, last);
}

inline bson_iterator bson_erase_one(bson_mut* const doc, const bson_iterator pos) mlib_noexcept {
    return bson_erase_range(doc, pos, bson_next(pos));
}

#define bson_erase(...) MLIB_ARGC_PICK(_bsonErase, __VA_ARGS__)
#define _bsonErase_argc_2(Doc, Pos) bson_erase_one((Doc), (Pos))
#define _bsonErase_argc_3(Doc, First, Last) bson_erase_range((Doc), (First), (Last))

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
inline bson_mut bson_mut_child(bson_mut* parent, bson_iterator subdoc_iter) mlib_noexcept {
    bson_mut ret = BSON_MUT_v2_NULL;
    if (bson_iterator_type(subdoc_iter) != bson_type_document
        && bson_iterator_type(subdoc_iter) != bson_type_array) {
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
    ret._offset_within_parent_data = (uint32_t)elem_offset;
    return ret;
}

/**
 * @brief Given a bson_mut that is a child of another bson_mut, obtain an
 * iterator within the parent that refers to the child document.
 *
 * @param doc A child document mutator
 * @return bson_iterator An iterator within the parent that refers to the child
 */
inline bson_iterator bson_mut_parent_iterator(bson_mut doc) mlib_noexcept {
    BV_ASSERT(doc._offset_within_parent_data > 0);
    bson_mut      par = *doc._parent_mut;
    bson_iterator ret = BSON_ITERATOR_NULL;
    // Recover the address of the element:
    ret._ptr         = bson_mut_data(par) + doc._offset_within_parent_data;
    ptrdiff_t offset = ret._ptr - bson_mut_data(par);
    ret._keylen      = (int32_t)mlibMath(assertNot(mlib_integer_allbits,
                                              sub(I(doc._bson_document_data - ret._ptr), 2)))
                      .i64;
    ret._rlen = (int32_t)(bson_size(par) - offset);
    return ret;
}

mlib_extern_c_end();

#if mlib_is_cxx()

namespace bson {
class mutator {
public:
    explicit mutator(bson_mut m) noexcept
        : _mut(m) {}

    explicit mutator(bson_doc& doc) noexcept
        : _mut(::bson_mutate(&doc)) {}

    explicit mutator(document& doc) noexcept
        : mutator(doc.get()) {}

    using iterator = ::bson_iterator;

    iterator begin() const noexcept { return bson_begin(_mut); }
    iterator end() const noexcept { return bson_end(_mut); }

    iterator find(std::string_view key) const noexcept { return view(*this).find(key); }

    operator view() const noexcept { return view::from_data(data(), byte_size()); }

    bson_byte*    data() const noexcept { return bson_mut_data(_mut); }
    std::uint32_t byte_size() const noexcept { return bson_size(_mut); }

    bool empty() const noexcept { return byte_size() == 5; }

    // Obtain a reference to the wrapped C API struct
    bson_mut&       get() noexcept { return _mut; }
    const bson_mut& get() const noexcept { return _mut; }

private:
    ::bson_mut _mut;

    template <typename V>
    auto _do_emplace(iterator      pos,
                     mlib_str_view key,
                     V value) noexcept -> decltype(::bson_insert(&_mut, pos, key, value)) {
        return ::bson_insert(&_mut, pos, key, value);
    }

    iterator _do_emplace(iterator pos, mlib_str_view, bson_eod) noexcept {
        // Attempting to emplace a bson_eod is a no-op
        return pos;
    }

    template <typename V>
    auto _emplace(iterator         pos,
                  std::string_view key_,
                  V const&         val) -> decltype(_do_emplace(pos, mlib_str_view{}, val)) {
        const mlib_str_view key = mlib_str_view::from(key_);
        const iterator      ret = _do_emplace(pos, key, val);
        if (ret == end()) {
            // This will only occur if there was an allocation failure
            throw std::bad_alloc();
        }
        return ret;
    }

public:
    struct inserted_subdocument;
    template <typename P>
    auto insert(iterator pos,
                P const& pair)  //
        -> decltype(_emplace(pos, std::get<0>(pair), std::get<1>(pair))) {
        return _emplace(pos, std::get<0>(pair), std::get<1>(pair));
    }

    iterator insert(iterator pos, bson_iterator::reference const& pair) {
        return _emplace(pos, pair.key(), pair.value());
    }

    template <typename V>
    auto
    emplace(iterator pos, std::string_view key, V const& val) -> decltype(_emplace(pos, key, val)) {
        return _emplace(pos, key, val);
    }

    template <typename P>
    auto push_back(P const& pair) -> decltype(insert(end(), pair)) {
        return insert(end(), pair);
    }

    template <typename V>
    auto emplace_back(std::string_view key, V const& val) -> decltype(emplace(end(), key, val)) {
        return emplace(end(), key, val);
    }

    inline inserted_subdocument insert_subdoc(iterator pos, std::string_view key);
    inline inserted_subdocument insert_array(iterator pos, std::string_view key);

    inline mutator push_subdoc(std::string_view key);
    inline mutator push_array(std::string_view key);

    iterator erase(iterator pos) noexcept { return ::bson_erase(&_mut, pos); }

    [[nodiscard]] mutator child(iterator pos) noexcept {
        return mutator(::bson_mut_child(&_mut, pos));
    }

    [[nodiscard]] iterator parent_iterator() const noexcept {
        return ::bson_mut_parent_iterator(_mut);
    }
};

struct mutator::inserted_subdocument {
    iterator position;
    mutator  mut;
};

mutator::inserted_subdocument mutator::insert_subdoc(iterator pos, std::string_view key) {
    auto it = ::bson_insert(&_mut, pos, key, bson_view{});
    return {it, child(it)};
}

mutator::inserted_subdocument mutator::insert_array(iterator pos, std::string_view key) {
    auto it = ::bson_insert(&_mut, pos, key, bson_array_view{});
    return {it, child(it)};
}

mutator mutator::push_subdoc(std::string_view key) { return insert_subdoc(end(), key).mut; }
mutator mutator::push_array(std::string_view key) { return insert_array(end(), key).mut; }

}  // namespace bson

#endif  // C++

mlib_extern_c_begin();

inline bson_iterator _bson_insert_value(bson_mut*      doc,
                                        bson_iterator  pos,
                                        mlib_str_view  key,
                                        bson_value_ref val) mlib_noexcept {
    switch (val.type) {
    case bson_type_eod:
        return pos;
    case bson_type_double: {
        bson_byte* out
            = _bson_prep_element_region(doc, &pos, bson_type_double, key, sizeof(double));
        if (out) {
            uint64_t tmp;
            memcpy(&tmp, &val.double_, sizeof val.double_);
            _bson_write_u64le(out, tmp);
        }
        return pos;
    }
    case bson_type_utf8:
        return _bson_insert_stringlike(doc, pos, key, bson_type_utf8, val.utf8);
    case bson_type_document: {
        if (!bson_data(val.document)) {
            // There was no document given. Re-call ourself with a view of an empty
            // doc:
            const bson_byte empty_doc[5] = {{5}};
            return _bson_insert_value(doc,
                                      pos,
                                      key,
                                      bson_value_ref_from(
                                          bson_view_from_data(empty_doc, sizeof empty_doc, NULL)));
        }
        // We have a document to insert:
        const uint32_t insert_size = bson_size(val.document);
        bson_byte* out = _bson_prep_element_region(doc, &pos, bson_type_document, key, insert_size);
        if (out) {
            // Copy the document into place:
            _bson_memcpy(out, bson_data(val.document), bson_size(val.document));
        }
        return pos;
    }
    case bson_type_array: {
        if (!bson_data(val.array)) {
            // There was no document given. Re-call ourself with a view of an empty
            // doc:
            const bson_byte empty_doc[5] = {{5}};
            bson_array_view arr          = {empty_doc};
            return _bson_insert_value(doc, pos, key, bson_value_ref_from(arr));
        }
        // We have a document to insert:
        const uint32_t insert_size = bson_size(val.array);
        bson_byte* out = _bson_prep_element_region(doc, &pos, bson_type_array, key, insert_size);
        if (out) {
            // Copy the document into place:
            _bson_memcpy(out, bson_data(val.array), bson_size(val.array));
        }
        return pos;
    }
    case bson_type_binary: {
        mlib_math_try();
        const uint32_t bin_size  = (uint32_t)mlibMathNonNegativeInt32(I(val.binary.data_len));
        const uint32_t elem_size = (uint32_t)mlibMathPositiveInt32(add(I(bin_size), 5));
        mlib_math_catch (_unused) {
            (void)_unused;
            return bson_end(*doc);
        }
        bson_byte* out = _bson_prep_element_region(doc, &pos, bson_type_binary, key, elem_size);
        if (out) {
            out      = _bson_write_u32le(out, bin_size);
            out[0].v = val.binary.subtype;
            ++out;
            out = _bson_memcpy(out, val.binary.data, bin_size);
        }
        return pos;
    }
    case bson_type_undefined:
        _bson_prep_element_region(doc, &pos, bson_type_undefined, key, 0);
        return pos;
    case bson_type_oid: {
        bson_byte* out = _bson_prep_element_region(doc, &pos, bson_type_oid, key, sizeof val.oid);
        if (out) {
            memcpy(out, &val.oid, sizeof val.oid);
        }
        return pos;
    }
    case bson_type_bool: {
        bson_byte* out = _bson_prep_element_region(doc, &pos, bson_type_bool, key, 1);
        if (out) {
            out[0].v = val.bool_ ? 1 : 0;
        }
        return pos;
    }
    case bson_type_datetime: {
        bson_byte* out
            = _bson_prep_element_region(doc, &pos, bson_type_datetime, key, sizeof val.datetime);
        if (out) {
            _bson_write_u64le(out, (uint64_t)val.datetime.utc_ms_offset);
        }
        return pos;
    }
    case bson_type_null:
        _bson_prep_element_region(doc, &pos, bson_type_null, key, 0);
        return pos;
    case bson_type_regex: {
        mlib_math_try();
        bson_regex_view rx = val.regex;
        // Neither regex nor options may contain embedded nulls, so we cannot trust
        // the caller giving us the correct length:
        const mlib_str_view trimmed_rx  = mlib_str_view_chopnulls(rx.regex);
        const mlib_str_view trimmed_opt = mlib_str_view_chopnulls(rx.options);
        // The total value size:
        const int32_t size = mlibMathNonNegativeInt32(
            checkMin(2, add(U(trimmed_rx.len), add(U(trimmed_opt.len), 5))));
        mlib_math_catch (_unused) {
            (void)_unused;
            return bson_end(*doc);
        }
        bson_byte* out = _bson_prep_element_region(doc, &pos, bson_type_regex, key, (uint32_t)size);
        if (out) {
            out        = _bson_memcpy_chr(out, rx.regex.data, rx.regex.len);
            (out++)->v = 0;
            if (rx.options.data) {
                out = _bson_memcpy_chr(out, rx.options.data, rx.options.len);
            }
            (out++)->v = 0;
        }
        return pos;
    }
    case bson_type_dbpointer: {
        mlib_math_try();
        bson_dbpointer_view dbp                  = val.dbpointer;
        const uint32_t      collname_string_size = (uint32_t)mlibMathPositiveInt32(
            add(strnlen(dbp.collection.data, U(dbp.collection.len)), 1));
        const uint32_t el_size
            = (uint32_t)mlibMathNonNegativeInt32(add(I(collname_string_size), I(12 + 4)));
        mlib_math_catch (_unused) {
            (void)_unused;
            return bson_end(*doc);
        }
        bson_byte* out
            = _bson_prep_element_region(doc, &pos, bson_type_dbpointer, key, (uint32_t)el_size);
        if (out) {
            out        = _bson_write_u32le(out, collname_string_size);
            out        = _bson_memcpy_chr(out, dbp.collection.data, collname_string_size - 1);
            (out++)->v = 0;
            out        = _bson_memcpy_u8(out, dbp.object_id.bytes, sizeof dbp.object_id);
        }
        return pos;
    }
    case bson_type_code:
        return _bson_insert_stringlike(doc, pos, key, bson_type_code, val.code.utf8);
    case bson_type_symbol:
        return _bson_insert_stringlike(doc, pos, key, bson_type_symbol, val.symbol.utf8);
    case bson_type_codewscope:
        // TODO
        assert(false && "TODO: Insert code_w_scope");
        abort();
    case bson_type_int32: {
        bson_byte* out
            = _bson_prep_element_region(doc, &pos, bson_type_int32, key, sizeof(int32_t));
        if (out) {
            out = _bson_write_u32le(out, (uint32_t)val.int32);
        }
        return pos;
    }
    case bson_type_timestamp: {
        bson_byte* out
            = _bson_prep_element_region(doc, &pos, bson_type_timestamp, key, sizeof val.timestamp);
        if (out) {
            out = _bson_write_u32le(out, val.timestamp.increment);
            out = _bson_write_u32le(out, val.timestamp.utc_sec_offset);
        }
        return pos;
    }
    case bson_type_int64: {
        bson_byte* out
            = _bson_prep_element_region(doc, &pos, bson_type_int64, key, sizeof val.int64);
        if (out) {
            out = _bson_write_u64le(out, (uint64_t)val.int64);
        }
        return pos;
    }
    case bson_type_decimal128: {
        bson_byte* out = _bson_prep_element_region(doc,
                                                   &pos,
                                                   bson_type_decimal128,
                                                   key,
                                                   sizeof val.decimal128);
        if (out) {
            out = _bson_memcpy_u8(out, val.decimal128.bytes, sizeof val.decimal128);
        }
        return pos;
    }
    case bson_type_maxkey:
        _bson_prep_element_region(doc, &pos, bson_type_maxkey, key, 0);
        return pos;
    case bson_type_minkey:
        _bson_prep_element_region(doc, &pos, bson_type_minkey, key, 0);
        return pos;
    }
    return pos;
}

mlib_extern_c_end();
