#pragma once

#include "./value_ref.h"

#include <bson/byte.h>
#include <bson/doc.h>
#include <bson/iterator.h>
#include <bson/types.h>
#include <bson/view.h>

#include <mlib/alloc.h>
#include <mlib/config.h>
#include <mlib/str.h>

#if mlib_is_cxx()
#include <string_view>
#endif

/**
 * @brief Convert an arbitrary value into a `bson_value_ref`
 */
#define bson_as_value_ref(X)                                                                       \
    MLIB_IF_CXX(bson_value_ref::from((X))) MLIB_IF_NOT_CXX(_bsonAsValue((X)))

#if mlib_is_cxx()

namespace bson {

template <typename T>
concept as_value_convertbile = requires(const T& obj) { ::bson_as_value_ref(obj); };

}  // namespace bson

#endif  // C++

typedef struct bson_value {
    bson_type type;
    union {
        bson_eod eod;
        double   double_;
        // String for utf8, code, and symbol
        mlib_str string;
        // Document for documents and arrays
        bson_doc document;
        struct {
            bson_byte_vec bytes;
            uint8_t       subtype;
        } binary;
        bson_oid      oid;
        bool          bool_;
        bson_datetime datetime;
        struct {
            mlib_str rx;
            mlib_str options;
        } regex;
        struct {
            mlib_str collection;
            bson_oid object_id;
        } dbpointer;
        int32_t         int32;
        bson_timestamp  timestamp;
        int64_t         int64;
        bson_decimal128 decimal128;
    };

#if mlib_is_cxx()
    template <bson::as_value_convertbile T>
    constexpr bool operator==(const T& val) const noexcept;
#endif  // C++
} bson_value;

mlib_constexpr void bson_value_delete(bson_value val) mlib_noexcept {
    switch (val.type) {
    case bson_type_utf8:
    case bson_type_code:
    case bson_type_symbol:
        mlib_str_delete(val.string);
        return;
    case bson_type_document:
    case bson_type_array:
        bson_delete(val.document);
        return;
    case bson_type_binary:
        bson_byte_vec_delete(val.binary.bytes);
        return;
    case bson_type_dbpointer:
        mlib_str_delete(val.dbpointer.collection);
        return;
    case bson_type_eod:
    case bson_type_double:
    case bson_type_undefined:
    case bson_type_oid:
    case bson_type_bool:
    case bson_type_datetime:
    case bson_type_null:
        break;
    case bson_type_regex:
        mlib_str_delete(val.regex.rx);
        mlib_str_delete(val.regex.options);
        break;
    case bson_type_codewscope:
    case bson_type_int32:
    case bson_type_timestamp:
    case bson_type_int64:
    case bson_type_decimal128:
    case bson_type_maxkey:
    case bson_type_minkey:
        break;
    }
}

// Declare a C conversion function
#define DECL_CONVERSION(FromType, TypeEnumerator, Member, Init)                                    \
    inline bson_value_ref MLIB_PASTE(_bson_value_ref_from_, FromType)(FromType arg)                \
        mlib_noexcept {                                                                            \
        bson_value_ref ret;                                                                        \
        ret.type   = TypeEnumerator;                                                               \
        ret.Member = Init;                                                                         \
        return ret;                                                                                \
    }                                                                                              \
    mlib_static_assert(true, "")
