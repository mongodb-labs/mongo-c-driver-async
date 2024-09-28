#include <amongoc/bson/view.h>

#include <mlib/config.h>

#include <string.h>

extern mlib_constexpr int32_t _bson_read_int32_le(const bson_byte* bytes) mlib_noexcept;

extern mlib_constexpr int64_t _bson_read_int64_le(const bson_byte* bytes) mlib_noexcept;

extern mlib_constexpr uint32_t _bson_byte_size(const bson_byte*) mlib_noexcept;

extern mlib_constexpr const bson_byte* _bson_data_as_const(const bson_byte*) mlib_noexcept;

extern mlib_constexpr bson_byte* _bson_data_as_mut(bson_byte*) mlib_noexcept;

extern mlib_constexpr bson_view bson_view_from_data(const bson_byte* const               data,
                                                    const size_t                         len,
                                                    enum bson_view_invalid_reason* const error)
    mlib_noexcept;

extern mlib_constexpr bson_iterator _bson_iterator_at(const bson_byte* const data,
                                                      int32_t bytes_remaining) mlib_noexcept;

extern mlib_constexpr bson_iterator bson_next(const bson_iterator it) mlib_noexcept;

extern inline bson_utf8_view bson_iterator_key(bson_iterator it) mlib_noexcept;

extern mlib_constexpr bson_type bson_iterator_type(bson_iterator it) mlib_noexcept;

extern mlib_constexpr const bson_byte* _bson_iterator_value_ptr(bson_iterator iter) mlib_noexcept;

extern mlib_constexpr const bson_byte* bson_iterator_data(const bson_iterator) mlib_noexcept;

extern mlib_constexpr int32_t bson_iterator_data_size(const bson_iterator) mlib_noexcept;

extern mlib_constexpr bson_iterator _bson_begin(bson_view v) mlib_noexcept;

extern mlib_constexpr bson_iterator _bson_end(bson_view v) mlib_noexcept;

extern mlib_constexpr bson_view _bson_view_from_ptr(const bson_byte*) mlib_noexcept;

extern mlib_constexpr bool bson_iterator_eq(bson_iterator left, bson_iterator right) mlib_noexcept;

extern mlib_constexpr bool bson_iterator_done(bson_iterator it) mlib_noexcept;

extern inline bool bson_key_eq(const bson_iterator, const char* key, int keylen) mlib_noexcept;

extern inline bson_iterator _bson_find(bson_view v, const char* key, int keylen) mlib_noexcept;

extern inline bson_utf8_view _bson_read_stringlike_at(const bson_byte* p) mlib_noexcept;

extern inline bson_utf8_view _bson_iterator_stringlike(bson_iterator it) mlib_noexcept;

extern mlib_constexpr double bson_iterator_double(bson_iterator it) mlib_noexcept;
extern inline bson_utf8_view bson_iterator_utf8(bson_iterator it) mlib_noexcept;
extern mlib_constexpr bson_view
bson_iterator_document(bson_iterator it, enum bson_view_invalid_reason* error) mlib_noexcept;
extern mlib_constexpr bson_binary    bson_iterator_binary(bson_iterator) mlib_noexcept;
extern mlib_constexpr bson_oid       bson_iterator_oid(bson_iterator) mlib_noexcept;
extern mlib_constexpr bool           bson_iterator_bool(bson_iterator) mlib_noexcept;
extern mlib_constexpr int64_t        bson_iterator_datetime_utc_ms(bson_iterator) mlib_noexcept;
extern inline bson_regex             bson_iterator_regex(bson_iterator) mlib_noexcept;
extern mlib_constexpr bson_dbpointer bson_iterator_dbpointer(bson_iterator) mlib_noexcept;
extern inline bson_utf8_view         bson_iterator_code(bson_iterator) mlib_noexcept;
extern inline bson_utf8_view         bson_iterator_symbol(bson_iterator) mlib_noexcept;
extern mlib_constexpr int32_t        bson_iterator_int32(bson_iterator) mlib_noexcept;
extern mlib_constexpr uint64_t       bson_iterator_timestamp(bson_iterator) mlib_noexcept;
extern mlib_constexpr int64_t        bson_iterator_int64(bson_iterator) mlib_noexcept;
extern mlib_constexpr bson_utf8_view bson_utf8_view_chopnulls(bson_utf8_view str) mlib_noexcept;

extern mlib_constexpr double  bson_iterator_as_double(bson_iterator it) mlib_noexcept;
extern mlib_constexpr bool    bson_iterator_as_bool(bson_iterator it) mlib_noexcept;
extern mlib_constexpr int32_t bson_iterator_as_int32(bson_iterator it) mlib_noexcept;
extern mlib_constexpr int64_t bson_iterator_as_int64(bson_iterator it, bool*) mlib_noexcept;

extern mlib_constexpr int32_t _bson_value_re_len(const char* valptr, int32_t maxlen) mlib_noexcept;

extern mlib_constexpr int32_t _bson_valsize(bson_type              tag,
                                            const bson_byte* const valptr,
                                            int32_t                val_maxlen) mlib_noexcept;

extern mlib_constexpr bson_iterator _bson_iterator_error(enum bson_iterator_error_cond err)
    mlib_noexcept;

void _bson_assert_fail(const char* expr, const char* file, int line) {
    fprintf(stderr,
            "bson/view ASSERTION FAILED at %s:%d: Expression [%s] evaluated to "
            "false\n",
            file,
            line,
            expr);
    abort();
}
