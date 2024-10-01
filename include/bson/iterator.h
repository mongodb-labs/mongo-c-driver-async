#pragma once

#include <bson/byte.h>
#include <bson/detail/assert.h>
#include <bson/detail/mem.h>
#include <bson/types.h>
#include <bson/utf8.h>

#include <mlib/config.h>
#include <mlib/integer.h>

#include <math.h>

#if mlib_is_cxx()
#include <iterator>
#endif

/**
 * @brief Reasons that advancing a bson_iterator may fail.
 */
enum bson_iter_errc {
    /**
     * @brief No error occurred (this value is equal to zero)
     */
    bson_iter_errc_okay = 0,
    /**
     * @brief There is not enough data in the buffer to find the next element.
     */
    bson_iter_errc_short_read = 1,
    /**
     * @brief The type tag on the element is not a recognized value.
     */
    bson_iter_errc_invalid_type,
    /**
     * @brief The element has an encoded length, but the length is too large for
     * the remaining buffer.
     */
    bson_iter_errc_invalid_length,
};

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
    constexpr bool operator==(const bson_iterator other) const noexcept;

    // Get the proxy reference
    constexpr reference operator*() const;

    // Preincrement
    constexpr bson_iterator& operator++() noexcept;
    // Postincrement
    constexpr bson_iterator operator++(int) noexcept {
        auto c = *this;
        ++*this;
        return c;
    }

    // Obtain a reference in-situ via `arrow`
    constexpr arrow operator->() const noexcept;

    // Obtain the error associated with this iterator, if one is defined
    [[nodiscard]] constexpr bson_iter_errc error() const noexcept;

    // Test whether the iterator has an errant state
    [[nodiscard]] constexpr bool has_error() const noexcept {
        return error() != bson_iter_errc_okay;
    }
    // Test whether the iterator points to a valid value (is not done and not an error)
    [[nodiscard]] constexpr bool has_value() const noexcept {
        return not has_error() and not stop();
    }

    // Test whether the iterator is the "done" iterator
    [[nodiscard]] constexpr bool stop() const noexcept;

    // If the iterator has an error condition, throw an exception
    constexpr void throw_if_error() const;

    // Obtain a pointer to the data referred-to by the iterator
    [[nodiscard]] constexpr const bson_byte* data() const noexcept { return _ptr; }

    // Obtain the size (in bytes) of the pointed-to data
    [[nodiscard]] constexpr size_type data_size() const noexcept;
#endif  // C++
} bson_iterator;

#define BSON_ITERATOR_NULL (mlib_init(bson_iterator){NULL, 0, 0})

mlib_extern_c_begin();

#if defined(__GNUC__) || defined(__clang__)
#define BV_LIKELY(X) __builtin_expect((X), 1)
#define BV_UNLIKELY(X) __builtin_expect((X), 0)
#else
#define BV_LIKELY(X) (X)
#define BV_UNLIKELY(X) (X)
#endif

/**
 * @internal
 * @brief Accepts a pointer-to-const `bson_byte` and returns it.
 *
 * Used as a type guard in the bson_data() macro
 */
mlib_constexpr const bson_byte* _bson_data_as_const(const bson_byte* p) mlib_noexcept { return p; }

// Equivalent to as_const, but accepts non-const
mlib_constexpr bson_byte* _bson_data_as_mut(bson_byte* p) mlib_noexcept { return p; }

/**
 * @brief Obtain a pointer to the beginning of the data for the given document.
 */
#define bson_data(X) _bson_data_as_const((X)._bson_document_data)
/**
 * @brief Obtain a mutable pointer to the beginning of the data for the given
 * document.
 *
 * @note This only works if the object is a mutable document reference (e.g. not
 * bson_view)
 */
#define bson_mut_data(X) _bson_data_as_mut((X)._bson_document_data)

/**
 * @brief Obtain the byte-size of the BSON document referred to by the given
 * bson_view.
 *
 * @param x A BSON object to inspect
 * @return uint32_t The size (in bytes) of the viewed document, or zero if `v`
 * is null
 */
#define bson_size(...) _bson_byte_size(bson_data((__VA_ARGS__)))
mlib_constexpr uint32_t _bson_byte_size(const bson_byte* p) mlib_noexcept {
    // A null document has length=0
    if (!p) {
        return 0;
    }
    // The length of a document is contained in a four-bytes little-endian
    // encoded integer. This size includes the header, the document body, and the
    // null terminator byte on the document.
    return _bson_read_u32le(p);
}

/**
 * @brief Obtain the byte-size of the given BSON document, as a signed integer
 */
