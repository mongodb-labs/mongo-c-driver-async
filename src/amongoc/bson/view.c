#include <amongoc/bson/view.h>

#include <string.h>

extern inline int32_t _bson_read_int32_le(const bson_byte* bytes);

extern inline int64_t _bson_read_int64_le(const bson_byte* bytes);

extern inline uint32_t _bson_byte_size(const bson_byte*);

extern inline const bson_byte* _bson_data_as_const(const bson_byte*);

extern inline bson_byte* _bson_data_as_mut(bson_byte*);

extern inline bson_view bson_view_from_data(const bson_byte* const               data,
                                            const size_t                         len,
                                            enum bson_view_invalid_reason* const error);

extern inline bson_iterator _bson_iterator_at(const bson_byte* const data, int32_t bytes_remaining);

extern inline bson_iterator bson_next(const bson_iterator it);

extern inline bson_utf8_view bson_iterator_key(bson_iterator it);

extern inline bson_type bson_iterator_type(bson_iterator it);

extern inline const bson_byte* _bson_iterator_value_ptr(bson_iterator iter);

extern inline const bson_byte* bson_iterator_data(const bson_iterator);

extern inline int32_t bson_iterator_data_size(const bson_iterator);

extern inline bson_iterator _bson_begin(bson_view v);

extern inline bson_iterator _bson_end(bson_view v);

extern inline bson_view _bson_view_from_ptr(const bson_byte*);

extern inline bool bson_iterator_eq(bson_iterator left, bson_iterator right);

extern inline bool bson_iterator_done(bson_iterator it);

extern inline bool bson_key_eq(const bson_iterator, const char* key);

extern inline bson_iterator _bson_find(bson_view v, const char* key);

extern inline bson_utf8_view _bson_read_stringlike_at(const bson_byte* p);

extern inline bson_utf8_view _bson_iterator_stringlike(bson_iterator it);

extern inline double         bson_iterator_double(bson_iterator it);
extern inline int32_t        bson_iterator_int32(bson_iterator it);
extern inline int64_t        bson_iterator_int64(bson_iterator it);
extern inline bson_utf8_view bson_iterator_utf8(bson_iterator it);
extern inline bson_view      bson_iterator_document(bson_iterator                  it,
                                                    enum bson_view_invalid_reason* error);
extern inline bson_binary    bson_iterator_binary(bson_iterator);
extern inline bson_oid       bson_iterator_oid(bson_iterator);
extern inline bool           bson_iterator_bool(bson_iterator);
extern inline int64_t        bson_iterator_datetime(bson_iterator);
extern inline bson_regex     bson_iterator_regex(bson_iterator);
extern inline bson_dbpointer bson_iterator_dbpointer(bson_iterator);
extern inline bson_utf8_view bson_iterator_code(bson_iterator);
extern inline bson_utf8_view bson_iterator_symbol(bson_iterator);
extern inline bson_utf8_view bson_utf8_view_chopnulls(bson_utf8_view str);

extern inline int32_t _bson_value_re_len(const char* valptr, int32_t maxlen);

extern inline int32_t
_bson_valsize(bson_type tag, const bson_byte* const valptr, int32_t val_maxlen);

extern inline bson_iterator _bson_iterator_error(enum bson_iterator_error_cond err);

void _bson_assert_fail(const char* expr, const char* file, int line) {
    fprintf(stderr,
            "bson/view ASSERTION FAILED at %s:%d: Expression [%s] evaluated to "
            "false\n",
            file,
            line,
            expr);
    abort();
}
