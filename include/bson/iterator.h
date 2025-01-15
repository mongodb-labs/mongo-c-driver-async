#pragma once

#include <bson/byte.h>
#include <bson/detail/assert.h>
#include <bson/detail/iter.h>
#include <bson/detail/mem.h>
#include <bson/iter_errc.h>
#include <bson/types.h>
#include <bson/value_ref.h>
#include <bson/view.h>

#include <mlib/config.h>
#include <mlib/integer.h>
#include <mlib/str.h>

#include <math.h>

#if mlib_is_cxx()
#include <iterator>
#include <optional>
#include <tuple>
#endif

/**
 * @brief A type that allows inspecting the elements of a BSON object or array.
 */
typedef struct bson_iterator {
    /// @private A pointer to the element referred-to by this iterator.
    const bson_byte* _ptr;
    /// @private The length of the key string, in bytes, not including the nul
    int32_t _keylen;
    /// @private The number of bytes remaining in the document, or an error code.
    int32_t _rlen;

#if mlib_is_cxx()
    // Proxy reference type for this iterator
    class reference;
    // Arrow helper, for `operator->()`
    class arrow;
    using difference_type   = std::int32_t;
    using size_type         = std::uint32_t;
    using value_type        = reference;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept  = std::forward_iterator_tag;
    using pointer           = bson_iterator;

    // Compare two iterators for equality
    inline bool operator==(const bson_iterator other) const noexcept;
    bool operator!=(const bson_iterator& other) const noexcept { return not(*this == other); }

    // Get the proxy reference
    inline reference operator*() const;

    // Preincrement
    inline bson_iterator& operator++() noexcept;
    // Postincrement
    bson_iterator operator++(int) noexcept {
        auto c = *this;
        ++*this;
        return c;
    }

    // Obtain a reference in-situ via `arrow`
    inline arrow operator->() const noexcept;

    // Obtain the error associated with this iterator, if one is defined
    [[nodiscard]] inline bson_iter_errc error() const noexcept;

    // Test whether the iterator has an errant state
    [[nodiscard]] bool has_error() const noexcept { return error() != bson_iter_errc_okay; }
    // Test whether the iterator points to a valid value (is not done and not an error)
    [[nodiscard]] bool has_value() const noexcept { return not has_error() and not stop(); }

    // Test whether the iterator is the "done" iterator
    [[nodiscard]] inline bool stop() const noexcept;

    // If the iterator has an error condition, throw an exception
    inline void throw_if_error() const;

    // Obtain a pointer to the data referred-to by the iterator
    [[nodiscard]] inline const bson_byte* data() const noexcept { return _ptr; }

    // Obtain the size (in bytes) of the pointed-to data
    [[nodiscard]] inline size_type data_size() const noexcept;
#endif  // C++
} bson_iterator;

#define BSON_ITERATOR_NULL (mlib_init(bson_iterator){NULL, 0, 0})

mlib_extern_c_begin();

/// @internal Generate a bson_iterator that encodes an error during iteration
inline bson_iterator _bson_iterator_error(enum bson_iter_errc err) mlib_noexcept {
    return mlib_init(bson_iterator){._ptr = NULL, ._keylen = 0, ._rlen = -((int32_t)err)};
}

/**
 * @internal
 * @brief Obtain an iterator pointing to the given 'data', which must be the
 * beginning of a document element, or to the null terminator at the end of the
 * BSON document.
 *
 * This is guaranteed to return a valid element iterator, a past-the-end
 * iterator, or to an errant iterator that indicates an error. This function
 * validates that the pointed-to element does not have a type+value that would
 * overrun the buffer specified by `data` and `maxlen`.
 *
 * @param data A pointer to the type tag that starts a document element, or to
 * the null-terminator at the end of a document.
 * @param bytes_remaining The number of bytes that are available following
 * `data` before overrunning the document. `data + bytes_remaining` will be the
 * past-the-end byte of the document.
 *
 * @return bson_iterator A new iterator, which may be stopped or errant.
 *
 * @note This function is not a public API, but is defined as an extern inline
 * for the aide of alias analysis, DCE, and CSE. It has an inline proof that it
 * will not overrun the buffer defined by `data` and `maxlen`.
 */
