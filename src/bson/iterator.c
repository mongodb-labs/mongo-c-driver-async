#include <bson/iterator.h>

extern mlib_constexpr const bson_byte* _bson_data_as_const(const bson_byte*) mlib_noexcept;

extern mlib_constexpr bson_byte* _bson_data_as_mut(bson_byte*) mlib_noexcept;

extern mlib_constexpr uint32_t _bson_byte_size(const bson_byte*) mlib_noexcept;
extern mlib_constexpr int32_t  _bson_byte_ssize(const bson_byte*) mlib_noexcept;

extern inline bson_iterator _bson_begin(const bson_byte* v) mlib_noexcept;
extern inline bson_iterator _bson_end(const bson_byte* v) mlib_noexcept;
extern inline bson_iterator bson_next(const bson_iterator it) mlib_noexcept;
extern inline bool          bson_stop(bson_iterator it) mlib_noexcept;

extern inline enum bson_iter_errc bson_iterator_get_error(bson_iterator it) mlib_noexcept;
extern inline bool bson_iterator_eq(bson_iterator left, bson_iterator right) mlib_noexcept;
extern inline bool _bson_key_eq(const bson_iterator, mlib_str_view) mlib_noexcept;

extern inline bson_iterator _bson_iterator_at(const bson_byte* const data,
                                              int32_t                bytes_remaining) mlib_noexcept;

extern inline mlib_str_view bson_key(bson_iterator it) mlib_noexcept;

extern inline bson_type bson_iterator_type(bson_iterator it) mlib_noexcept;

extern inline const bson_byte* _bson_iterator_value_ptr(bson_iterator iter) mlib_noexcept;

extern inline const bson_byte* bson_iterator_data(const bson_iterator) mlib_noexcept;
extern inline bson_iterator    _bson_find(const bson_byte* v, mlib_str_view) mlib_noexcept;

extern inline mlib_str_view _bson_read_stringlike_at(const bson_byte* p) mlib_noexcept;

extern inline mlib_str_view _bson_iterator_stringlike(bson_iterator it) mlib_noexcept;

extern mlib_constexpr int32_t _bson_value_re_len(const char* valptr, int32_t maxlen) mlib_noexcept;

extern mlib_constexpr int32_t _bson_valsize(bson_type              tag,
                                            const bson_byte* const valptr,
                                            int32_t                val_maxlen) mlib_noexcept;

extern inline bson_iterator _bson_recover_iterator(const bson_byte* doc_data_begin,
                                                   ptrdiff_t        elem_offset) mlib_noexcept;
