#pragma once

#include <mlib/config.h>
#include <mlib/cstring.h>
#include <mlib/integer.h>

#if defined(__GNUC__) || defined(__clang__)
#define BV_LIKELY(X) __builtin_expect((X), 1)
#define BV_UNLIKELY(X) __builtin_expect((X), 0)
#else
#define BV_LIKELY(X) (X)
#define BV_UNLIKELY(X) (X)
#endif

#include "./types.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if mlib_is_cxx()
#include <cinttypes>
#include <cstdlib>
#include <iterator>
#include <stdexcept>
#include <string_view>
#endif

mlib_extern_c_begin();

struct _bson_t;       // Forward-declare for bson_t interop
struct _bson_iter_t;  // Forward-declare for bson_iter_t interop

// clang-format off
#pragma push_macro("BSON_VIEW_CHECKED")
#ifdef BSON_VIEW_CHECKED
   #if BSON_VIEW_CHECKED == 1
      #define BSON_VIEW_CHECKED_DEFAULT 1
   #elif BSON_VIEW_CHECKED == 0
      #define BSON_VIEW_CHECKED_DEFAULT 0
   #else
      #define BSON_VIEW_CHECKED_DEFAULT 0
   #endif
   // Undef the macro, we'll re-define it later
   #undef BSON_VIEW_CHECKED
#else
   #define BSON_VIEW_CHECKED_DEFAULT 1
#endif
// clang-format on

enum {
    /// Toggle on/off to enable/disable debug assertions
    BSON_VIEW_CHECKED = BSON_VIEW_CHECKED_DEFAULT
};

/**
 * @brief A type that is the size of a byte, but does not alias with other types
 * (except char)
 */
typedef struct bson_byte {
    /// The 8-bit value of the byte
    uint8_t v;
} bson_byte;

typedef struct bson_view bson_view;

/**
 * @internal
 * @brief Read a little-endian 32-bit unsigned integer from the given array of
 * four octets
 *
 * @param bytes Pointer-to an array of AT LEAST four octets
 * @return uint32_t The decoded integer value.
 */
mlib_constexpr int32_t _bson_read_int32_le(const bson_byte* bytes) mlib_noexcept {
    uint32_t u32 = 0;
    u32 |= bytes[3].v;
    u32 <<= 8;
    u32 |= bytes[2].v;
    u32 <<= 8;
    u32 |= bytes[1].v;
    u32 <<= 8;
    u32 |= bytes[0].v;
    return (int32_t)u32;
}

/**
 * @internal
 * @brief Read a litt-elendian 64-bit unsigned integer from the given array of
 * eight octets.
 *
 * @param bytes Pointer-to an array of AT LEAST eight octets
 * @return uint64_t The decoded integer value.
 */
mlib_constexpr int64_t _bson_read_int64_le(const bson_byte* bytes) mlib_noexcept {
    const uint64_t lo  = (uint64_t)_bson_read_int32_le(bytes);
    const uint64_t hi  = (uint64_t)_bson_read_int32_le(bytes + 4);
    const uint64_t u64 = (hi << 32) | lo;
    return (int64_t)u64;
}

/// Fire an assertion failure. This function will abort the program and will not
/// return to the caller.
extern void _bson_assert_fail(const char*, const char* file, int line);

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
 * @brief Reasons that advancing a bson_iterator may fail.
 */
enum bson_iterator_error_cond {
    /**
     * @brief No error occurred (this value is equal to zero)
     */
    BSON_ITER_NO_ERROR = 0,
    /**
     * @brief There is not enough data in the buffer to find the next element.
     */
    BSON_ITER_SHORT_READ = 1,
    /**
     * @brief The type tag on the element is not a recognized value.
     */
    BSON_ITER_INVALID_TYPE,
    /**
     * @brief The element has an encoded length, but the length is too large for
     * the remaining buffer.
     */
    BSON_ITER_INVALID_LENGTH,
};

/**
 * @brief A type that allows inspecting the elements of a bson_view document.
 *
 * This type is trivial, and can be copied and passed as a pointer-like type.
 * This type is zero-initializable.
 */
typedef struct bson_iterator {
    /// A pointer to the element referred-to by this iterator.
    const bson_byte* _ptr;
    /// @private The length of the key string, in bytes, not including the nul
    int32_t _keylen;
    /// @private The number of bytes remaining in the document, or an error code.
    int32_t _rlen;

#if mlib_is_cxx()

    class reference;
    class arrow;
    using difference_type   = int32_t;
    using value_type        = reference;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept  = std::forward_iterator_tag;
    using pointer           = bson_iterator;

    constexpr bool operator==(const bson_iterator other) const noexcept;

    inline reference operator*() const;

    inline bson_iterator& operator++() noexcept;

    bson_iterator operator++(int) noexcept {
        auto c = *this;
        ++*this;
        return c;
    }

    inline arrow operator->() const noexcept;

    [[nodiscard]] inline bson_iterator_error_cond error() const noexcept;

    [[nodiscard]] bool has_error() const noexcept { return error() != BSON_ITER_NO_ERROR; }

    [[nodiscard]] inline bool done() const noexcept;

    [[nodiscard]] bool has_value() const noexcept { return not has_error() and not done(); }

    inline void throw_if_error() const;

    [[nodiscard]] const bson_byte* data() const noexcept { return _ptr; }

    [[nodiscard]] inline std::size_t data_size() const noexcept;
    [[nodiscard]] inline bson_type   type() const noexcept;
#endif
} bson_iterator;