DECL_CONVERSION(double, bson_type_double, double_, arg);
DECL_CONVERSION(mlib_str, bson_type_utf8, utf8, mlib_as_str_view(arg));
DECL_CONVERSION(mlib_str_mut, bson_type_utf8, utf8, mlib_as_str_view(arg));
DECL_CONVERSION(mlib_str_view, bson_type_utf8, utf8, arg);
DECL_CONVERSION(bson_view, bson_type_document, document, arg);
DECL_CONVERSION(bson_doc, bson_type_document, document, bson_as_view(arg));
DECL_CONVERSION(bson_mut, bson_type_document, document, bson_as_view(arg));
DECL_CONVERSION(bson_array_view, bson_type_array, array, arg);
DECL_CONVERSION(bson_binary_view, bson_type_binary, binary, arg);
DECL_CONVERSION(bson_oid, bson_type_oid, oid, arg);
DECL_CONVERSION(bool, bson_type_bool, bool_, arg);
DECL_CONVERSION(bson_datetime, bson_type_datetime, datetime, arg);
DECL_CONVERSION(bson_regex_view, bson_type_regex, regex, arg);
DECL_CONVERSION(bson_dbpointer_view, bson_type_dbpointer, dbpointer, arg);
DECL_CONVERSION(bson_code_view, bson_type_code, code, arg);
DECL_CONVERSION(bson_symbol_view, bson_type_symbol, symbol, arg);
DECL_CONVERSION(int32_t, bson_type_int32, int32, arg);
DECL_CONVERSION(bson_timestamp, bson_type_timestamp, timestamp, arg);
DECL_CONVERSION(int64_t, bson_type_int64, int64, arg);
DECL_CONVERSION(bson_decimal128, bson_type_decimal128, decimal128, arg);
// From C string
inline bson_value_ref _bson_value_ref_from_cstring(const char* s) mlib_noexcept {
    return _bson_value_ref_from_mlib_str_view(mlib_as_str_view(s));
}
mlib_constexpr bson_value_ref _bson_value_ref_dup(bson_value_ref ref) mlib_noexcept { return ref; }
mlib_constexpr bson_value_ref _bson_value_ref_from_value(bson_value val) mlib_noexcept {
    bson_value_ref ret;
    ret.type = val.type;
    switch (val.type) {
    case bson_type_eod:
    case bson_type_undefined:
    case bson_type_null:
    case bson_type_maxkey:
    case bson_type_minkey:
        break;
    case bson_type_double:
        ret.double_ = val.double_;
        break;
    case bson_type_code:
    case bson_type_symbol:
    case bson_type_utf8:
        ret.utf8 = mlib_as_str_view(val.string);
        break;
    case bson_type_document:
    case bson_type_array:
        ret.document = bson_as_view(val.document);
        break;
    case bson_type_binary:
        ret.binary.subtype  = val.binary.subtype;
        ret.binary.data     = val.binary.bytes.data;
        ret.binary.data_len = val.binary.bytes.size;
        break;
    case bson_type_oid:
        ret.oid = val.oid;
        break;
    case bson_type_bool:
        ret.bool_ = val.bool_;
        break;
    case bson_type_datetime:
        ret.datetime = val.datetime;
        break;
    case bson_type_regex:
    case bson_type_dbpointer:
        ret.dbpointer.collection = mlib_as_str_view(val.dbpointer.collection);
        ret.dbpointer.object_id  = val.dbpointer.object_id;
        break;
    case bson_type_codewscope:
        assert(false && "TODO: Code w/ scope");
        abort();
    case bson_type_int32:
        ret.int32 = val.int32;
        break;
    case bson_type_timestamp:
        ret.timestamp = val.timestamp;
        break;
    case bson_type_int64:
        ret.int64 = val.int64;
        break;
    case bson_type_decimal128:
        ret.decimal128 = val.decimal128;
        break;
    default:
        abort();
    }
    return ret;
}
#undef DECL_CONVERSION
#define _bsonAsValue(X)                                                                            \
    _Generic((X),                                                                                  \
        double: _bson_value_ref_from_double,                                                       \
        float: _bson_value_ref_from_double,                                                        \
        char*: _bson_value_ref_from_cstring,                                                       \
        const char*: _bson_value_ref_from_cstring,                                                 \
        mlib_str: _bson_value_ref_from_mlib_str,                                                   \
        mlib_str_view: _bson_value_ref_from_mlib_str_view,                                         \
        mlib_str_mut: _bson_value_ref_from_mlib_str_mut,                                           \
        bson_view: _bson_value_ref_from_bson_view,                                                 \
        bson_doc: _bson_value_ref_from_bson_doc,                                                   \
        bson_mut: _bson_value_ref_from_bson_mut,                                                   \
        bson_array_view: _bson_value_ref_from_bson_array_view,                                     \
        bson_binary_view: _bson_value_ref_from_bson_binary_view,                                   \
        bson_oid: _bson_value_ref_from_bson_oid,                                                   \
        bool: MLIB_PASTE(_bson_value_ref_from_, bool),                                             \
        bson_datetime: _bson_value_ref_from_bson_datetime,                                         \
        bson_regex_view: _bson_value_ref_from_bson_regex_view,                                     \
        bson_dbpointer_view: _bson_value_ref_from_bson_dbpointer_view,                             \
        bson_code_view: _bson_value_ref_from_bson_code_view,                                       \
        bson_symbol_view: _bson_value_ref_from_bson_symbol_view,                                   \
        int32_t: _bson_value_ref_from_int32_t,                                                     \
        bson_timestamp: _bson_value_ref_from_bson_timestamp,                                       \
        bson_decimal128: _bson_value_ref_from_bson_decimal128,                                     \
        int64_t: _bson_value_ref_from_int64_t,                                                     \
        bson_value: _bson_value_ref_from_value,                                                    \
        bson_value_ref: _bson_value_ref_dup)((X))

