#ifndef BSON_TYPES_H_INCLUDED
#define BSON_TYPES_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The type of the value of a BSON element within a document
 */
typedef enum bson_type {
   /// The special end-of-document zero byte
   BSON_TYPE_EOD = 0x00,
   /// IEEE754 double-precision floating point number
   BSON_TYPE_DOUBLE = 0x01,
   /// A UTF-8 encoded text string
   BSON_TYPE_UTF8 = 0x02,
   /// A nested BSON document
   BSON_TYPE_DOCUMENT = 0x03,
   /// A nested BSON document marked as an array
   BSON_TYPE_ARRAY = 0x04,
   /// A string of arbitrary bytes
   BSON_TYPE_BINARY = 0x05,
   /// An "undefined" value (deprecated)
   BSON_TYPE_UNDEFINED = 0x06,
   /// An ObjectID value
   BSON_TYPE_OID = 0x07,
   /// A boolean value
   BSON_TYPE_BOOL = 0x08,
   /// A 64bit UTC timestamp (milliseconds since the Unix Epoch)
   BSON_TYPE_DATE_TIME = 0x09,
   /// A null value
   BSON_TYPE_NULL = 0x0A,
   /// A regular expression
   BSON_TYPE_REGEX = 0x0B,
   /// DBPointer (deprecated)
   BSON_TYPE_DBPOINTER = 0x0C,
   /// JavaScript code
   BSON_TYPE_CODE = 0x0D,
   /// A symbol (Deprecated)
   BSON_TYPE_SYMBOL = 0x0E,
   /// JavaScript code with a scope (Deprecated)
   BSON_TYPE_CODEWSCOPE = 0x0F,
   /// A 32-bit signed integer
   BSON_TYPE_INT32 = 0x10,
   /// A MongoDB timestamp value
   BSON_TYPE_TIMESTAMP = 0x11,
   /// A 64-bit signed integer
   BSON_TYPE_INT64 = 0x12,
   /// An IEEE754 128-bit decimal floating point number
   BSON_TYPE_DECIMAL128 = 0x13,
   /// Max key sentinel
   BSON_TYPE_MAXKEY = 0x7F,
   /// Min key sentinel
   BSON_TYPE_MINKEY = 0xFF,
} bson_type;

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // BSON_TYPES_H_INCLUDED
