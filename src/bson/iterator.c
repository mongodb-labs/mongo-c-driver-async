#include <bson/iterator.h>
#include <bson/utf8.h>

extern mlib_constexpr const bson_byte* _bson_data_as_const(const bson_byte*) mlib_noexcept;

extern mlib_constexpr bson_byte* _bson_data_as_mut(bson_byte*) mlib_noexcept;

extern mlib_constexpr uint32_t _bson_byte_size(const bson_byte*) mlib_noexcept;
extern mlib_constexpr int32_t  _bson_byte_ssize(const bson_byte*) mlib_noexcept;

extern mlib_constexpr bson_iterator _bson_begin(const bson_byte* v) mlib_noexcept;
extern mlib_constexpr bson_iterator _bson_end(const bson_byte* v) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_next(const bson_iterator it) mlib_noexcept;
extern mlib_constexpr bool          bson_stop(bson_iterator it) mlib_noexcept;

extern mlib_constexpr enum bson_iter_errc bson_iterator_get_error(bson_iterator it) mlib_noexcept;
extern mlib_constexpr bool bson_iterator_eq(bson_iterator left, bson_iterator right) mlib_noexcept;
extern mlib_constexpr bool _bson_key_eq(const bson_iterator, bson_utf8_view) mlib_noexcept;

extern mlib_constexpr bson_iterator _bson_iterator_at(const bson_byte* const data,
                                                      int32_t bytes_remaining) mlib_noexcept;

extern inline bson_utf8_view bson_key(bson_iterator it) mlib_noexcept;

extern mlib_constexpr bson_type bson_iterator_type(bson_iterator it) mlib_noexcept;

extern mlib_constexpr const bson_byte* _bson_iterator_value_ptr(bson_iterator iter) mlib_noexcept;

extern mlib_constexpr const bson_byte* bson_iterator_data(const bson_iterator) mlib_noexcept;
extern inline bson_iterator            _bson_find(const bson_byte* v, bson_utf8_view) mlib_noexcept;

extern inline bson_utf8_view _bson_read_stringlike_at(const bson_byte* p) mlib_noexcept;

extern inline bson_utf8_view _bson_iterator_stringlike(bson_iterator it) mlib_noexcept;

extern mlib_constexpr int32_t _bson_value_re_len(const char* valptr, int32_t maxlen) mlib_noexcept;

extern mlib_constexpr int32_t _bson_valsize(bson_type              tag,
                                            const bson_byte* const valptr,
                                            int32_t                val_maxlen) mlib_noexcept;

extern mlib_constexpr double         bson_iterator_double(bson_iterator it) mlib_noexcept;
extern inline bson_utf8_view         bson_iterator_utf8(bson_iterator it) mlib_noexcept;
extern mlib_constexpr bson_binary    bson_iterator_binary(bson_iterator) mlib_noexcept;
extern mlib_constexpr bson_oid       bson_iterator_oid(bson_iterator) mlib_noexcept;
extern mlib_constexpr bool           bson_iterator_bool(bson_iterator) mlib_noexcept;
extern mlib_constexpr bson_datetime  bson_iterator_datetime(bson_iterator) mlib_noexcept;
extern inline bson_regex             bson_iterator_regex(bson_iterator) mlib_noexcept;
extern mlib_constexpr bson_dbpointer bson_iterator_dbpointer(bson_iterator) mlib_noexcept;
extern inline bson_code              bson_iterator_code(bson_iterator) mlib_noexcept;
extern inline bson_symbol            bson_iterator_symbol(bson_iterator) mlib_noexcept;
extern mlib_constexpr int32_t        bson_iterator_int32(bson_iterator) mlib_noexcept;
extern mlib_constexpr bson_timestamp bson_iterator_timestamp(bson_iterator) mlib_noexcept;
extern mlib_constexpr int64_t        bson_iterator_int64(bson_iterator) mlib_noexcept;
extern inline struct bson_decimal128 bson_iterator_decimal128(bson_iterator) mlib_noexcept;
extern mlib_constexpr bson_utf8_view bson_utf8_view_chopnulls(bson_utf8_view str) mlib_noexcept;

extern mlib_constexpr double  bson_iterator_as_double(bson_iterator it, bool*) mlib_noexcept;
extern mlib_constexpr bool    bson_iterator_as_bool(bson_iterator it) mlib_noexcept;
extern mlib_constexpr int32_t bson_iterator_as_int32(bson_iterator it, bool*) mlib_noexcept;
extern mlib_constexpr int64_t bson_iterator_as_int64(bson_iterator it, bool*) mlib_noexcept;