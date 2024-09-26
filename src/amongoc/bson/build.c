#include <amongoc/bson/build.h>

extern mlib_constexpr bson_byte* _bson_write_int32le(bson_byte*, int32_t) mlib_noexcept;
extern mlib_constexpr bson_byte* _bson_write_int64le(bson_byte*, int64_t) mlib_noexcept;

extern mlib_constexpr bson_byte*
_bson_memcpy(bson_byte*, const bson_byte* src, uint32_t) mlib_noexcept;
extern mlib_constexpr bson_byte*
_bson_memcpy_chr(bson_byte*, const char* src, uint32_t) mlib_noexcept;
extern mlib_constexpr bson_byte*
_bson_memcpy_u8(bson_byte*, const uint8_t* src, uint32_t) mlib_noexcept;
extern mlib_constexpr bson_byte*
_bson_memmove(bson_byte*, const bson_byte*, uint32_t) mlib_noexcept;
extern mlib_constexpr bson_byte* _bson_memset(bson_byte*, char v, uint32_t) mlib_noexcept;

extern mlib_constexpr uint32_t bson_capacity(bson_mut) mlib_noexcept;

extern mlib_constexpr bool _bson_mut_realloc(bson_mut*, uint32_t) mlib_noexcept;

extern mlib_constexpr int32_t bson_reserve(bson_mut*, uint32_t) mlib_noexcept;

extern mlib_constexpr bson_mut bson_mut_new_ex(mlib_allocator allocator,
                                               uint32_t       reserve) mlib_noexcept;

extern inline bson_mut bson_mut_new(void) mlib_noexcept;

extern mlib_constexpr void bson_mut_delete(bson_mut) mlib_noexcept;

extern mlib_constexpr bson_byte* _bson_mut_data_at(bson_mut doc, bson_iterator pos) mlib_noexcept;

extern mlib_constexpr bson_byte*
_bson_splice_region(bson_mut* doc, bson_byte*, uint32_t, uint32_t, const bson_byte*) mlib_noexcept;

extern mlib_constexpr bson_byte* _bson_prep_element_region(bson_mut* const      d,
                                                           bson_iterator* const pos,
                                                           const bson_type      type,
                                                           bson_utf8_view       key,
                                                           const uint32_t datasize) mlib_noexcept;

extern mlib_constexpr bson_iterator _bson_insert_stringlike(bson_mut*,
                                                            bson_iterator,
                                                            bson_utf8_view,
                                                            bson_type,
                                                            bson_utf8_view) mlib_noexcept;

extern mlib_constexpr bson_iterator bson_insert_double(bson_mut*,
                                                       bson_iterator,
                                                       bson_utf8_view,
                                                       double) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_insert_doc(bson_mut*,
                                                    bson_iterator,
                                                    bson_utf8_view,
                                                    bson_view) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_insert_array(bson_mut*,
                                                      bson_iterator,
                                                      bson_utf8_view) mlib_noexcept;
extern mlib_constexpr bson_mut      bson_mut_subdocument(bson_mut*, bson_iterator) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_parent_iterator(bson_mut) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_insert_binary(bson_mut*,
                                                       bson_iterator,
                                                       bson_utf8_view,
                                                       bson_binary) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_insert_undefined(bson_mut*,
                                                          bson_iterator,
                                                          bson_utf8_view) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_insert_bool(bson_mut*, bson_iterator, bson_utf8_view, bool)
    mlib_noexcept;
extern mlib_constexpr bson_iterator bson_insert_datetime(bson_mut*,
                                                         bson_iterator,
                                                         bson_utf8_view,
                                                         int64_t) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_insert_null(bson_mut*,
                                                     bson_iterator,
                                                     bson_utf8_view) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_insert_regex(bson_mut*,
                                                      bson_iterator,
                                                      bson_utf8_view,
                                                      bson_regex) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_insert_dbpointer(bson_mut*,
                                                          bson_iterator,
                                                          bson_utf8_view,
                                                          bson_dbpointer) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_insert_code_with_scope(bson_mut*,
                                                                bson_iterator,
                                                                bson_utf8_view,
                                                                bson_utf8_view,
                                                                bson_view) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_insert_int32(bson_mut*,
                                                      bson_iterator,
                                                      bson_utf8_view,
                                                      int32_t) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_insert_timestamp(bson_mut*,
                                                          bson_iterator,
                                                          bson_utf8_view,
                                                          uint64_t) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_insert_int64(bson_mut*,
                                                      bson_iterator,
                                                      bson_utf8_view,
                                                      int64_t) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_insert_decimal128(bson_mut*,
                                                           bson_iterator,
                                                           bson_utf8_view,
                                                           struct bson_decimal128) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_insert_maxkey(bson_mut*,
                                                       bson_iterator,
                                                       bson_utf8_view) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_insert_minkey(bson_mut*,
                                                       bson_iterator,
                                                       bson_utf8_view) mlib_noexcept;

extern mlib_constexpr char* _bson_write_uint(uint32_t v, char* at) mlib_noexcept;
extern mlib_constexpr struct bson_array_element_integer_keybuf
                       bson_tmp_uint_string(uint32_t idx) mlib_noexcept;
mlib_thread_local char _bson_tmp_integer_key_tl_storage[12] = {0};
extern mlib_constexpr void
bson_relabel_array_elements_at(bson_mut* doc, bson_iterator pos, uint32_t idx) mlib_noexcept;
extern mlib_constexpr void          bson_relabel_array_elements(bson_mut* doc) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_splice_disjoint_ranges(bson_mut*     doc,
                                                                bson_iterator pos,
                                                                bson_iterator delete_end,
                                                                bson_iterator from_begin,
                                                                bson_iterator from_end)
    mlib_noexcept;
extern mlib_constexpr bson_iterator bson_erase_range(bson_mut* doc,
                                                     bson_iterator,
                                                     bson_iterator) mlib_noexcept;

extern mlib_constexpr bson_iterator bson_set_key(bson_mut*      doc,
                                                 bson_iterator  pos,
                                                 bson_utf8_view newkey) mlib_noexcept;

extern mlib_constexpr bson_utf8_view bson_utf8_view_from_data(const char* s,
                                                              size_t      len) mlib_noexcept;
extern mlib_constexpr bson_utf8_view bson_utf8_view_from_cstring(const char* s) mlib_noexcept;
extern mlib_constexpr bson_utf8_view bson_utf8_view_autolen(const char* s,
                                                            ssize_t     len) mlib_noexcept;