inline bson_iterator _bson_iterator_at(const bson_byte* const data, int32_t maxlen) mlib_noexcept {
    // Assert: maxlen > 0
    BV_ASSERT(maxlen > 0);
    // Define:
    const int last_index = maxlen - 1;
    //? Prove: last_index >= 0
    // Given : (N > M) ⋀ (N > 0) → (N - 1 >= M)
    // Derive: (last_index = maxlen - 1)
    //       ⋀ (maxlen > 0)
    //*      ⊢ last_index >= 0
    BV_ASSERT(last_index >= 0);
    // Assume: data[last_index] = 0
    BV_ASSERT(data[last_index].v == 0);

    /// The type of the next data element
    const bson_type type = (bson_type)data[0].v;

    if (BV_UNLIKELY(maxlen == 1)) {
        // There's only the last byte remaining. Creation of the original
        // bson_view validated that the data ended with a nullbyte, so we may
        // assume that the only remaining byte is a nul.
        BV_ASSERT(data[last_index].v == 0);
        // This "zero" type is actually the null terminator at the end of the
        // document.
        return mlib_init(bson_iterator){
            ._ptr    = data,
            ._keylen = 0,
            ._rlen   = 1,
        };
    }

    //? Prove: maxlen > 1
    // Assume: maxlen != 1
    // Derive: maxlen > 0
    //       ⋀ maxlen != 1
    //*      ⊢ maxlen > 1

    //? Prove: maxlen - 1 >= 1
    // Derive: maxlen > 1
    //       ⋀ ((N > M) ⋀ (N > 0) → (N - 1 >= M))
    //*      ⊢ maxlen - 1 >= 1

    // Define:
    const int key_maxlen = maxlen - 1;

    //? Prove: key_maxlen >= 1
    // Given : key_maxlen = maxlen - 1
    //       ⋀ maxlen - 1 >= 1
    //*      ⊢ key_maxlen >= 1
    BV_ASSERT(key_maxlen >= 1);

    //? Prove: key_maxlen = last_index
    // Subst : key_maxlen = maxlen - 1
    //       ⋀ last_index = maxlen - 1
    //*      ⊢ key_maxlen = last_index

    // Define: ∀ (n : n >= 0 ⋀ n < key_maxlen) . keyptr[n] = data[n+1];
    const char* const keyptr = (const char*)data + 1;

    // Define: KeyLastIndex = last_index - 1

    //? Prove: keyptr[KeyLastIndex] = data[last_index]
    // Derive: keyptr[n] = data[n+1]
    //       ⊢ keyptr[last_index-1] = data[(last_index-1)+1]
    // Simplf: keyptr[last_index-1] = data[last_index]
    // Subst : KeyLastIndex = last_index - 1
    //*      ⊢ keyptr[KeyLastIndex] = data[last_index]

    //? Prove: keyptr[KeyLastIndex] = 0
    // Derive: data[last_index] = 0
    //       ⋀ keyptr[KeyLastIndex] = data[last_index]
    //*      ⊢ keyptr[KeyLastIndex] = 0

    /*
    Assume: The guarantees of R = strnlen(S, M):

       # HasNul(S, M) = If there exists a null byte in S[0..M):
       HasNul(S, M) ↔ (∃ (n : n >= 0 ⋀ n < M) . S[n] = 0)

       # If HasNull(S, M), then R = strlen(S, M) guarantees:
         HasNull(S, M) → R >= 0
                       ⋀ R < M
                       ⋀ S[R] = 0

       # If not HasNull(S, M), then R = strlen(S, M) guarantees:
       ¬ HasNull(S, M) → R = M
    */

    //? Prove: KeyLastIndex < key_maxlen
    // Given : N - 1 < N
    // Derive: KeyLastIndex = key_maxlen - 1
    //       ⋀ (key_maxlen - 1) < key_maxlen
    //*      ⊢ (KeyLastIndex < key_maxlen)

    //? Prove: HasNul(keyptr, key_maxlen)
    // Derive: (KeyLastIndex < key_maxlen)
    //       ⋀ (keyptr[KeyLastIndex] = 0)
    //       ⋀ key_maxlen >= 1
    //*      ⊢ HasNul(keyptr, key_maxlen)

    // Define:
    const size_t keylen_ = strlen(keyptr);
    BV_ASSERT(keylen_ < INT32_MAX);
    const int32_t keylen = (int32_t)keylen_;

    // Derive: HasNul(keyptr, key_maxlen)
    //       ⋀ R.keylen = strnlen(S.keyptr, M.key_maxlen)
    //       ⋀ HasNul(S, M) -> (R >= 0) ⋀ (R < M) ⋀ (S[R] = 0)
    //*      ⊢ (keylen >= 0)
    //*      ⋀ (keylen < key_maxlen)
    //*      ⋀ (keyptr[keylen] = 0)
    BV_ASSERT(keylen >= 0);
    BV_ASSERT(keylen < key_maxlen);
    BV_ASSERT(keyptr[keylen] == 0);

    // Define:
    const int32_t p1 = key_maxlen - keylen;
    // Given : (M < N) → (N - M) > 0
    // Derive: (keylen < key_maxlen)
    //       ⋀ ((M < N) → (N - M) > 0)
    //       ⊢ (key_maxlen - keylen) > 0
    // Derive: (p1 = key_maxlen - keylen)
    //       ⋀ ((key_maxlen - keylen) > 0)
    //       ⊢ p1 > 0
    BV_ASSERT(p1 > 0);
    // Define: ValMaxLen = p1 - 1
    const int32_t val_maxlen = p1 - 1;
    // Given : (N > M) ⋀ (N > 0) → (N - 1 >= M)
    // Derive: (p1 > 0)
    //       ⋀ ((N > M) ⋀ (N > 0) → (N - 1 >= M))
    //       ⊢ (p1-1 >= 0)
    // Derive: ValMaxLen = p1 - 1
    //       ⋀ (p1-1 >= 0)
    //       ⊢ ValMaxLen >= 0
    BV_ASSERT(val_maxlen >= 0);

    if (BV_UNLIKELY(val_maxlen < 1)) {
        // We require at least one byte for the document's own nul terminator
        return _bson_iterator_error(bson_iter_errc_short_read);
    }

    // Assume: ValMaxLen > 0
    BV_ASSERT(val_maxlen > 0);

    bool need_check_size = true;
    if (val_maxlen > 16) {
        // val_maxlen is larger than all fixed-sized BSON value objects. If we are
        // parsing a fixed-size object, we can skip size checking
        need_check_size = !(
            type == bson_type_null || type == bson_type_undefined || type == bson_type_timestamp
            || type == bson_type_datetime || type == bson_type_double || type == bson_type_bool
            || type == bson_type_decimal128 || type == bson_type_int32 || type == bson_type_int64);
    }

    if (need_check_size) {
        const char* const valptr = keyptr + (keylen + 1);

        const int32_t vallen = _bson_valsize(type, (const bson_byte*)valptr, val_maxlen);
        if (BV_UNLIKELY(vallen < 0)) {
            return _bson_iterator_error((enum bson_iter_errc)(-vallen));
        }
        // Check that a subdocument has a null terminator
        if (type == bson_type_document || type == bson_type_array) {
            if (BV_UNLIKELY(valptr[vallen - 1])) {
                // The final byte in a subdocument/array should be null
                return _bson_iterator_error(bson_iter_errc_invalid_document);
            }
        }
    }
    return mlib_init(bson_iterator){
        ._ptr    = data,
        ._keylen = keylen,
        ._rlen   = maxlen,
    };
}

