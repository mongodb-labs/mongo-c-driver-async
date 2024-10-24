#pragma once

#include <bson/byte.h>

#include <mlib/config.h>
#include <mlib/str.h>

#if mlib_is_cxx()
#include <span>
#endif

/**
 * @brief Declares a X-macro invocation list for each BSON type. It has the following signature:
 *
 * X(<octet>, <view type>, <owner type>, <basename>, <safename>)
 */
#define BSON_TYPE_X_LIST                                                                           \
    X(0, bson_eod, bson_eod, eod, eod)                                                             \
    X(1, double, double, double, double_)                                                          \
    X(2, mlib_str_view, mlib_str, utf8, utf8)                                                      \
    X(3, bson_view, bson_doc, document, document)                                                  \
    X(4, bson_array_view, bson_array, array, array)                                                \
    X(5, bson_binary_view, bson_binary, binary, binary)                                            \
    X(6, bson_undefined, bson_undefined, undefined, undefined)                                     \
    X(7, bson_oid, bson_oid, oid, oid)                                                             \
    X(8, bool, bool, bool, bool_)                                                                  \
    X(9, bson_datetime, bson_datetime, datetime, datetime)                                         \
    X(10, bson_null, bson_null, null, null)                                                        \
    X(11, bson_regex_view, bson_regex, regex, regex)                                               \
    X(12, bson_dbpointer_view, bson_dbpointer, dbpointer, dbpointer)                               \
    X(13, bson_code_view, bson_code, code, code)                                                   \
    X(14, bson_symbol_view, bson_symbol, symbol, symbol)                                           \
    X(16, int32_t, int32_t, int32, int32)                                                          \
    X(17, bson_timestamp, bson_timestamp, timestamp, timestamp)                                    \
    X(18, int64_t, int64_t, int64, int64)                                                          \
    X(19, bson_decimal128, bson_decimal128, decimal128, decimal128)                                \
    X(0x7f, bson_maxkey, bson_maxkey, maxkey, maxkey)                                              \
    X(0xff, bson_minkey, bson_minkey, minkey, minkey)

/**
 * @brief The type of the value of a BSON element within a document
 */
typedef enum bson_type {
    /// The special end-of-document zero byte
    bson_type_eod = 0x00,
    /// IEEE754 double-precision floating point number
    bson_type_double = 0x01,
    /// A UTF-8 encoded text string
    bson_type_utf8 = 0x02,
    /// A nested BSON document
    bson_type_document = 0x03,
    /// A nested BSON document marked as an array
    bson_type_array = 0x04,
    /// A string of arbitrary bytes
    bson_type_binary = 0x05,
    /// An "undefined" value (deprecated)
    bson_type_undefined = 0x06,
    /// An ObjectID value
    bson_type_oid = 0x07,
    /// A boolean value
    bson_type_bool = 0x08,
    /// A 64bit UTC timestamp (milliseconds since the Unix Epoch)
    bson_type_datetime = 0x09,
    /// A null value
    bson_type_null = 0x0A,
    /// A regular expression
    bson_type_regex = 0x0B,
    /// DBPointer (deprecated)
    bson_type_dbpointer = 0x0C,
    /// JavaScript code
    bson_type_code = 0x0D,
    /// A symbol (Deprecated)
    bson_type_symbol = 0x0E,
    /// JavaScript code with a scope (Deprecated)
    bson_type_codewscope = 0x0F,
    /// A 32-bit signed integer
    bson_type_int32 = 0x10,
    /// A MongoDB timestamp value
    bson_type_timestamp = 0x11,
    /// A 64-bit signed integer
    bson_type_int64 = 0x12,
    /// An IEEE754 128-bit decimal floating point number
    bson_type_decimal128 = 0x13,
    /// Max key sentinel
    bson_type_maxkey = 0x7F,
    /// Min key sentinel
    bson_type_minkey = 0xFF,
} bson_type;

/**
 * @brief This is a special fake value type that is used at compile-time to disambiguate
 * the special "no value" of a bson_value/bson_value_ref. It will never actually be
 * read from or written into a BSON object.
 */
typedef struct bson_eod {
    mlib_empty_aggregate_c_compat;
    MLIB_IF_CXX(bool operator==(const bson_eod&) const = default;)
} bson_eod;

typedef struct bson_undefined {
    mlib_empty_aggregate_c_compat;
    MLIB_IF_CXX(bool operator==(const bson_undefined&) const = default;)
} bson_undefined;

typedef struct bson_null {
    mlib_empty_aggregate_c_compat;
    MLIB_IF_CXX(bool operator==(const bson_null&) const = default;)
} bson_null;

typedef struct bson_maxkey {
    mlib_empty_aggregate_c_compat;
    MLIB_IF_CXX(bool operator==(const bson_maxkey&) const = default;)
} bson_maxkey;

typedef struct bson_minkey {
    mlib_empty_aggregate_c_compat;
    MLIB_IF_CXX(bool operator==(const bson_minkey&) const = default;)
} bson_minkey;

typedef struct bson_binary_view {
    const bson_byte* data;
    uint32_t         data_len;
    uint8_t          subtype;
#if mlib_is_cxx()
    constexpr std::span<const bson_byte> bytes_span() const noexcept {
        return std::span{data, data_len};
    }
    MLIB_IF_CXX(bool operator==(const bson_binary_view&) const noexcept;)
#endif  //
} bson_binary_view;

typedef struct bson_oid {
    uint8_t bytes[12];
    MLIB_IF_CXX(bool operator==(const bson_oid&) const = default;)
} bson_oid;
#define BSON_OID_ZERO (mlib_init(bson_oid){{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}})

typedef struct bson_datetime {
    int64_t utc_ms_offset;
    MLIB_IF_CXX(bool operator==(const bson_datetime&) const = default;)
} bson_datetime;

typedef struct bson_regex_view {
    mlib_str_view regex;
    mlib_str_view options;
    MLIB_IF_CXX(bool operator==(const bson_regex_view&) const = default;)
} bson_regex_view;

#define BSON_REGEX_NULL (mlib_init(bson_regex_view){mlib_str_view_null, mlib_str_view_null})

typedef struct bson_dbpointer_view {
    mlib_str_view collection;
    bson_oid      object_id;
    MLIB_IF_CXX(bool operator==(const bson_dbpointer_view&) const = default;)
} bson_dbpointer_view;

#define BSON_DBPOINTER_NULL (mlib_init(bson_dbpointer_view){NULL, 0, BSON_OID_ZERO})

typedef struct bson_code_view {
    mlib_str_view utf8;
    MLIB_IF_CXX(bool operator==(const bson_code_view&) const = default;)
} bson_code_view;

typedef struct bson_symbol_view {
    mlib_str_view utf8;
    MLIB_IF_CXX(bool operator==(const bson_symbol_view&) const = default;)
} bson_symbol_view;

typedef struct bson_timestamp {
    uint32_t increment;
    uint32_t utc_sec_offset;
    MLIB_IF_CXX(bool operator==(const bson_timestamp&) const = default;)
} bson_timestamp;

typedef struct bson_decimal128 {
    uint8_t bytes[16];
    MLIB_IF_CXX(bool operator==(const bson_decimal128&) const = default;)
} bson_decimal128;

#if mlib_is_cxx()
namespace bson {
using null      = ::bson_null;
using undefined = ::bson_undefined;
using minkey    = ::bson_minkey;
using maxkey    = ::bson_maxkey;

}  // namespace bson
#endif