#define bson_ssize(...) _bson_byte_ssize(bson_data((__VA_ARGS__)))
// We implement ssize() as a separate function rather than a cast so that the
// scope operator in `::bson_ssize()` works correctly.
mlib_constexpr int32_t _bson_byte_ssize(const bson_byte* p) mlib_noexcept {
    return (int32_t)_bson_byte_size(p);
}

/// @internal Generate a bson_iterator that encodes an error during iteration
mlib_constexpr bson_iterator _bson_iterator_error(enum bson_iter_errc err) mlib_noexcept {
    return mlib_init(bson_iterator){._ptr = NULL, ._keylen = 0, ._rlen = -((int32_t)err)};
}

/**
 * @internal
 * @brief Compute the byte-length of a regular expression BSON element.
 *
 * @param valptr The beginning of the value of the regular expression element
 * (the byte following the element key's NUL character)
 * @param maxlen The number of bytes available following `valptr`
 * @return int32_t The number of bytes occupied by the regex value.
 */
mlib_constexpr int32_t _bson_value_re_len(const char* valptr, int32_t maxlen) mlib_noexcept {
    BV_ASSERT(maxlen > 0);
    mlib_math_try();
    // A regex is encoded as <cstring><cstring>
    int64_t rx_len = mlibMathNonNegativeInt32(strnlen32(valptr, I(maxlen)));
    // Because the entire document is guaranteed to have null terminator
    // (this was checked before the iterator was created) and `maxlen >= 0`, we
    // can assume that rx_len is less than `maxlen`
    rx_len += 1;  // Add the null terminator. Now rx_len *might* be equal to
                  // maxlen.
    // The pointer to the beginning of the regex option cstring. If the
    // regex is missing the null terminator, then 'rx_len' will be equal to
    // maxlen, which will cause this pointer to point
    // past-the-end of the entire document.
    const char* opt_begin_ptr = (const char*)valptr + rx_len;
    // The number of bytes available for the regex option string. If the
    // regex cstring was missing a null terminator, this will end up as
    // zero.
    const int64_t opt_bytes_avail = mlibMathNonNegativeInt32(sub(I(maxlen), I(rx_len)));
    // The length of the option string. If the regex string was missing a
    // null terminator, then strnlen()'s maxlen argument will be zero, and
    // opt_len will therefore also be zero.
    int64_t opt_len = mlibMathNonNegativeInt32(strnlen32(opt_begin_ptr, I(opt_bytes_avail)));
    /// The number of bytes remaining in the doc following the option. This
    /// includes the null terminator for the option string, which we
    /// haven't passed yet.
    const int64_t trailing_bytes_remain
        = mlibMathNonNegativeInt32(sub(I(opt_bytes_avail), I(opt_len)));
    mlib_math_catch (_unused) {
        (void)_unused;
        return -(int)bson_iter_errc_invalid_length;
    }
    // There MUST be two more null terminators (the one after the opt
    // string, and the one at the end of the doc itself), so
    // 'trailing_bytes' must be at least two.
    if (trailing_bytes_remain < 2) {
        return -(int)bson_iter_errc_short_read;
    }
    // Advance past the option string's nul
    opt_len += 1;
    // This is the value's length
    int32_t ret = mlibMathNonNegativeInt32(add(I(rx_len), I(opt_len)));
    mlib_math_catch (_unused) {
        (void)_unused;
        return -(int)bson_iter_errc_invalid_length;
    }
    return ret;
}

/**
 * @internal
 * @brief Compute the size of the value data in a BSON element stored in
 * contiguous memory.
 *
 * @param tag The type tag
 * @param valptr The pointer where the value would begin
 * @param val_maxlen The number of bytes available following `valptr`
 * @return int32_t The size of the value, in bytes, or a negative encoded @ref
 * bson_iter_error_cond
 *
 * @pre The `val_maxlen` MUST be greater than zero.
 */