/**
 * @internal
 * @brief Recover an iterator within a document's data based on the byte offset of the desired
 * element
 *
 * @param doc_data_begin Pointer to the document header
 * @param elem_offset The byte offset of the element being requested. May point to the null
 * terminator to recover the end iterator
 */
inline bson_iterator _bson_recover_iterator(const bson_byte* doc_data_begin,
                                            ptrdiff_t        elem_offset) mlib_noexcept {
    BV_ASSERT(elem_offset >= 4);
    int32_t len = (int32_t)_bson_read_u32le(doc_data_begin);
    BV_ASSERT(elem_offset < len);
    return _bson_iterator_at(doc_data_begin + elem_offset, len - (int32_t)elem_offset);
}

/**
 * @brief Obtain a bson_iterator referring to the first position within `v`.
 */
#define bson_begin(...) _bson_begin(bson_data(__VA_ARGS__))
inline bson_iterator _bson_begin(const bson_byte* data) mlib_noexcept {
    BV_ASSERT(data);
    const int32_t   sz       = (int32_t)_bson_read_u32le(data);
    const ptrdiff_t tailsize = sz - (ptrdiff_t)sizeof(int32_t);
    BV_ASSERT(tailsize > 0);
    BV_ASSERT(tailsize < INT32_MAX);
    return _bson_iterator_at(data + sizeof(int32_t), (int32_t)tailsize);
}

