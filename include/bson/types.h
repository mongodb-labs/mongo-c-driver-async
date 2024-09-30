#pragma once

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
    bson_type_date_time = 0x09,
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