/**
 * @internal
 * @brief Accepts a pointer-to-const `bson_byte` and returns it.
 *
 * Used as a type guard in the bson_data() macro
 */
mlib_constexpr const bson_byte* _bson_data_as_const(const bson_byte* p) mlib_noexcept { return p; }

// Equivalen to as_const, but accepts non-const
mlib_constexpr bson_byte* _bson_data_as_mut(bson_byte* p) mlib_noexcept { return p; }

/**
 * @brief Obtain a pointer to the beginning of the data for the given document.
 */
#define bson_data(X) (_bson_data_as_const((X)._bson_document_data))
/**
 * @brief Obtain a mutable pointer to the beginning of the data for the given
 * document.
 *
 * @note This only works if the object is a mutable document reference (e.g. not
 * bson_view)
 */
#define bson_mut_data(X) (_bson_data_as_mut((X)._bson_document_data))

/**
 * @brief A type specifically representing a nullable read-only view of a BSON
 * document.
 *
 * @note This structure SHOULD NOT be created manually. Prefer instead to use
 * @ref bson_view_from_data, @ref bson_view_from_bson_t, or
 * @ref bson_view_from_iter, which will validate the header and trailer of the
 * pointed-to data. Use @ref BSON_VIEW_NULL to initialize a view to a "null"
 * value.
 *
 * This type is trivial and zero-initializable.
 */
typedef struct bson_view {
    /**
     * @brief Pointer to the beginning of the referenced BSON document, or NULL.
     *
     * If NULL, the view refers to nothing.
     */
    const bson_byte* _bson_document_data;

#if mlib_is_cxx()
    /**
     * @brief Alias of @ref bson_iterator
     */
    using iterator = bson_iterator;

    /**
     * @brief Equivalent to @ref bson_begin()
     */
    [[nodiscard]] inline iterator begin() const noexcept;

    /**
     * @brief Equivalent to @ref bson_end()
     */
    [[nodiscard]] inline iterator end() const noexcept;

    /**
     * @brief Check whether this view is non-null
     *
     * @return true If the view refers to a document
     * @return false If the view is null
     */
    explicit operator bool() const noexcept { return has_value(); }

    [[nodiscard]] const bson_byte* data() const noexcept { return bson_data(*this); }

    /**
     * @brief Determine whether the view refers to an empty document.
     */
    [[nodiscard]] bool empty() const noexcept { return not has_value() or byte_size() == 5; }

    /**
     * @brief Obtain the size of the document data, in bytes
     */
    [[nodiscard]] inline uint32_t byte_size() const noexcept;

    /**
     * @brief Determine whether this view is non-null
     *
     * @return true If the view refers to a document
     * @return false If the view is null
     */
    [[nodiscard]] bool has_value() const noexcept { return data() != nullptr; }

    /**
     * @brief Obtain an iterator referring to the first element with the
     * specified key.
     *
     * @param key The key to search for
     * @return iterator An iterator referring to the found element, or the end
     * iterator, or an errant iterator
     */
    [[nodiscard]] inline iterator find(std::string_view key) const noexcept;

    /**
     * @brief Construct a view from the given input data
     *
     * @param b Pointer to the beginning of a buffer
     * @param datalen The addressable length beginning at `b`
     * @return bson_view A non-null view
     *
     * @throws bson_view_error if parsing the data buffer results in an error.
     *
     * @note Use @ref bson_view_from_data() for a non-throwing equivalent API.
     */
    [[nodiscard]] static inline bson_view from_data(const bson_byte* b, size_t datalen);
#endif
} bson_view;

/// A "null" constant expression for bson_view objects
#define BSON_VIEW_NULL (mlib_init(bson_view){NULL})

mlib_constexpr uint32_t _bson_byte_size(const bson_byte* p) mlib_noexcept {
    // A null document has length=0
    if (!p) {
        return 0;
    }
    // The length of a document is contained in a four-bytes little-endian
    // encoded integer. This size includes the header, the document body, and the
    // null terminator byte on the document.
    return (uint32_t)_bson_read_int32_le(p);
}

/**
 * @brief Obtain the byte-size of the BSON document referred to by the given
 * bson_view.
 *
 * @param x A `bson_view` or `bson_mut` to inspect
 * @return uint32_t The size (in bytes) of the viewed document, or zero if `v`
 * is null
 */
#define bson_size(...) _bson_byte_size(bson_data((__VA_ARGS__)))
/**
 * @brief Obtain the byte-size of the given BSON document, as a signed integer
 *
 * @param x A `bson_view` or `bson_mut` to inspect
 * @return uint32_t The size (in bytes) of the viewed document, or zero if `v`
 * is null
 */
#define bson_ssize(...) ((int32_t)bson_size(__VA_ARGS__))

/**
 * @brief The reason that we may have failed to create a bson_view object in
 * @see bson_view_from_trusted_data() or @see bson_view_from_untrusted_data
 */