/**
 * @brief Obtain a past-the-end "done" iterator for the given document `v`
 */
#define bson_end(...) _bson_end(bson_data(__VA_ARGS__))
inline bson_iterator _bson_end(const bson_byte* v) mlib_noexcept {
    BV_ASSERT(v);
    const uint32_t len = _bson_read_u32le(v);
    return _bson_iterator_at(v + len - 1, 1);
}

/**
 * @brief Determine whether two non-error iterators are equivalent (i.e. refer
 * to the same position in the same document)
 */
inline bool bson_iterator_eq(bson_iterator left, bson_iterator right) mlib_noexcept {
    return left._ptr == right._ptr;
}

/**
 * @brief Determine whether the given iterator is at the end OR has encountered
 * an error.
 */
inline bool bson_stop(bson_iterator it) mlib_noexcept { return it._rlen <= 1; }

/// @internal Obtain a pointer to the beginning of the referred-to element's
/// value
inline const bson_byte* _bson_iterator_value_ptr(bson_iterator iter) mlib_noexcept {
    return iter._ptr + 1 + iter._keylen + 1;
}

/**
 * @brief Obtain a pointer to the beginning of the element data in memory
 */
inline const bson_byte* bson_iterator_data(const bson_iterator it) mlib_noexcept { return it._ptr; }

/**
 * @brief Obtain the type of the BSON element referred-to by the given iterator
 *
 * @param it The iterator to inspect
 * @return bson_type The type of the referred-to element. Returns BSON_TYPE_EOD
 * if the iterator refers to the end of the document.
 *
 * @pre bson_iterator_get_error(it) == false
 */
inline bson_type bson_iterator_type(bson_iterator it) mlib_noexcept {
    BV_ASSERT(it._rlen > 0);
    return (bson_type)(it._ptr[0].v);
}

/**
 * @brief Obtain the byte-size of the pointed-to element
 *
 * @param it An iterator pointing to an element. Must not be an end/error
 * iterator
 * @return uint32_t The size of the element, in bytes. This includes the tag,
 * the key, and the value data.
 */
inline uint32_t bson_iterator_data_size(const bson_iterator it) mlib_noexcept {
    BV_ASSERT(!bson_stop(it));
    const int32_t val_offset = 1  // The type
        + it._keylen              // The key
        + 1;                      // The nul after the key
    BV_ASSERT(val_offset > 0);
    const int32_t valsize = _bson_valsize(bson_iterator_type(it), it._ptr + val_offset, INT32_MAX);
    const int32_t remain  = INT32_MAX - valsize;
    BV_ASSERT(val_offset < remain);
    return (uint32_t)(val_offset + valsize);
}

/**
 * @brief Obtain an iterator referring to the next position in the BSON document
 * after the given iterator's referred-to element.
 *
 * @param it A valid iterator to a bson `V` element. Must not be a
 * stopped/errored iterator (i.e. @ref bson_iterator_done must return `false`)
 * @return bson_iterator A new iterator `Next`.
 */
inline bson_iterator bson_next(const bson_iterator it) mlib_noexcept {
    const int32_t          skip   = (int32_t)bson_iterator_data_size(it);
    const bson_byte* const newptr = bson_iterator_data(it) + skip;
    const int32_t          remain = it._rlen - skip;
    BV_ASSERT(remain >= 0);
    return _bson_iterator_at(newptr, remain);
}