mlib_constexpr int32_t _bson_valsize(bson_type              tag,
                                     const bson_byte* const valptr,
                                     int32_t                val_maxlen) mlib_noexcept {
    // Assume: ValMaxLen > 0
    BV_ASSERT(val_maxlen > 0);
    /// "const_sizes" are the minimum distance we need to consume for each
    /// datatype. Length-prefixed strings have a 32-bit integer which needs to be
    /// skipped. The length prefix on strings already includes the null
    /// terminator after the string itself.
    ///
    /// docs/arrays are also length-prefixed, but this length prefix accounts for
    /// itself as part of its value, so it will be taken care of automatically
    /// when we jump based on that value.
    ///
    /// The table has 256 entries to consider ever possible value of an octet
    /// type tag. Most of these are INT32_MAX, which will trigger an overflow
    /// guard that checks the type tag again. Note the special minkey and maxkey
    /// tags, which are zero-length and valid, but not contiguous with the
    /// lower 20 type tags.
    const int32_t const_sizes[256] = {
        0,          // 0x0 BSON_TYPE_EOD
        8,          // 0x1 double
        4,          // 0x2 utf8
        0,          // 0x3 document,
        0,          // 0x4 array
        4 + 1,      // 0x5 binary (+1 for subtype)
        0,          // 0x6 undefined
        12,         // 0x7 OID (twelve bytes)
        1,          // 0x8 bool
        8,          // 0x9 datetime
        0,          // 0xa null
        INT32_MAX,  // 0xb regex. This value is special and triggers the overflow
                    // guard, which then inspects the regular expression value
                    // to find the length.
        12,         // 0xc dbpointer
        4,          // 0xd JS code
        4,          // 0xe Symbol
        4 + 4,      // 0xf Code with scope
        4,          // 0x10 int32
        8,          // 0x11 MongoDB timestamp
        8,          // 0x12 int64
        16,         // 0x13 decimal128
                    // clang-format off
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, 0 /* 0x7f maxkey */,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX,
      INT32_MAX, 0 /* 0xff minkey */
                    // clang-format on
    };

    const uint32_t tag_idx = (uint32_t)tag;
    BV_ASSERT(tag_idx < 256);

    const int32_t const_size = const_sizes[tag_idx];

    /// "varsize_pick" encodes whether a type tag contains a length prefix
    /// before its value.
    const bool varsize_pick[256] = {
        false,  // 0x0 BSON_TYPE_EOD
        false,  // 0x1 double
        true,   // 0x2 utf8
        true,   // 0x3 document,
        true,   // 0x4 array
        true,   // 0x5 binary,
        false,  // 0x6 undefined
        false,  // 0x7 OID (twelve bytes)
        false,  // 0x8 bool
        false,  // 0x9 datetime
        false,  // 0xa null
        false,  // 0xb regex
        true,   // 0xc dbpointer
        true,   // 0xd JS code
        true,   // 0xe Symbol
        true,   // 0xf Code with scope
        false,  // 0x10 int32
        false,  // 0x11 MongoDB timestamp
        false,  // 0x12 int64
        false,  // 0x13 decimal128
                // Remainder of array elements are zero-initialized to "false"
    };

    const bool has_varsize_prefix = varsize_pick[tag];

    int64_t full_len = const_size;
    if (has_varsize_prefix) {
        const bool have_enough = val_maxlen > 4;
        if (BV_UNLIKELY(!have_enough)) {
            // We require at least four bytes to read the int32_t length prefix of
            // the value.
            return -bson_iter_errc_invalid_length;
        }
        // The length of the value given by the length prefix:
        const int32_t varlen = (int32_t)_bson_read_u32le(valptr);
        BV_ASSERT(varlen >= 0);
        full_len += varlen;
    }
    // Check whether there is enough room to hold the value's required size.
    BV_ASSERT(full_len >= 0);
    if (BV_LIKELY(full_len < val_maxlen)) {
        // We have a good value size
        BV_ASSERT(full_len < INT32_MAX);
        return (int32_t)full_len;
    }

    // full_len was computed to be larger than val_maxlen.
    if (BV_LIKELY(tag == bson_type_regex)) {
        // Oops! We're actually reading a regular expression, which is designed
        // above to trigger the overrun check so that we can do something
        // different to calculate its size:
        return _bson_value_re_len((const char*)valptr, val_maxlen);
    }
    if (const_size == INT32_MAX) {
        // We indexed into the const_sizes table at some other invalid type
        return -bson_iter_errc_invalid_type;
    }
    // We indexed into a valid type and know the length that it should be, but we
    // do not have enough in val_maxlen to hold the value that was requested.
    // Indicate an overrun by returning an error
    return -bson_iter_errc_invalid_length;
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
mlib_constexpr bson_iterator _bson_iterator_at(const bson_byte* const data,
                                               int32_t                maxlen) mlib_noexcept {
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
            || type == bson_type_date_time || type == bson_type_double || type == bson_type_bool
            || type == bson_type_decimal128 || type == bson_type_int32 || type == bson_type_int64);
    }

    if (need_check_size) {
        const char* const valptr = keyptr + (keylen + 1);

        const int32_t vallen = _bson_valsize(type, (const bson_byte*)valptr, val_maxlen);
        if (BV_UNLIKELY(vallen < 0)) {
            return _bson_iterator_error((enum bson_iter_errc)(-vallen));
        }
    }
    return mlib_init(bson_iterator){
        ._ptr    = data,
        ._keylen = keylen,
        ._rlen   = maxlen,
    };
}