enum bson_view_invalid_reason {
    /**
     * @brief There is no error creating the view, and the view is ready.
     * Equivalent to zero.
     */
    BSON_VIEW_OKAY,
    /**
     * @brief The given data buffer is too short to possibly hold the
     * document.
     *
     * If the buffer is less than five bytes, it is impossible to be a valid
     * document. If the buffer is more than five bytes and this error occurs,
     * the document header declares a length that is longer than the buffer.
     */
    BSON_VIEW_SHORT_READ,
    /**
     * @brief The document header declares an invalid length.
     */
    BSON_VIEW_INVALID_HEADER,
    /**
     * @brief The document does not have a null terminator
     */
    BSON_VIEW_INVALID_TERMINATOR,
};

/**
 * @brief View the given pointed-to data as a BSON document.
 *
 * @param data A pointer to the beginning of a BSON document.
 * @param len The length of the pointed-to data buffer.
 * @param[out] error An optional out-param to determine why view creation may
 * have failed.
 * @return bson_view A view, or BSON_VIEW_NULL if the data is invalid.
 *
 * @throw BSON_VIEW_SHORT_READ if data_len is shorter than the length of the
 * document as declared by the header, or data_len is less than five (the
 * minimum size of a BSON document).
 * @throw BSON_VIEW_INVALID_HEADER if the BSON header in the data is of an
 * invalid length (either too large or too small).
 * @throw BSON_VIEW_INVALID_TERMINATOR if the BSON null terminator is missing
 * from the input data.
 *
 * @note IMPORTANT: This function DOES NOT consider 'data_len' being longer
 * than the document to be an error. This allows support of buffered reads of
 * unknown sizes. The actual length of the resulting document can be obtained
 * with @ref bson_view_len.
 *
 * @note IMPORTANT: This function DOES NOT validate the elements of the
 * document: Document elements must be validated during iteration.
 *
 * The returned view is valid *until* one of:
 *
 * - Dereferencing `data` would be undefined behavior.
 * - Any data accessible via `data` is modified.
 */
mlib_constexpr bson_view bson_view_from_data(const bson_byte* const               data,
                                             const size_t                         data_len,
                                             enum bson_view_invalid_reason* const error)
    mlib_noexcept {
    bson_view                     ret = BSON_VIEW_NULL;
    enum bson_view_invalid_reason err = BSON_VIEW_OKAY;
    int32_t                       len = 0;
    // All BSON data must be at least five bytes long
    if (data_len < 5) {
        err = BSON_VIEW_SHORT_READ;
        goto done;
    }
    // Read the length header. This includes the header's four bytes, the
    // document's element data, and the null terminator byte.
    len = _bson_read_int32_le(data);
    // Check that the size is in-bounds
    if (len < 5) {
        err = BSON_VIEW_INVALID_HEADER;
        goto done;
    }
    // Check that the buffer is large enough to hold expected the document
    if ((uint32_t)len > data_len) {
        // Not enough data to do the read
        err = BSON_VIEW_SHORT_READ;
        goto done;
    }
    // The document must have a zero-byte at the end.
    if (data[len - 1].v != 0) {
        err = BSON_VIEW_INVALID_TERMINATOR;
        goto done;
    }
    // Okay!
    ret._bson_document_data = data;
done:
    if (error) {
        *error = err;
    }
    return ret;
}

/**
 * @brief A pointer+length pair referring to a read-only array of `char`.
 *
 * For a well-formed bson_utf8_view `V`, if the `V.data` member is not NULL,
 * `V.data` points to the beginning of an array of `char` with a length of at
 * least `V.len`.
 *
 * @note The viewed array is NOT guaranteed to be null-terminated in general.
 */
typedef struct bson_utf8_view {
    /// A pointer to the beginning of a char array.
    const char* data;
    /**
     * @brief The length of the array pointed-to by `data`, if `data` is non
     * NULL.
     */
    size_t len;

#if mlib_is_cxx()
    constexpr static bson_utf8_view from_str(const std::string_view& sv) noexcept {
        return bson_utf8_view{sv.data(), sv.size()};
    }

    constexpr explicit operator std::string_view() const noexcept {
        return std::string_view(data, len);
    }

    constexpr bool operator==(std::string_view sv) const noexcept {
        return std::string_view(*this) == sv;
    }
#endif
} bson_utf8_view;

/**
 * @brief Create a bson_utf8_view from a pointer-to-array and a length
 *
 * @param s A pointer to an array of `char`
 * @param len The length of the new view. This value must not be greater than
 * the length of the array pointed-to by `s`
 * @return bson_utf8_view A view of the array at `s`, with `len` characters.
 *
 * @note The returned view MAY contain null characters, if the input array
 * contains null characters.
 */
mlib_constexpr bson_utf8_view bson_utf8_view_from_data(const char* s, size_t len) mlib_noexcept {
    return mlib_init(bson_utf8_view){s, len};
}

/**
 * @brief Create a bson_utf8_view from a null-terminated pointer-to-array of
 * char
 *
 * @param s A pointer to an array of `char`, which must include a null character
 * @return bson_utf8_view A UTF-8 view that views the longest prefix of the
 * array pointed-to by `s` that contains no null characters
 *
 * @note The returned bson_utf8_view is guaranteed to NOT contain embedded null
 * characters.
 */