#define bson_value_copy(...)                                                                       \
    MLIB_PASTE(_bsonValueCopyArgc_, MLIB_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)
#define _bsonValueCopyArgc_1(X) _bson_value_copy(bson_as_value_ref((X)), mlib_default_allocator)
#define _bsonValueCopyArgc_2(X, Alloc) _bson_value_copy(bson_as_value_ref((X)), (Alloc))
static mlib_constexpr bson_value _bson_value_copy(bson_value_ref val,
                                                  mlib_allocator alloc) mlib_noexcept {
    bson_value ret;
    ret.type = val.type;
    switch (val.type) {
    case bson_type_eod:
    case bson_type_undefined:
    case bson_type_null:
    case bson_type_maxkey:
    case bson_type_minkey:
        break;
    case bson_type_double:
        ret.double_ = val.double_;
        break;
    case bson_type_utf8:
        ret.string = mlib_str_copy(val.utf8).str;
        if (!ret.string.data) {
            ret.type = bson_type_eod;
            return ret;
        }
        break;
    case bson_type_document:
    case bson_type_array:
        ret.document = bson_new(val.document);
        if (!bson_data(ret.document)) {
            ret.type = bson_type_eod;
            return ret;
        }
        break;
    case bson_type_binary: {
        bool alloc_okay  = false;
        ret.binary.bytes = bson_byte_vec_new_n(val.binary.data_len, &alloc_okay, alloc);
        if (!alloc_okay) {
            ret.type = bson_type_eod;
            return ret;
        }
        ret.binary.subtype = val.binary.subtype;
        break;
    }
    case bson_type_oid:
        ret.oid = val.oid;
        break;
    case bson_type_bool:
        ret.bool_ = val.bool_;
        break;
    case bson_type_datetime:
        ret.datetime = val.datetime;
        break;
    case bson_type_regex:
        ret.regex.rx = mlib_str_copy(val.regex.regex).str;
        if (!ret.regex.rx.data) {
            ret.type = bson_type_eod;
            return ret;
        }
        ret.regex.options = mlib_str_copy(val.regex.options).str;
        if (!ret.regex.options.data) {
            mlib_str_delete(ret.regex.rx);
            ret.type = bson_type_eod;
            return ret;
        }
        break;
    case bson_type_dbpointer:
        ret.dbpointer.collection = mlib_str_copy(val.dbpointer.collection).str;
        if (!ret.dbpointer.collection.data) {
            ret.type = bson_type_eod;
            return ret;
        }
        ret.dbpointer.object_id = val.dbpointer.object_id;
        break;
    case bson_type_code:
        ret.string = mlib_str_copy(val.code.utf8).str;
        if (!ret.string.data) {
            ret.type = bson_type_eod;
            return ret;
        }
        break;
    case bson_type_symbol:
        ret.string = mlib_str_copy(val.symbol.utf8).str;
        if (!ret.string.data) {
            ret.type = bson_type_eod;
            return ret;
        }
        break;
    case bson_type_int32:
        ret.int32 = val.int32;
        break;
    case bson_type_timestamp:
        ret.timestamp = val.timestamp;
        break;
    case bson_type_int64:
        ret.int64 = val.int64;
        break;
    case bson_type_decimal128:
        ret.decimal128 = val.decimal128;
        break;
    case bson_type_codewscope:
        assert(false && "Do not use code_w_scope");
        abort();
    }
    return ret;
}

#define T bson_value
#define VecDestroyElement bson_value_delete
#include <mlib/vec.t.h>

#if mlib_is_cxx()
constexpr bson_value_ref bson_value_ref::from(bson_value const& v) noexcept {
    return _bson_value_ref_from_value(v);
}

template <bson::as_value_convertbile T>
constexpr bool bson_value::operator==(const T& val) const noexcept {
    return bson_as_value_ref(*this) == val;
}
#endif  // C++