/**
 * @brief Obtain a bson_iterator referring to the first position within `v`.
 *
 * @param v A non-null bson_view
 * @return bson_iterator A new iterator `It` referring to the first valid
 * position of `v`, or an errant `It` if there is no valid first element.
 *
 * If `v` is empty, the returned iterator `It` will be a valid past-the-end
 * iterator, @ref bson_iterator_done() will return `true` for `It`, and `It`
 * will be equivalent to `bson_end(v)`.
 *
 * Otherwise, if the first element within `v` is well-formed, the returned `It`
 * will refer to that element.
 *
 * Otherwise, the returned `It` will indicate an error that can be obtained with
 * @ref bson_iterator_get_error, and @ref bson_iterator_done() will return
 * `true`.
 *
 * The returned iterator (as well as any iterator derived from it) is valid for
 * as long as `v` would be valid.
 */
#define bson_begin(...) _bson_begin(bson_data(__VA_ARGS__))
mlib_constexpr bson_iterator _bson_begin(const bson_byte* data) mlib_noexcept {
    BV_ASSERT(data);
    const int32_t   sz       = (int32_t)_bson_read_u32le(data);
    const ptrdiff_t tailsize = sz - (ptrdiff_t)sizeof(int32_t);
    BV_ASSERT(tailsize > 0);
    BV_ASSERT(tailsize < INT32_MAX);
    return _bson_iterator_at(data + sizeof(int32_t), (int32_t)tailsize);
}

/**
 * @brief Obtain a past-the-end "done" iterator for the given document `v`
 *
 * @param v A non-null BSON object
 * @return bson_iterator A new iterator referring to the end of `v`.
 *
 * The returned iterator does not refer to any element of the document. The
 * returned iterator will be considered "done" by @ref bson_iterator_done(). The
 * returned iterator is valid for as long as `v` would be valid.
 */
#define bson_end(...) _bson_end(bson_data(__VA_ARGS__))
mlib_constexpr bson_iterator _bson_end(const bson_byte* v) mlib_noexcept {
    BV_ASSERT(v);
    const uint32_t len = _bson_read_u32le(v);
    return _bson_iterator_at(v + len - 1, 1);
}

/**
 * @brief Determine whether two non-error iterators are equivalent (i.e. refer
 * to the same position in the same document)
 *
 * @param left A valid iterator
 * @param right A valid iterator
 * @return true If the two iterators refer to the same position, including the
 * past-the-end iterator as a valid position.
 * @return false Otherwise.
 *
 * At least one of the two iterators must not indicate an error. If both
 * iterators represent an error, the result is unspecified.
 */
mlib_constexpr bool bson_iterator_eq(bson_iterator left, bson_iterator right) mlib_noexcept {
    return left._ptr == right._ptr;
}

/**
 * @brief Determine whether the given iterator is at the end OR has encountered
 * an error.
 *
 * @param it A valid iterator derived from a bson_view or other valid iterator.
 * @retval true If advancing `it` with @ref bson_next would be illegal.
 * @retval false If `bson_next(it)` would provide a well-defined value.
 *
 * To determine whether there is an error, use @ref bson_iterator_get_error
 */
mlib_constexpr bool bson_stop(bson_iterator it) mlib_noexcept { return it._rlen <= 1; }

/// @internal Obtain a pointer to the beginning of the referred-to element's
/// value
mlib_constexpr const bson_byte* _bson_iterator_value_ptr(bson_iterator iter) mlib_noexcept {
    return iter._ptr + 1 + iter._keylen + 1;
}

/**
 * @brief Obtain a pointer to the beginning of the element data in memory
 *
 * @param it An iterator pointing to an element, or pointing to the end of the
 * document
 * @return const bson_byte* A pointer to the beginning of the element data if
 * `it` is not an end iterator. Otherwise, contains a pointer to the null
 * terminator at the end of the document.
 */
mlib_constexpr const bson_byte* bson_iterator_data(const bson_iterator it) mlib_noexcept {
    return it._ptr;
}

/**
 * @brief Obtain the type of the BSON element referred-to by the given iterator
 *
 * @param it The iterator to inspect
 * @return bson_type The type of the referred-to element. Returns BSON_TYPE_EOD
 * if the iterator refers to the end of the document.
 *
 * @pre bson_iterator_get_error(it) == false
 */