mlib_constexpr bson_utf8_view bson_utf8_view_from_cstring(const char* s) mlib_noexcept {
    return bson_utf8_view_from_data(s, mlib_strlen(s));
}

/**
 * @brief Create a bson_utf8_view, automatically determining the size if the
 * given size argument is less than zero.
 *
 * @param s A pointer-to-array of char
 * @param len The length of the view to return, or less-than-zero to request
 * an automatic length
 * @return bson_utf8_view A new bson_utf8_view for the given parameters
 *
 * If `len` is *less than* zero, this call is equivalent to
 * `bson_utf8_view_from_cstring(s)`, otherwise it is equivalent to
 * `bson_utf8_view_from_data(s, len)`
 */
mlib_constexpr bson_utf8_view bson_utf8_view_autolen(const char* s, ssize_t len) mlib_noexcept {
    if (len < 0) {
        return bson_utf8_view_from_cstring(s);
    } else {
        return bson_utf8_view_from_data(s, (size_t)len);
    }
}

/**
 * @brief Return the longest prefix of `str` that does not contain embedded null
 * characters.
 *
 * @param str A UTF-8 string, possibly containing embedded null characters.
 * @return bson_utf8_view A string that begins at the same position as `str`,
 * but truncated at the first embedded null character, or equivalent to `str`
 * if `str` contains no null characters.
 */