/**
 * @brief Obtain the error associated with the given iterator.
 */
inline enum bson_iter_errc bson_iterator_get_error(bson_iterator it) mlib_noexcept {
    return it._rlen < 0 ? (enum bson_iter_errc) - it._rlen : bson_iter_errc_okay;
}

/**
 * @brief Obtain a string view referring to the key string of the BSON
 * document element referred-to by the given iterator.
 */
inline mlib_str_view bson_key(bson_iterator it) mlib_noexcept {
    BV_ASSERT(it._rlen >= it._keylen + 1);
    BV_ASSERT(it._ptr);
    return mlib_str_view_data((const char*)it._ptr + 1, (uint32_t)it._keylen);
}

/**
 * @brief Compare a bson_iterator's key with a string
 */
#define bson_key_eq(Iterator, Key) _bson_key_eq((Iterator), mlib_str_view_from((Key)))
inline bool _bson_key_eq(const bson_iterator it, mlib_str_view key) mlib_noexcept {
    BV_ASSERT(it._keylen >= 0);
    if (key.len > INT32_MAX || (int32_t)key.len != it._keylen) {
        return false;
    }
    const bson_byte* it_key_ptr = it._ptr + 1;
    return memcmp(it_key_ptr, key.data, key.len) == 0;
}

#define bson_foreach(IterName, Viewable)                                                           \
    _bsonForEach(IterName, bson_view_from(Viewable), _bsonForEachOnlyOnce, _bsonForEachView)
#define bson_foreach_subrange(IterName, First, Last)                                               \
    _bsonForeachSubrange(IterName,                                                                 \
                         First,                                                                    \
                         Last,                                                                     \
                         _bsonSubrangeIterator,                                                    \
                         _bsonSubrangeEnd,                                                         \
                         _bsonSubrangeOuterOnce,                                                   \
                         _bsonSubrangeInnerOnce)
// clang-format off

#define _bsonForeachSubrange(IterName, FirstInit, LastInit, Iterator, Sentinel, OuterOnce, InnerOnce) \
   /* OuterOnce ensures the whole foreach operation only runs one time */ \
   for (int OuterOnce = 1; OuterOnce;) \
   /* Grab the sentinel once for the whole loop: */ \
   for (const bson_iterator Sentinel = (LastInit); OuterOnce; ) \
   /* Keep a 'DidBreak' variable that detects if the loop body executed a 'break' statement' */ \
   for (bool DidBreak = false; OuterOnce; \
        /* If we hit this loopstep statement, the whole foreach is finished: */ \
        OuterOnce = 0) \
   /* Create the actual iterator (whose name is private). */ \
   for (bson_iterator Iterator = (FirstInit); \
        /* Continually advance until we hit the sentinel, or 'DidBreak' is true */ \
        !DidBreak && !bson_iterator_eq(Iterator, Sentinel); \
        /* Check if the current iterator has an error condition */ \
        Iterator = bson_iterator_get_error(Iterator) == bson_iter_errc_okay \
            /* If not, advance to the next iterator. This *may* produce an errant iterator */ \
            ? bson_next(Iterator) \
            /* The current iterator is bad. Immediately jump to the end so we stop the loop */ \
            : Sentinel) \
   /* An inner-scope loop for a generating the visible name of the iterator. Also only runs once: */ \
   for (int InnerOnce = 1; InnerOnce;) \
   /* Declare the iterator variable that is visible in the loop body: */ \
   for (const bson_iterator IterName = Iterator; InnerOnce; InnerOnce = 0) \
   /* Set 'DidBreak' to true. If we 'continue' or hit the loop end, we immediately set it to false: */ \
   for (DidBreak = true; InnerOnce; DidBreak = false, InnerOnce = 0) \
   if (0) {} else

