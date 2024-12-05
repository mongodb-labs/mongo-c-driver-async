#define VecDeclareExternInlines_bson_value_vec

#include <bson/value.h>

#define DECL_CONVERSION(FromType, TypeEnumerator, Member, Init)                                    \
    extern inline bson_value_ref MLIB_PASTE(_bson_value_ref_from_, FromType)(FromType arg)         \
        mlib_noexcept

DECL_CONVERSION(double, bson_type_double, double_, arg);
DECL_CONVERSION(mlib_str, bson_type_utf8, utf8, mlib_str_view_from(arg));
DECL_CONVERSION(mlib_str_mut, bson_type_utf8, utf8, mlib_str_view_from(arg));
DECL_CONVERSION(mlib_str_view, bson_type_utf8, utf8, arg);
DECL_CONVERSION(bson_view, bson_type_document, document, arg);
DECL_CONVERSION(bson_doc, bson_type_document, document, bson_view_from(arg));
DECL_CONVERSION(bson_mut, bson_type_document, document, bson_view_from(arg));
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

extern inline bson_value_ref         _bson_value_ref_from_cstring(const char* s) mlib_noexcept;
extern mlib_constexpr bson_value_ref _bson_value_ref_dup(bson_value_ref ref) mlib_noexcept;
extern mlib_constexpr bson_value_ref _bson_value_ref_from_value(bson_value val) mlib_noexcept;

extern mlib_constexpr bool    bson_value_as_bool(bson_value_ref it) mlib_noexcept;
extern mlib_constexpr int32_t bson_value_as_int32(bson_value_ref it, bool*) mlib_noexcept;
extern mlib_constexpr int64_t bson_value_as_int64(bson_value_ref it, bool*) mlib_noexcept;
extern mlib_constexpr double  bson_value_as_double(bson_value_ref it, bool*) mlib_noexcept;

extern mlib_constexpr void bson_value_delete(bson_value val) mlib_noexcept;