mlib_constexpr bson_type bson_iterator_type(bson_iterator it) mlib_noexcept {
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
mlib_constexpr uint32_t bson_iterator_data_size(const bson_iterator it) mlib_noexcept {
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
 *
 * If `it` referred to the final element in `V`, then the returned `Next` will
 * refer to the past-the-end position of `V`, and @ref bson_iterator_done(Next)
 * will return `true`.
 *
 * Otherwise, if the next element in `V` is in-bounds, then `Next` will refer to
 * that element (see below).
 *
 * Otherwise, `Next` will indicate an error that can be obtained with @ref
 * bson_iterator_error, and @ref bson_iterator_done will return `true`.
 *
 * @note This function will do basic validation on the next element to ensure
 * that it is "in-bounds", i.e. does not overrun the byte array referred to by
 * `V`. The pointed-to element may require additional validation based on its
 * type. The "in-bounds" check is here to ensure that a subsequent call to
 * `bson_next` or any iterator-dereferrencing functions will not overrun `V`.
 */
mlib_constexpr bson_iterator bson_next(const bson_iterator it) mlib_noexcept {
    const int32_t          skip   = (int32_t)bson_iterator_data_size(it);
    const bson_byte* const newptr = bson_iterator_data(it) + skip;
    const int32_t          remain = it._rlen - skip;
    BV_ASSERT(remain >= 0);
    return _bson_iterator_at(newptr, remain);
}

/**
 * @brief Obtain the error associated with the given iterator.
 *
 * @param it An iterator that may or may not have encountered an error
 * @return enum bson_iterator_error_cond The error that occurred while creating
 * `it`. Returns BSON_ITER_NO_ERROR if there was no error.
 */
mlib_constexpr enum bson_iter_errc bson_iterator_get_error(bson_iterator it) mlib_noexcept {
    return it._rlen < 0 ? (enum bson_iter_errc) - it._rlen : bson_iter_errc_okay;
}

/**
 * @brief Obtain a bson_utf8_view referring to the key string of the BSON
 * document element referred-to by the given iterator.
 *
 * @param it The iterator under inspection.
 * @return const char* A pointer to the beginning of a null-terminated string
 *
 * @pre bson_iterator_done(it) == false
 */
inline bson_utf8_view bson_key(bson_iterator it) mlib_noexcept {
    BV_ASSERT(it._rlen >= it._keylen + 1);
    BV_ASSERT(it._ptr);
    return (bson_utf8_view){.data = (const char*)it._ptr + 1, .len = (uint32_t)it._keylen};
}

/**
 * @brief Compare a bson_iterator's key with a string
 */
#define bson_key_eq(Iterator, Key) _bson_key_eq((Iterator), bson_as_utf8((Key)))
mlib_constexpr bool _bson_key_eq(const bson_iterator it, bson_utf8_view key) mlib_noexcept {
    const bson_utf8_view it_key = bson_key(it);
    BV_ASSERT(it._keylen >= 0);
    if (it_key.len != key.len) {
        return false;
    }
    if (mlib_is_consteval()) {
        for (uint32_t n = 0; n < key.len; ++n) {
            if (key.data[n] != it_key.data[n]) {
                return false;
            }
        }
        return true;
    } else {
        return memcmp(it_key.data, key.data, key.len) == 0;
    }
}

#define bson_foreach(IterName, Viewable)                                                           \
    _bsonForEach(IterName, bson_as_view(Viewable), _bsonForEachOnlyOnce, _bsonForEachView)
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
 *
 * @param v A document to inspect
 * @param key The key to search for
 * @return bson_iterator An iterator pointing to the found element, OR the
 * done-iterator if no element was found, OR an errant iterator if a parsing
 * error occured.
 */
#define bson_find(Doc, Key) _bson_find(bson_data((Doc)), bson_as_utf8((Key)))
mlib_constexpr bson_iterator _bson_find(const bson_byte* v, bson_utf8_view key) mlib_noexcept {
    bson_foreach_subrange(iter, _bson_begin(v), _bson_end(v)) {
        if (!bson_iterator_get_error(iter) && bson_key_eq(iter, key)) {
            return iter;
        }
    }
    return _bson_end(v);
}

inline bson_utf8_view _bson_read_stringlike_at(const bson_byte* p) mlib_noexcept {
    const int32_t len = (int32_t)_bson_read_u32le(p);
    // String length checks were already performed by our callers
    BV_ASSERT(len >= 1);
    return mlib_init(bson_utf8_view){(const char*)(p + sizeof len), (uint32_t)len - 1};
}

inline bson_utf8_view _bson_iterator_stringlike(bson_iterator it) mlib_noexcept {
    const bson_byte* after_key = _bson_iterator_value_ptr(it);
    bson_utf8_view   r         = _bson_read_stringlike_at(after_key);
    BV_ASSERT(r.len > 0);
    BV_ASSERT(r.len < (size_t)it._rlen);
    return r;
}

/**
 * @brief Obtain the value of a IEE754 double-precision floating point in the
 * element referred-to by the given iterator.
 *
 * @param it A non-error iterator
 * @return double If `it` refers to an element with type DOUBLE, returns the
 * value contained in that element, otherwise returns zero.
 */
mlib_constexpr double bson_iterator_double(bson_iterator it) mlib_noexcept {
    if (bson_iterator_type(it) != bson_type_double) {
        return 0.0;
    }
    uint64_t bytes = _bson_read_u64le(_bson_iterator_value_ptr(it));
    double   d     = 0;
    memcpy(&d, &bytes, sizeof bytes);
    return d;
}

/**
 * @brief Obtain the range of UTF-8 encoded bytes referred-to by the given
 * iterator
 *
 * @param it A non-error iterator
 * @return bson_utf8_view If the element is of type UTF-8, returns a view to the
 * UTF-8 string it contains, otherwise returns a null view.
 *
 * @note The null-terminated `char` array might not be a valid UTF-8 code unit
 * sequence. Extra validation is required.
 * @note The `char` array may contain null characters.
 */
inline bson_utf8_view bson_iterator_utf8(bson_iterator it) mlib_noexcept {
    if (bson_iterator_type(it) != bson_type_utf8) {
        return mlib_init(bson_utf8_view){NULL, 0};
    }
    return _bson_iterator_stringlike(it);
}

typedef struct bson_binary {
    const bson_byte* data;
    uint32_t         data_len;
    uint8_t          subtype;
} bson_binary;

mlib_constexpr bson_binary bson_iterator_binary(bson_iterator it) mlib_noexcept {
    BV_ASSERT(!bson_stop(it));
    if (bson_iterator_type(it) != bson_type_binary) {
        return mlib_init(bson_binary){0, 0, 0};
    }
    const bson_byte* const valptr = _bson_iterator_value_ptr(it);
    const int32_t          size   = (int32_t)_bson_read_u32le(valptr);
    BV_ASSERT(size >= 0);
    const uint8_t          subtype = valptr[4].v;
    const bson_byte* const p       = valptr + 5;
    return mlib_init(bson_binary){p, (uint32_t)size, subtype};
}

typedef struct bson_oid {
    uint8_t bytes[12];
} bson_oid;
#define BSON_OID_ZERO (mlib_init(bson_oid){{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}})

mlib_constexpr bson_oid bson_iterator_oid(bson_iterator it) mlib_noexcept {
    BV_ASSERT(!bson_stop(it));
    if (bson_iterator_type(it) != bson_type_oid) {
        return mlib_init(bson_oid){0};
    }
    bson_oid ret;
    memcpy(&ret, _bson_iterator_value_ptr(it), sizeof ret);
    return ret;
}

/**
 * @brief Obtain the boolean value of the referred-to element.
 *
 * If the element is not a boolean value, returns false.
 *
 * @param it A non-error iterator
 * @retval true If the element contains a true value
 * @retval false IF the element contains a false value, or the element is not a
 * bool value.
 */
mlib_constexpr bool bson_iterator_bool(bson_iterator it) mlib_noexcept {
    if (bson_iterator_type(it) != bson_type_bool) {
        return false;
    }
    return _bson_iterator_value_ptr(it)[0].v != 0;
}

typedef struct bson_datetime {
    int64_t utc_ms_offset;
} bson_datetime;

mlib_constexpr bson_datetime bson_iterator_datetime(bson_iterator it) mlib_noexcept {
    bson_datetime ret = {0};
    if (bson_iterator_type(it) != bson_type_date_time) {
        return ret;
    }
    ret.utc_ms_offset = (int64_t)_bson_read_u64le(_bson_iterator_value_ptr(it));
    return ret;
}

typedef struct bson_regex {
    const char* regex;
    const char* options;
    uint32_t    regex_len;
    uint32_t    options_len;
} bson_regex;

#define BSON_REGEX_NULL (mlib_init(bson_regex){NULL, NULL, 0, 0})

inline bson_regex bson_iterator_regex(bson_iterator it) mlib_noexcept {
    const bson_regex null_regex = BSON_REGEX_NULL;

    if (bson_iterator_type(it) != bson_type_regex) {
        return null_regex;
    }
    const bson_byte* const p = _bson_iterator_value_ptr(it);
    mlib_math_try();
    const char* const regex   = (const char*)p;
    const uint32_t    rx_len  = mlibMath(castUInt32(strlen32(regex)));
    const char* const options = regex + rx_len + 1;
    const uint32_t    opt_len = mlibMath(castUInt32(strlen32(options)));
    mlib_math_catch (_unused) {
        (void)_unused;
        return null_regex;
    }
    return mlib_init(bson_regex){regex, options, rx_len, opt_len};
}

typedef struct bson_dbpointer {
    const char* collection;
    uint32_t    collection_len;
    bson_oid    object_id;
} bson_dbpointer;

#define BSON_DBPOINTER_NULL (mlib_init(bson_dbpointer){NULL, 0, BSON_OID_ZERO})

mlib_constexpr bson_dbpointer bson_iterator_dbpointer(bson_iterator it) mlib_noexcept {
    const bson_dbpointer null_dbp = BSON_DBPOINTER_NULL;
    if (bson_iterator_type(it) != bson_type_dbpointer) {
        return null_dbp;
    }
    const bson_byte* const p              = _bson_iterator_value_ptr(it);
    const int32_t          coll_name_size = (int32_t)_bson_read_u32le(p);
    bson_dbpointer         ret;
    ret.collection = (const char*)p + 4;
    BV_ASSERT(coll_name_size > 0);
    ret.collection_len = (uint32_t)coll_name_size - 1;
    memcpy(&ret.object_id.bytes, p + 4 + coll_name_size, sizeof(bson_oid));
    return ret;
}

typedef struct bson_code {
    bson_utf8_view utf8;
} bson_code;

inline bson_code bson_iterator_code(bson_iterator it) mlib_noexcept {
    bson_code ret = {BSON_UTF8_NULL};
    if (bson_iterator_type(it) != bson_type_code) {
        return ret;
    }
    ret.utf8 = _bson_iterator_stringlike(it);
    return ret;
}

typedef struct bson_symbol {
    bson_utf8_view utf8;
} bson_symbol;

inline bson_symbol bson_iterator_symbol(bson_iterator it) mlib_noexcept {
    bson_symbol ret = {BSON_UTF8_NULL};
    if (bson_iterator_type(it) != bson_type_symbol) {
        return ret;
    }
    ret.utf8 = _bson_iterator_stringlike(it);
    return ret;
}

mlib_constexpr int32_t bson_iterator_int32(bson_iterator it) mlib_noexcept {
    if (bson_iterator_type(it) != bson_type_int32) {
        return 0;
    }
    return (int32_t)_bson_read_u32le(_bson_iterator_value_ptr(it));
}

typedef struct bson_timestamp {
    uint32_t increment;
    uint32_t utc_sec_offset;
} bson_timestamp;

mlib_constexpr bson_timestamp bson_iterator_timestamp(bson_iterator it) mlib_noexcept {
    bson_timestamp ret = {0, 0};
    if (bson_iterator_type(it) != bson_type_timestamp) {
        return ret;
    }
    ret.increment      = _bson_read_u32le(_bson_iterator_value_ptr(it));
    ret.utc_sec_offset = _bson_read_u32le(_bson_iterator_value_ptr(it) + 4);
    return ret;
}

mlib_constexpr int64_t bson_iterator_int64(bson_iterator it) mlib_noexcept {
    if (bson_iterator_type(it) != bson_type_int64) {
        return 0;
    }
    return (int64_t)_bson_read_u64le(_bson_iterator_value_ptr(it));
}

struct bson_decimal128 {
    uint8_t bytes[16];
};

inline struct bson_decimal128 bson_iterator_decimal128(bson_iterator it) mlib_noexcept {
    struct bson_decimal128 ret;
    memset(&ret, 0, sizeof ret);
    if (bson_iterator_type(it) != bson_type_decimal128) {
        return ret;
    }
    memcpy(&ret, _bson_iterator_value_ptr(it), sizeof ret);
    return ret;
}

mlib_constexpr double bson_iterator_as_double(bson_iterator it, bool* okay) mlib_noexcept {
    switch (bson_iterator_type(it)) {
    case bson_type_double:
        (okay && (*okay = true));
        return bson_iterator_double(it);
    case bson_type_eod:
    case bson_type_utf8:
    case bson_type_document:
    case bson_type_array:
    case bson_type_binary:
    case bson_type_undefined:
    case bson_type_oid:
        (okay && (*okay = false));
        return 0;
    case bson_type_bool:
        (okay && (*okay = true));
        return bson_iterator_bool(it) ? 1 : 0;
    case bson_type_date_time:
    case bson_type_null:
    case bson_type_regex:
    case bson_type_dbpointer:
    case bson_type_code:
    case bson_type_symbol:
    case bson_type_codewscope:
        (okay && (*okay = false));
        return 0;
    case bson_type_int32:
        (okay && (*okay = true));
        return (double)bson_iterator_int32(it);
    case bson_type_timestamp:
        (okay && (*okay = false));
        return 0;
    case bson_type_int64:
        (okay && (*okay = true));
        return (double)bson_iterator_int64(it);
    case bson_type_decimal128:
    case bson_type_maxkey:
    case bson_type_minkey:
    default:
        (okay && (*okay = false));
        return 0;
    }
}

mlib_constexpr bool bson_iterator_as_bool(bson_iterator it) mlib_noexcept {
    switch (bson_iterator_type(it)) {
    case bson_type_eod:
        return false;
    case bson_type_double:
        return fpclassify(bson_iterator_double(it)) != FP_ZERO;
    case bson_type_utf8:
        return bson_iterator_utf8(it).len != 0;
    case bson_type_document:
    case bson_type_array: {
        // Return `true` if the pointed-to document is non-empty
        return _bson_read_u32le(_bson_iterator_value_ptr(it)) > 5;
    }
    case bson_type_binary: {
        bson_binary bin = bson_iterator_binary(it);
        return bin.data_len != 0;
    }
    case bson_type_undefined:
        return false;
    case bson_type_oid:
        return true;
    case bson_type_bool:
        return bson_iterator_bool(it);
    case bson_type_date_time:
        return true;
    case bson_type_null:
        return false;
    case bson_type_regex:
    case bson_type_dbpointer:
    case bson_type_code:
    case bson_type_symbol:
    case bson_type_codewscope:
        return true;
    case bson_type_int32:
        return bson_iterator_int32(it) != 0;
    case bson_type_timestamp:
        return true;
    case bson_type_int64:
        return bson_iterator_int64(it) != 0;
    case bson_type_decimal128:
        return true;
    case bson_type_maxkey:
    case bson_type_minkey:
        return false;
    }
    return false;
}

mlib_constexpr int32_t bson_iterator_as_int32(bson_iterator it, bool* okay) mlib_noexcept {
    switch (bson_iterator_type(it)) {
    case bson_type_double:
        (okay && (*okay = true));
        return (int32_t)bson_iterator_as_double(it, NULL);
    case bson_type_eod:
    case bson_type_utf8:
    case bson_type_document:
    case bson_type_array:
    case bson_type_binary:
    case bson_type_undefined:
    case bson_type_oid:
        (okay && (*okay = false));
        return 0;
    case bson_type_bool:
        (okay && (*okay = true));
        return bson_iterator_bool(it) ? 1 : 0;
    case bson_type_date_time:
    case bson_type_null:
    case bson_type_regex:
    case bson_type_dbpointer:
    case bson_type_code:
    case bson_type_symbol:
    case bson_type_codewscope:
        (okay && (*okay = false));
        return 0;
    case bson_type_int32:
        (okay && (*okay = true));
        return bson_iterator_int32(it);
    case bson_type_timestamp:
        (okay && (*okay = false));
        return 0;
    case bson_type_int64:
        (okay && (*okay = true));
        return (int32_t)bson_iterator_int64(it);
    case bson_type_decimal128:
    case bson_type_maxkey:
    case bson_type_minkey:
    default:
        (okay && (*okay = false));
        return 0;
    }
}

mlib_constexpr int64_t bson_iterator_as_int64(bson_iterator it, bool* okay) mlib_noexcept {
    switch (bson_iterator_type(it)) {
    case bson_type_double:
        (okay && (*okay = true));
        return (int64_t)bson_iterator_as_double(it, NULL);
    case bson_type_eod:
    case bson_type_utf8:
    case bson_type_document:
    case bson_type_array:
    case bson_type_binary:
    case bson_type_undefined:
    case bson_type_oid:
        (okay && (*okay = false));
        return 0;
    case bson_type_bool:
        (okay && (*okay = true));
        return bson_iterator_bool(it) ? 1 : 0;
    case bson_type_date_time:
    case bson_type_null:
    case bson_type_regex:
    case bson_type_dbpointer:
    case bson_type_code:
    case bson_type_symbol:
    case bson_type_codewscope:
        (okay && (*okay = false));
        return 0;
    case bson_type_int32:
        (okay && (*okay = true));
        return bson_iterator_int32(it);
    case bson_type_timestamp:
        (okay && (*okay = false));
        return 0;
    case bson_type_int64:
        (okay && (*okay = true));
        return bson_iterator_int64(it);
    case bson_type_decimal128:
    case bson_type_maxkey:
    case bson_type_minkey:
    default:
        return 0;
    }
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

constexpr std::uint32_t bson_iterator::data_size() const noexcept {
    return bson_iterator_data_size(*this);
}

constexpr bool bson_iterator::operator==(bson_iterator other) const noexcept {
    return bson_iterator_eq(*this, other);
}

constexpr bson_iter_errc bson_iterator::error() const noexcept {
    return bson_iterator_get_error(*this);
}
constexpr void bson_iterator::throw_if_error() const {
    switch (this->error()) {
    case bson_iter_errc_okay:
        return;
#define X(Cond)                                                                                    \
    case Cond:                                                                                     \
        throw bson::iterator_error_of<Cond>()
        X(bson_iter_errc_short_read);
        X(bson_iter_errc_invalid_length);
        X(bson_iter_errc_invalid_type);
#undef X
    }
}

constexpr bson_iterator& bson_iterator::operator++() noexcept {
    throw_if_error();
    return *this = bson_next(*this);
}

constexpr bool bson_iterator::stop() const noexcept { return bson_stop(*this); }

#endif  // C++

#undef BV_LIKELY
#undef BV_UNLIKELY