mlib_constexpr bson_utf8_view bson_utf8_view_chopnulls(bson_utf8_view str) mlib_noexcept {
    size_t len = strnlen(str.data, str.len);
    str.len    = len;
    return str;
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
mlib_constexpr bool bson_iterator_done(bson_iterator it) mlib_noexcept { return it._rlen <= 1; }

/**
 * @brief Obtain the error associated with the given iterator.
 *
 * @param it An iterator that may or may not have encountered an error
 * @return enum bson_iterator_error_cond The error that occurred while creating
 * `it`. Returns BSON_ITER_NO_ERROR if there was no error.
 */
static mlib_constexpr enum bson_iterator_error_cond bson_iterator_get_error(bson_iterator it) {
    return it._rlen < 0 ? (enum bson_iterator_error_cond) - it._rlen : BSON_ITER_NO_ERROR;
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
mlib_constexpr bson_utf8_view bson_iterator_key(bson_iterator it) mlib_noexcept {
    BV_ASSERT(it._rlen >= it._keylen + 1);
    BV_ASSERT(it._ptr);
    return (bson_utf8_view){.data = (const char*)it._ptr + 1, .len = (uint32_t)it._keylen};
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

/// @internal Generate a bson_iterator that encodes an error during iteration
mlib_constexpr bson_iterator _bson_iterator_error(enum bson_iterator_error_cond err) mlib_noexcept {
    return mlib_init(bson_iterator){._rlen = -((int32_t)err)};
}

/// @internal Obtain a pointer to the beginning of the referred-to element's
/// value
mlib_constexpr const bson_byte* _bson_iterator_value_ptr(bson_iterator iter) mlib_noexcept {
    return iter._ptr + 1 + iter._keylen + 1;
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
    mlib_math_catch(_) { return -(int)BSON_ITER_INVALID_LENGTH; }
    // There MUST be two more null terminators (the one after the opt
    // string, and the one at the end of the doc itself), so
    // 'trailing_bytes' must be at least two.
    if (trailing_bytes_remain < 2) {
        return -(int)BSON_ITER_SHORT_READ;
    }
    // Advance past the option string's nul
    opt_len += 1;
    // This is the value's length
    int32_t ret = mlibMathNonNegativeInt32(add(I(rx_len), I(opt_len)));
    mlib_math_catch(_) { return -(int)BSON_ITER_INVALID_LENGTH; }
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
    static const int32_t const_sizes[256] = {
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
    static const bool varsize_pick[256] = {
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
            return -BSON_ITER_INVALID_LENGTH;
        }
        // The length of the value given by the length prefix:
        const int32_t varlen = _bson_read_int32_le(valptr);
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
    if (BV_LIKELY(tag == BSON_TYPE_REGEX)) {
        // Oops! We're actually reading a regular expression, which is designed
        // above to trigger the overrun check so that we can do something
        // different to calculate its size:
        return _bson_value_re_len((const char*)valptr, val_maxlen);
    }
    if (const_size == INT32_MAX) {
        // We indexed into the const_sizes table at some other invalid type
        return -BSON_ITER_INVALID_TYPE;
    }
    // We indexed into a valid type and know the length that it should be, but we
    // do not have enough in val_maxlen to hold the value that was requested.
    // Indicate an overrun by returning an error
    return -BSON_ITER_INVALID_LENGTH;
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
        return _bson_iterator_error(BSON_ITER_SHORT_READ);
    }

    // Assume: ValMaxLen > 0
    BV_ASSERT(val_maxlen > 0);

    bool need_check_size = true;
    if (val_maxlen > 16) {
        // val_maxlen is larger than all fixed-sized BSON value objects. If we are
        // parsing a fixed-size object, we can skip size checking
        need_check_size = !(
            type == BSON_TYPE_NULL || type == BSON_TYPE_UNDEFINED || type == BSON_TYPE_TIMESTAMP
            || type == BSON_TYPE_DATE_TIME || type == BSON_TYPE_DOUBLE || type == BSON_TYPE_BOOL
            || type == BSON_TYPE_DECIMAL128 || type == BSON_TYPE_INT32 || type == BSON_TYPE_INT64);
    }

    if (need_check_size) {
        const char* const valptr = keyptr + (keylen + 1);

        const int32_t vallen = _bson_valsize(type, (const bson_byte*)valptr, val_maxlen);
        if (BV_UNLIKELY(vallen < 0)) {
            return _bson_iterator_error((enum bson_iterator_error_cond)(-vallen));
        }
    }
    return mlib_init(bson_iterator){
        ._ptr    = data,
        ._keylen = keylen,
        ._rlen   = maxlen,
    };
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
 * @brief Obtain the byte-size of the pointed-to element
 *
 * @param it An iterator pointing to an element. Must not be an end/error
 * iterator
 * @return int32_t The size of the element, in bytes. This includes the tag,
 * the key, and the value data.
 */
mlib_constexpr int32_t bson_iterator_data_size(const bson_iterator it) mlib_noexcept {
    BV_ASSERT(!bson_iterator_done(it));
    const int32_t val_offset = 1  // The type
        + it._keylen              // The key
        + 1;                      // The nul after the key
    BV_ASSERT(val_offset > 0);
    const int32_t valsize = _bson_valsize(bson_iterator_type(it), it._ptr + val_offset, INT32_MAX);
    const int32_t remain  = INT32_MAX - valsize;
    BV_ASSERT(val_offset < remain);
    return val_offset + valsize;
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
    const int32_t          skip   = bson_iterator_data_size(it);
    const bson_byte* const newptr = bson_iterator_data(it) + skip;
    const int32_t          remain = it._rlen - skip;
    BV_ASSERT(remain >= 0);
    return _bson_iterator_at(newptr, remain);
}

/**
 * @brief Obtain a bson_view for the given document-like entity.
 *
 * This can be used with @ref bson_view or @ref bson_mut.
 */
#define bson_view_of(...) _bson_view_from_ptr(bson_data(__VA_ARGS__))

mlib_constexpr bson_view _bson_view_from_ptr(const bson_byte* p) mlib_noexcept {
    int32_t len = _bson_read_int32_le(p);
    BV_ASSERT(len >= 0);
    return bson_view_from_data(p, (uint32_t)len, NULL);
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
#define bson_begin(...) _bson_begin(bson_view_of(__VA_ARGS__))

mlib_constexpr bson_iterator _bson_begin(bson_view v) mlib_noexcept {
    BV_ASSERT(v._bson_document_data);
    const ptrdiff_t tailsize = bson_size(v) - sizeof(int32_t);
    BV_ASSERT(tailsize > 0);
    BV_ASSERT(tailsize < INT32_MAX);
    return _bson_iterator_at(bson_data(v) + sizeof(int32_t), (int32_t)tailsize);
}

/**
 * @brief Obtain a past-the-end "done" iterator for the given document `v`
 *
 * @param v A non-null bson_view or bson_mut
 * @return bson_iterator A new iterator referring to the end of `v`.
 *
 * The returned iterator does not refer to any element of the document. The
 * returned iterator will be considered "done" by @ref bson_iterator_done(). The
 * returned iterator is valid for as long as `v` would be valid.
 */
#define bson_end(...) _bson_end(bson_view_of(__VA_ARGS__))

mlib_constexpr bson_iterator _bson_end(bson_view v) mlib_noexcept {
    BV_ASSERT(bson_data(v));
    return _bson_iterator_at(bson_data(v) + bson_size(v) - 1, 1);
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
 * @brief Obtain the value of a IEE754 double-precision floating point in the
 * element referred-to by the given iterator.
 *
 * @param it A non-error iterator
 * @return double If `it` refers to an element with type DOUBLE, returns the
 * value contained in that element, otherwise returns zero.
 */
mlib_constexpr double bson_iterator_double(bson_iterator it) mlib_noexcept {
    if (bson_iterator_type(it) != BSON_TYPE_DOUBLE) {
        return 0.0;
    }
    uint64_t bytes = (uint64_t)_bson_read_int64_le(_bson_iterator_value_ptr(it));
    double   d     = 0;
    memcpy(&d, &bytes, sizeof bytes);
    return d;
}

mlib_constexpr bson_utf8_view _bson_read_stringlike_at(const bson_byte* p) mlib_noexcept {
    const int32_t len = _bson_read_int32_le(p);
    // String length checks were already performed by our callers
    BV_ASSERT(len >= 1);
    return mlib_init(bson_utf8_view){(const char*)(p + sizeof len), (uint32_t)len - 1};
}

mlib_constexpr bson_utf8_view _bson_iterator_stringlike(bson_iterator it) mlib_noexcept {
    const bson_byte* after_key = _bson_iterator_value_ptr(it);
    bson_utf8_view   r         = _bson_read_stringlike_at(_bson_iterator_value_ptr(it));
    BV_ASSERT(r.len > 0);
    BV_ASSERT(r.len < it._rlen);
    return r;
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
mlib_constexpr bson_utf8_view bson_iterator_utf8(bson_iterator it) mlib_noexcept {
    if (bson_iterator_type(it) != BSON_TYPE_UTF8) {
        return mlib_init(bson_utf8_view){NULL};
    }
    return _bson_iterator_stringlike(it);
}

/**
 * @brief Obtain a view of the BSON document referred to by the given iterator.
 *
 * If the iterator does not refer to a document/array element, returns @ref
 * BSON_VIEW_NULL
 *
 * @param it A valid iterator referring to an element of a bson_view
 * @return bson_view A view derived from the bson_view being iterated with `it`.
 */
mlib_constexpr bson_view
bson_iterator_document(bson_iterator it, enum bson_view_invalid_reason* error) mlib_noexcept {
    BV_ASSERT(!bson_iterator_done(it));
    if (bson_iterator_type(it) != BSON_TYPE_DOCUMENT && bson_iterator_type(it) != BSON_TYPE_ARRAY) {
        return BSON_VIEW_NULL;
    }
    const bson_byte* const valptr     = _bson_iterator_value_ptr(it);
    const int64_t          val_offset = (valptr - it._ptr);
    const int64_t          val_remain = it._rlen - val_offset;
    BV_ASSERT(val_remain > 0);
    return bson_view_from_data(valptr, (size_t)val_remain, error);
}

typedef struct bson_binary {
    const bson_byte* data;
    uint32_t         data_len;
    uint8_t          subtype;
} bson_binary;

mlib_constexpr bson_binary bson_iterator_binary(bson_iterator it) mlib_noexcept {
    BV_ASSERT(!bson_iterator_done(it));
    if (bson_iterator_type(it) != BSON_TYPE_BINARY) {
        return mlib_init(bson_binary){0};
    }
    const bson_byte* const valptr = _bson_iterator_value_ptr(it);
    const int32_t          size   = _bson_read_int32_le(valptr);
    BV_ASSERT(size >= 0);
    const uint8_t          subtype = valptr[4].v;
    const bson_byte* const p       = valptr + 5;
    return mlib_init(bson_binary){p, (uint32_t)size, subtype};
}

typedef struct bson_oid {
    uint8_t bytes[12];
} bson_oid;

mlib_constexpr bson_oid bson_iterator_oid(bson_iterator it) mlib_noexcept {
    BV_ASSERT(!bson_iterator_done(it));
    if (bson_iterator_type(it) != BSON_TYPE_OID) {
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
    if (bson_iterator_type(it) != BSON_TYPE_BOOL) {
        return false;
    }
    return _bson_iterator_value_ptr(it)[0].v != 0;
}

mlib_constexpr int64_t bson_iterator_datetime(bson_iterator it) mlib_noexcept {
    if (bson_iterator_type(it) != BSON_TYPE_DATE_TIME) {
        return 0;
    }
    return _bson_read_int64_le(_bson_iterator_value_ptr(it));
}

typedef struct bson_regex {
    const char* regex;
    int32_t     regex_len;
    const char* options;
    int32_t     options_len;
} bson_regex;

mlib_constexpr bson_regex bson_iterator_regex(bson_iterator it) mlib_noexcept {
    const bson_regex null_regex = {0};

    if (bson_iterator_type(it) != BSON_TYPE_REGEX) {
        return null_regex;
    }
    const bson_byte* const p = _bson_iterator_value_ptr(it);
    mlib_math_try();
    const char* const regex   = (const char*)p;
    const int64_t     rx_len  = mlibMath(cast(int64_t, strlen32(regex)));
    const char* const options = regex + rx_len + 1;
    const int64_t     opt_len = mlibMath(cast(int64_t, strlen32(options)));
    mlib_math_catch(_) { return null_regex; }
    return mlib_init(bson_regex){regex, (int32_t)rx_len, options, (int32_t)opt_len};
}

typedef struct bson_dbpointer {
    const char* collection;
    uint32_t    collection_len;
    bson_oid    object_id;
} bson_dbpointer;

mlib_constexpr bson_dbpointer bson_iterator_dbpointer(bson_iterator it) mlib_noexcept {
    const bson_dbpointer null_dbp = {0};
    if (bson_iterator_type(it) != BSON_TYPE_DBPOINTER) {
        return null_dbp;
    }
    const bson_byte* const p              = _bson_iterator_value_ptr(it);
    const int32_t          coll_name_size = _bson_read_int32_le(p);
    bson_dbpointer         ret;
    ret.collection = (const char*)p + 4;
    BV_ASSERT(coll_name_size > 0);
    ret.collection_len = (uint32_t)coll_name_size - 1;
    memcpy(&ret.object_id.bytes, p + 4 + coll_name_size, sizeof(bson_oid));
    return ret;
}

mlib_constexpr bson_utf8_view bson_iterator_code(bson_iterator it) mlib_noexcept {
    if (bson_iterator_type(it) != BSON_TYPE_CODE) {
        return mlib_init(bson_utf8_view){0};
    }
    return _bson_iterator_stringlike(it);
}

mlib_constexpr bson_utf8_view bson_iterator_symbol(bson_iterator it) mlib_noexcept {
    if (bson_iterator_type(it) != BSON_TYPE_SYMBOL) {
        return mlib_init(bson_utf8_view){0};
    }
    return _bson_iterator_stringlike(it);
}

mlib_constexpr int32_t bson_iterator_int32(bson_iterator it) mlib_noexcept {
    if (bson_iterator_type(it) != BSON_TYPE_INT32) {
        return 0;
    }
    return _bson_read_int32_le(_bson_iterator_value_ptr(it));
}

mlib_constexpr int64_t bson_iterator_int64(bson_iterator it) mlib_noexcept {
    if (bson_iterator_type(it) != BSON_TYPE_INT64) {
        return 0;
    }
    return _bson_read_int64_le(_bson_iterator_value_ptr(it));
}

/**
 * @brief Compare a bson_iterator's key with a string
 *
 * @param it An iterator pointing to an element under consideration
 * @param key The key against which to compare
 * @retval true If the element's key is byte-wise equivalent to `key`
 * @retval false Otherwise
 */
mlib_constexpr bool bson_key_eq(const bson_iterator it, const char* key, int keylen) mlib_noexcept {
    const bson_utf8_view k = bson_iterator_key(it);
    BV_ASSERT(it._keylen >= 0);
    if (keylen < 0)
        keylen = mlib_strlen(key);
    return (uint32_t)k.len == mlib_strnlen(key, keylen)
        && memcmp(k.data, key, (uint32_t)it._keylen) == 0;
}

/**
 * @brief Find the first element within a document that has the given key
 *
 * @param v A document to inspect
 * @param key The key to search for
 * @return bson_iterator An iterator pointing to the found element, OR the
 * done-iterator if no element was found, OR an errant iterator if a parsing
 * error occured.
 */
#define bson_find(View, Key) _bson_find(bson_view_of(Doc), Key, strlen(Key))

mlib_constexpr bson_iterator _bson_find(bson_view v, const char* key, int keylen) mlib_noexcept {
    bson_iterator it = bson_begin(v);
    for (; !bson_iterator_done(it); it = bson_next(it)) {
        if (bson_key_eq(it, key, keylen)) {
            break;
        }
    }
    return it;
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
   for (const bson_iterator Sentinel = LastInit; OuterOnce; ) \
   /* Keep a 'DidBreak' variable that detects if the loop body executed a 'break' statement' */ \
   for (bool DidBreak = false; OuterOnce; \
        /* If we hit this loopstep statement, the whole foreach is finished: */ \
        OuterOnce = 0) \
   /* Create the actual iterator (whose name is private). */ \
   for (bson_iterator Iterator = FirstInit; \
        /* Continually advance until we hit the sentinel, or 'DidBreak' is true */ \
        !DidBreak && !bson_iterator_eq(Iterator, Sentinel); \
        Iterator = bson_next(Iterator)) \
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

mlib_extern_c_end();

#if mlib_is_cxx()
struct bson_iterator_error : std::runtime_error {
    bson_iterator_error() noexcept
        : std::runtime_error("Invalid element in BSON document data") {}

    [[nodiscard]] virtual bson_iterator_error_cond cond() const noexcept = 0;
};

template <bson_iterator_error_cond Cond>
struct bson_iterator_error_of : bson_iterator_error {
    [[nodiscard]] bson_iterator_error_cond cond() const noexcept override { return Cond; }
};

struct bson_view_error : std::runtime_error {
    bson_view_error() noexcept
        : std::runtime_error("Invalid BSON document data") {}

    [[nodiscard]] virtual bson_view_invalid_reason reason() const noexcept = 0;
};

template <bson_view_invalid_reason R>
struct bson_view_error_of : bson_view_error {
    [[nodiscard]] bson_view_invalid_reason reason() const noexcept { return R; }
};

class bson_iterator::reference {
    bson_iterator _iter;
    friend ::bson_iterator;

    explicit reference(bson_iterator it)
        : _iter(it) {}

public:
    [[nodiscard]] bson_type type() const noexcept { return bson_iterator_type(_iter); }

    [[nodiscard]] std::string_view key() const noexcept {
        return std::string_view(bson_iterator_key(_iter));
    }
    [[nodiscard]] double         double_() const noexcept { return bson_iterator_double(_iter); }
    [[nodiscard]] bson_utf8_view as_utf8() const noexcept { return bson_iterator_utf8(_iter); }
    [[nodiscard]] std::int32_t   int32() const noexcept { return bson_iterator_int32(_iter); }
    [[nodiscard]] std::int64_t   int64() const noexcept { return bson_iterator_int64(_iter); }
    [[nodiscard]] bool           boolean() const noexcept { return bson_iterator_bool(_iter); }

    [[nodiscard]] double as_double() const noexcept {
        switch (type()) {
        case BSON_TYPE_DOUBLE:
            return double_();
        case BSON_TYPE_INT32:
            return double(int32());
        case BSON_TYPE_INT64:
            return double(int64());
        case BSON_TYPE_BOOL:
            return boolean() ? 1.0 : 0.0;
        case BSON_TYPE_EOD:
        case BSON_TYPE_UTF8:
        case BSON_TYPE_DOCUMENT:
        case BSON_TYPE_ARRAY:
        case BSON_TYPE_BINARY:
        case BSON_TYPE_UNDEFINED:
        case BSON_TYPE_OID:
        case BSON_TYPE_DATE_TIME:
        case BSON_TYPE_NULL:
        case BSON_TYPE_REGEX:
        case BSON_TYPE_DBPOINTER:
        case BSON_TYPE_CODE:
        case BSON_TYPE_SYMBOL:
        case BSON_TYPE_CODEWSCOPE:
        case BSON_TYPE_TIMESTAMP:
        case BSON_TYPE_DECIMAL128:
        case BSON_TYPE_MAXKEY:
        case BSON_TYPE_MINKEY:
            return 0;
        default:
            BV_ASSERT(false);
            std::terminate();
        }
    }

    [[nodiscard]] bool as_boolean() const noexcept {
        switch (type()) {
        case BSON_TYPE_UNDEFINED:
        case BSON_TYPE_NULL:
        case BSON_TYPE_EOD:
        case BSON_TYPE_MAXKEY:
        case BSON_TYPE_MINKEY:
            return false;
        case BSON_TYPE_OID:
        case BSON_TYPE_DOCUMENT:
        case BSON_TYPE_ARRAY:
        case BSON_TYPE_BINARY:
        case BSON_TYPE_UTF8:
        case BSON_TYPE_DATE_TIME:
        case BSON_TYPE_DBPOINTER:
        case BSON_TYPE_REGEX:
        case BSON_TYPE_CODEWSCOPE:
        case BSON_TYPE_SYMBOL:
        case BSON_TYPE_TIMESTAMP:
        case BSON_TYPE_CODE:
        case BSON_TYPE_DECIMAL128:
            return true;
        case BSON_TYPE_DOUBLE:
        case BSON_TYPE_INT32:
        case BSON_TYPE_INT64:
            return double_() != 0;
        case BSON_TYPE_BOOL:
            return boolean();
        default:
            BV_ASSERT(false);
            std::terminate();
        }
    }

    [[nodiscard]] bson_view as_document() const {
        bson_view_invalid_reason error;
        auto                     v = bson_iterator_document(_iter, &error);
        switch (error) {
        case BSON_VIEW_OKAY:
            return v;
#define X(R)                                                                                       \
    case R:                                                                                        \
        throw bson_view_error_of<R>()
            X(BSON_VIEW_INVALID_HEADER);
            X(BSON_VIEW_INVALID_TERMINATOR);
            X(BSON_VIEW_SHORT_READ);
#undef X
        }
        abort();
    }

    [[nodiscard]] bson_view as_array() const {
        if (type() != BSON_TYPE_ARRAY) {
            return BSON_VIEW_NULL;
        }
        return as_document();
    }
};

class bson_iterator::arrow {
public:
    bson_iterator::reference _ref;

    const bson_iterator::reference* operator->() const noexcept { return &_ref; }
};

std::size_t bson_iterator::data_size() const noexcept { return bson_iterator_data_size(*this); }

bson_iterator::reference bson_iterator::operator*() const {
    throw_if_error();
    return reference(*this);
}

bson_iterator::arrow bson_iterator::operator->() const noexcept { return arrow{**this}; }

constexpr bool bson_iterator::operator==(bson_iterator other) const noexcept {
    return bson_iterator_eq(*this, other);
}

bson_iterator_error_cond bson_iterator::error() const noexcept {
    return bson_iterator_get_error(*this);
}

void bson_iterator::throw_if_error() const {
    switch (this->error()) {
    case BSON_ITER_NO_ERROR:
        return;
#define X(Cond)                                                                                    \
    case Cond:                                                                                     \
        throw bson_iterator_error_of<Cond>()
        X(BSON_ITER_SHORT_READ);
        X(BSON_ITER_INVALID_LENGTH);
        X(BSON_ITER_INVALID_TYPE);
#undef X
    }
}

bson_iterator bson_view::begin() const noexcept { return bson_begin(*this); }

bson_iterator bson_view::end() const noexcept { return bson_end(*this); }

uint32_t bson_view::byte_size() const noexcept { return bson_size(*this); }

bson_iterator bson_view::find(std::string_view key) const noexcept {
    return _bson_find(*this, key.data(), static_cast<int>(key.size()));
}

inline bson_iterator& bson_iterator::operator++() noexcept {
    throw_if_error();
    return *this = bson_next(*this);
}

bool bson_iterator::done() const noexcept { return bson_iterator_done(*this); }

bson_view bson_view::from_data(const bson_byte* b, size_t datalen) {
    bson_view_invalid_reason error;
    auto                     v = bson_view_from_data(b, datalen, &error);
    switch (error) {
    case BSON_VIEW_OKAY:
        return v;
#define X(Reason)                                                                                  \
    case Reason:                                                                                   \
        throw bson_view_error_of<Reason>()
        X(BSON_VIEW_SHORT_READ);
        X(BSON_VIEW_INVALID_HEADER);
        X(BSON_VIEW_INVALID_TERMINATOR);
    }
#undef X
    __builtin_unreachable();
}
#endif

#undef BV_ASSERT
#undef BV_LIKELY
#undef BV_UNLIKELY

#pragma pop_macro("BSON_VIEW_CHECKED")
