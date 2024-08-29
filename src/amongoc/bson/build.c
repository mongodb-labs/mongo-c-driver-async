#include "amongoc/bson/view.h"
#include <amongoc/bson/build.h>

extern inline bson_byte* _bson_write_int32le(bson_byte*, int32_t);
extern inline bson_byte* _bson_write_int64le(bson_byte*, int64_t);

extern inline bson_byte* _bson_memcpy(bson_byte*, const void* src, uint32_t);

extern inline uint32_t bson_capacity(bson_mut);

extern inline bson_byte*
bson_mut_default_reallocate(bson_byte*, uint32_t, uint32_t, uint32_t*, void*);

extern inline bool _bson_mut_realloc(bson_mut*, uint32_t);

extern inline int32_t bson_reserve(bson_mut*, uint32_t);

extern inline bson_mut bson_mut_new_ex(const mlib_allocator* allocator, uint32_t reserve);

extern inline bson_mut bson_mut_new(void);

extern inline void bson_mut_delete(bson_mut);

extern inline bson_byte* _bson_mut_data_at(bson_mut doc, bson_iterator pos);

extern inline bson_byte*
_bson_splice_region(bson_mut* doc, bson_byte*, uint32_t, uint32_t, const bson_byte*);

extern inline bson_byte* _bson_prep_element_region(bson_mut* const      d,
                                                   bson_iterator* const pos,
                                                   const bson_type      type,
                                                   bson_utf8_view       key,
                                                   const uint32_t       datasize);

extern inline bson_iterator
_bson_insert_stringlike(bson_mut*, bson_iterator, bson_utf8_view, bson_type, bson_utf8_view);

extern inline bson_iterator bson_insert_double(bson_mut*, bson_iterator, bson_utf8_view, double);
extern inline bson_iterator bson_insert_doc(bson_mut*, bson_iterator, bson_utf8_view, bson_view);
extern inline bson_iterator bson_insert_array(bson_mut*, bson_iterator, bson_utf8_view);
extern inline bson_mut      bson_mut_subdocument(bson_mut*, bson_iterator);
extern inline bson_iterator bson_parent_iterator(bson_mut);
extern inline bson_iterator
bson_insert_binary(bson_mut*, bson_iterator, bson_utf8_view, bson_binary);
extern inline bson_iterator bson_insert_undefined(bson_mut*, bson_iterator, bson_utf8_view);
extern inline bson_iterator bson_insert_bool(bson_mut*, bson_iterator, bson_utf8_view, bool);
extern inline bson_iterator bson_insert_datetime(bson_mut*, bson_iterator, bson_utf8_view, int64_t);
extern inline bson_iterator bson_insert_null(bson_mut*, bson_iterator, bson_utf8_view);
extern inline bson_iterator bson_insert_regex(bson_mut*, bson_iterator, bson_utf8_view, bson_regex);
extern inline bson_iterator
bson_insert_dbpointer(bson_mut*, bson_iterator, bson_utf8_view, bson_dbpointer);
extern inline bson_iterator
bson_insert_code_with_scope(bson_mut*, bson_iterator, bson_utf8_view, bson_utf8_view, bson_view);
extern inline bson_iterator bson_insert_int32(bson_mut*, bson_iterator, bson_utf8_view, int32_t);
extern inline bson_iterator
bson_insert_timestamp(bson_mut*, bson_iterator, bson_utf8_view, uint64_t);
extern inline bson_iterator bson_insert_int64(bson_mut*, bson_iterator, bson_utf8_view, int64_t);
extern inline bson_iterator
bson_insert_decimal128(bson_mut*, bson_iterator, bson_utf8_view, struct bson_decimal128);
extern inline bson_iterator bson_insert_maxkey(bson_mut*, bson_iterator, bson_utf8_view);
extern inline bson_iterator bson_insert_minkey(bson_mut*, bson_iterator, bson_utf8_view);

extern inline char*          _bson_write_uint(uint32_t v, char* at);
extern inline bson_utf8_view bson_tmp_uint_string(uint32_t idx);
_Thread_local char           _bson_tmp_integer_key_tl_storage[12] = {0};
extern inline void bson_relabel_array_elements_at(bson_mut* doc, bson_iterator pos, uint32_t idx);
extern inline void bson_relabel_array_elements(bson_mut* doc);
extern inline bson_iterator bson_splice_disjoint_ranges(bson_mut*     doc,
                                                        bson_iterator pos,
                                                        bson_iterator delete_end,
                                                        bson_iterator from_begin,
                                                        bson_iterator from_end);
extern inline bson_iterator bson_erase_range(bson_mut* doc, bson_iterator, bson_iterator);

extern inline bson_iterator bson_set_key(bson_mut* doc, bson_iterator pos, bson_utf8_view newkey);

extern inline bson_utf8_view bson_utf8_view_from_data(const char* s, size_t len);
extern inline bson_utf8_view bson_utf8_view_from_cstring(const char* s);
extern inline bson_utf8_view bson_utf8_view_autolen(const char* s, ssize_t len);