#define _bsonForEach(IterName, ViewInit, View, OuterOnce) \
   /* OuterOnce ensures the whole foreach operation only runs one time */ \
   for (int OuterOnce = 1; OuterOnce;) \
   /* Create the actual view from the initializing expression: */ \
   for (const bson_view View = ViewInit; OuterOnce;) \
   /* If we hit this loopstep statement, the whole foreach is finished: */ \
   for (; OuterOnce; OuterOnce = 0) \
   bson_foreach_subrange(IterName, bson_begin(View), bson_end(View))
// clang-format on

/**
 * @brief Find the first element within a document that has the given key
 */
#define bson_find(Doc, Key) _bson_find(bson_data((Doc)), mlib_str_view_from((Key)))
inline bson_iterator _bson_find(const bson_byte* v, mlib_str_view key) mlib_noexcept {
    bson_foreach_subrange(iter, _bson_begin(v), _bson_end(v)) {
        if (!bson_iterator_get_error(iter) && bson_key_eq(iter, key)) {
            return iter;
        }
    }
    return _bson_end(v);
}

inline mlib_str_view _bson_read_stringlike_at(const bson_byte* p) mlib_noexcept {
    const int32_t len = (int32_t)_bson_read_u32le(p);
    // String length checks were already performed by our callers
    BV_ASSERT(len >= 1);
    return mlib_str_view_data((const char*)(p + sizeof len), (uint32_t)len - 1);
}

inline mlib_str_view _bson_iterator_stringlike(bson_iterator it) mlib_noexcept {
    const bson_byte* after_key = _bson_iterator_value_ptr(it);
    mlib_str_view    r         = _bson_read_stringlike_at(after_key);
    BV_ASSERT(r.len > 0);
    BV_ASSERT(r.len < (size_t)it._rlen);
    return r;
}

/**
 * @brief Obtain a reference to the iterator's element's value.
 *
 * @param it A valid non-error iterator
 * @return inline
 */
inline bson_value_ref bson_iterator_value(bson_iterator const it) mlib_noexcept {
    BV_ASSERT(!bson_iterator_get_error(it));
    bson_value_ref ret;
    ret.type = bson_iterator_type(it);
    switch (bson_iterator_type(it)) {
    case bson_type_eod:
    case bson_type_undefined:
    case bson_type_null:
    case bson_type_maxkey:
    case bson_type_minkey:
        break;
    case bson_type_double: {
        uint64_t bytes = _bson_read_u64le(_bson_iterator_value_ptr(it));
        memcpy(&ret.double_, &bytes, sizeof bytes);
        break;
    }
    case bson_type_utf8:
        ret.utf8 = _bson_iterator_stringlike(it);
        break;
    case bson_type_array:
    case bson_type_document: {
        const bson_byte* const valptr     = _bson_iterator_value_ptr(it);
        const int64_t          val_offset = (valptr - it._ptr);
        const int64_t          val_remain = it._rlen - val_offset;
        BV_ASSERT(val_remain > 0);
        bson_view v = bson_view_from_data(valptr, (size_t)val_remain, NULL);
        if (bson_iterator_type(it) == bson_type_document) {
            ret.document = v;
        } else {
            ret.array._bson_document_data = bson_data(v);
        }
        break;
    }
    case bson_type_binary: {
        const bson_byte* const valptr = _bson_iterator_value_ptr(it);
        const int32_t          size   = (int32_t)_bson_read_u32le(valptr);
        BV_ASSERT(size >= 0);
        const uint8_t          subtype = valptr[4].v;
        const bson_byte* const p       = valptr + 5;
        ret.binary.data                = p;
        ret.binary.data_len            = (uint32_t)size;
        ret.binary.subtype             = subtype;
        break;
    }
    case bson_type_oid: {
        memcpy(&ret.oid, _bson_iterator_value_ptr(it), sizeof ret.oid);
        break;
    }
    case bson_type_bool:
        ret.bool_ = _bson_iterator_value_ptr(it)[0].v != 0;
        break;
    case bson_type_datetime:
        ret.datetime.utc_ms_offset = (int64_t)_bson_read_u64le(_bson_iterator_value_ptr(it));
        break;
    case bson_type_regex: {
        const bson_regex_view  null_regex = BSON_REGEX_NULL;
        const bson_byte* const p          = _bson_iterator_value_ptr(it);
        mlib_math_try();
        const char* const regex   = (const char*)p;
        const uint32_t    rx_len  = mlibMath(castUInt32(strlen32(regex)));
        const char* const options = regex + rx_len + 1;
        const uint32_t    opt_len = mlibMath(castUInt32(strlen32(options)));
        ret.regex.regex           = mlib_str_view_data(regex, rx_len);
        ret.regex.options         = mlib_str_view_data(options, opt_len);
        mlib_math_catch (_unused) {
            (void)_unused;
            ret.regex = null_regex;
        }
        break;
    }
    case bson_type_dbpointer: {
        const bson_byte* const p              = _bson_iterator_value_ptr(it);
        const int32_t          coll_name_size = (int32_t)_bson_read_u32le(p);
        BV_ASSERT(coll_name_size > 0);
        ret.dbpointer.collection
            = mlib_str_view_data((const char*)p + 4, (size_t)(coll_name_size - 1));
        memcpy(&ret.dbpointer.object_id.bytes, p + 4 + coll_name_size, sizeof(bson_oid));
        break;
    }
    case bson_type_code:
        ret.code.utf8 = _bson_iterator_stringlike(it);
        break;
    case bson_type_symbol:
        ret.symbol.utf8 = _bson_iterator_stringlike(it);
        break;
    case bson_type_codewscope:
        assert(false && "TODO: Code w/ scope");
        abort();
        break;
    case bson_type_int32:
        ret.int32 = (int32_t)_bson_read_u32le(_bson_iterator_value_ptr(it));
        break;
    case bson_type_timestamp:
        ret.timestamp.increment      = _bson_read_u32le(_bson_iterator_value_ptr(it));
        ret.timestamp.utc_sec_offset = _bson_read_u32le(_bson_iterator_value_ptr(it) + 4);
        break;
    case bson_type_int64:
        ret.int64 = (int64_t)_bson_read_u64le(_bson_iterator_value_ptr(it));
        break;
    case bson_type_decimal128:
        memcpy(&ret.decimal128, _bson_iterator_value_ptr(it), sizeof ret.decimal128);
        break;
    }
    return ret;
}

