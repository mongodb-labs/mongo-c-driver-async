/**
 * @file iter.h
 * @brief Implementation details for document iteration
 * @date 2024-10-07
 */

#pragma once

#include <bson/byte.h>
#include <bson/detail/assert.h>
#include <bson/detail/mem.h>
#include <bson/iter_errc.h>
#include <bson/types.h>

#include <mlib/config.h>
#include <mlib/integer.h>

#include <stdint.h>

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
