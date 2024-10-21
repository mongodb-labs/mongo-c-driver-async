#include <bson/mut.h>
#include <bson/view.h>

extern mlib_constexpr bson_byte* _bson_mut_data_at(bson_mut doc, bson_iterator pos) mlib_noexcept;

extern mlib_constexpr bson_byte*
_bson_splice_region(bson_mut* doc, bson_byte*, size_t, size_t, const bson_byte*) mlib_noexcept;

extern mlib_constexpr bson_byte* _bson_prep_element_region(bson_mut* const      d,
                                                           bson_iterator* const pos,
                                                           const bson_type      type,
                                                           mlib_str_view        key,
                                                           const uint32_t datasize) mlib_noexcept;

extern mlib_constexpr bson_iterator _bson_insert_stringlike(bson_mut*,
                                                            bson_iterator,
                                                            mlib_str_view,
                                                            bson_type,
                                                            mlib_str_view) mlib_noexcept;

extern mlib_constexpr char* _bson_write_uint(uint32_t v, char* at) mlib_noexcept;
extern mlib_constexpr struct bson_array_element_integer_keybuf
bson_tmp_uint_string(uint32_t idx) mlib_noexcept;
extern mlib_constexpr void
bson_relabel_array_elements_at(bson_mut* doc, bson_iterator pos, uint32_t idx) mlib_noexcept;
extern mlib_constexpr void          bson_relabel_array_elements(bson_mut* doc) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_splice_disjoint_ranges(bson_mut*     doc,
                                                                bson_iterator pos,
                                                                bson_iterator delete_end,
                                                                bson_iterator from_begin,
                                                                bson_iterator from_end)
    mlib_noexcept;
extern mlib_constexpr bson_iterator bson_insert_disjoint_range(bson_mut*     doc,
                                                               bson_iterator pos,
                                                               bson_iterator from_begin,
                                                               bson_iterator from_end)
    mlib_noexcept;
extern mlib_constexpr bson_iterator bson_erase_range(bson_mut* doc,
                                                     bson_iterator,
                                                     bson_iterator) mlib_noexcept;

extern mlib_constexpr bson_iterator bson_set_key(bson_mut*     doc,
                                                 bson_iterator pos,
                                                 mlib_str_view newkey) mlib_noexcept;
extern mlib_constexpr bson_mut      bson_mut_child(bson_mut*, bson_iterator) mlib_noexcept;
extern mlib_constexpr bson_iterator bson_mut_parent_iterator(bson_mut) mlib_noexcept;

extern mlib_constexpr bson_iterator bson_insert_code_with_scope(bson_mut*      doc,
                                                                bson_iterator  pos,
                                                                mlib_str_view  key,
                                                                bson_code_view code,
                                                                bson_view      scope) mlib_noexcept;