mlib_extern_c_end();

#if mlib_is_cxx()

namespace bson {

struct iterator_error : std::runtime_error {
    iterator_error() noexcept
        : std::runtime_error("Invalid element in BSON document data") {}

    [[nodiscard]] virtual bson_iter_errc cond() const noexcept = 0;
};

template <bson_iter_errc Cond>
struct iterator_error_of : iterator_error {
    [[nodiscard]] bson_iter_errc cond() const noexcept override { return Cond; }
};

}  // namespace bson

inline std::uint32_t bson_iterator::data_size() const noexcept {
    return bson_iterator_data_size(*this);
}

inline bool bson_iterator::operator==(bson_iterator other) const noexcept {
    return bson_iterator_eq(*this, other);
}

inline bson_iter_errc bson_iterator::error() const noexcept {
    return bson_iterator_get_error(*this);
}
inline void bson_iterator::throw_if_error() const {
    switch (this->error()) {
    case bson_iter_errc_okay:
        return;
#define X(Cond)                                                                                    \
    case Cond:                                                                                     \
        throw bson::iterator_error_of<Cond>()
        X(bson_iter_errc_short_read);
        X(bson_iter_errc_invalid_length);
        X(bson_iter_errc_invalid_type);
        X(bson_iter_errc_invalid_document);
#undef X
    }
}

inline bson_iterator bson_view::begin() const noexcept { return bson_begin(*this); }
inline bson_iterator bson_view::end() const noexcept { return bson_end(*this); }
inline bson_iterator bson_array_view::begin() const noexcept { return bson_begin(*this); }
inline bson_iterator bson_array_view::end() const noexcept { return bson_end(*this); }
inline bson_iterator bson_view::find(std::string_view key) const noexcept {
    return ::bson_find(*this, key);
}

class bson_iterator::reference {
    bson_iterator _iter;
    friend ::bson_iterator;

    inline explicit reference(bson_iterator it)
        : _iter(it) {}

public:
    [[nodiscard]] inline bson_type type() const noexcept { return bson_iterator_type(_iter); }

    [[nodiscard]] std::string_view key() const noexcept {
        return std::string_view(bson_key(_iter));
    }

    [[nodiscard]] ::bson_value_ref value() const noexcept { return ::bson_iterator_value(_iter); }

    bool operator==(const reference& other) const noexcept {
        return key() == other.key() and value() == other.value();
    }

#define TRY_AS(Type, Getter, TypeTag)                                                              \
    friend std::optional<Type> bson_value_try_convert(reference const& self, Type*) noexcept {     \
        if (self.type() == TypeTag) {                                                              \
            return Getter;                                                                         \
        }                                                                                          \
        return {};                                                                                 \
    }                                                                                              \
    static_assert(true, "")
    TRY_AS(double, self.value().double_, bson_type_double);
    TRY_AS(std::string_view, self.value().utf8, bson_type_utf8);
    TRY_AS(bson_array_view, self.value().array, bson_type_array);
    TRY_AS(bson_binary_view, self.value().binary, bson_type_binary);
    TRY_AS(bson::undefined, bson::undefined{}, bson_type_undefined);
    TRY_AS(bson_oid, self.value().oid, bson_type_oid);
    TRY_AS(bool, self.value().bool_, bson_type_bool);
    TRY_AS(bson_datetime, self.value().datetime, bson_type_datetime);
    TRY_AS(bson::null, bson::null{}, bson_type_null);
    TRY_AS(bson_regex_view, self.value().regex, bson_type_regex);
    TRY_AS(::bson_dbpointer_view, self.value().dbpointer, bson_type_dbpointer);
    TRY_AS(bson_code_view, self.value().code, bson_type_code);
    TRY_AS(bson_symbol_view, self.value().symbol, bson_type_symbol);
    // TRY_AS(std::string_view, self.value().code, BSON_TYPE_CODEWSCOPE); // TODO
    TRY_AS(std::int32_t, self.value().int32, bson_type_int32);
    TRY_AS(bson_timestamp, self.value().timestamp, bson_type_timestamp);
    TRY_AS(std::int64_t, self.value().int64, bson_type_int64);
    TRY_AS(bson_decimal128, self.value().decimal128, bson_type_decimal128);
    TRY_AS(bson::minkey, bson::minkey{}, bson_type_minkey);
    TRY_AS(bson::maxkey, bson::maxkey{}, bson_type_maxkey);
#undef TRY_AS

    friend std::optional<bson_view> bson_value_try_convert(reference const& self,
                                                           bson_view*) noexcept {
        if (self.type() != bson_type_document and self.type() != bson_type_array) {
            return {};
        }
        return self.value().document;
    }

    friend std::optional<bson_value_ref> bson_value_try_convert(reference const& self,
                                                                bson_value_ref*) noexcept {
        return self.value();
    }

    template <typename T>
    [[nodiscard]] inline std::optional<T> try_as() const noexcept {
        return bson_value_try_convert(*this, static_cast<T*>(nullptr));
    }

    template <std::size_t N>
    friend auto get(reference const& self) {
        if constexpr (N == 0) {
            return self.key();
        } else {
            return self.value();
        }
    }
};

class bson_iterator::arrow {
public:
    bson_iterator::reference _ref;

    inline const bson_iterator::reference* operator->() const noexcept { return &_ref; }
};

inline bson_iterator::reference bson_iterator::operator*() const {
    throw_if_error();
    return reference(*this);
}

inline bson_iterator::arrow bson_iterator::operator->() const noexcept { return arrow{**this}; }

inline bson_iterator& bson_iterator::operator++() noexcept {
    throw_if_error();
    return *this = bson_next(*this);
}

inline bool bson_iterator::stop() const noexcept { return bson_stop(*this); }

template <>
struct std::tuple_size<bson_iterator::reference> : std::integral_constant<std::size_t, 2> {};

template <>
struct std::tuple_element<0, bson_iterator::reference> {
    using type = std::string_view;
};

template <>
struct std::tuple_element<1, bson_iterator::reference> {
    using type = ::bson_value_ref;
};
#endif  // C++
