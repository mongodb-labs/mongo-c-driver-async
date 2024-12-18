#define VecDeclareExternInlines_bson_doc_vec

#include <bson/doc.h>
#include <bson/view.h>

#include <mlib/config.h>

extern mlib_constexpr uint32_t       bson_doc_capacity(bson_doc d) mlib_noexcept;
extern mlib_constexpr mlib_allocator bson_doc_get_allocator(bson_doc m) mlib_noexcept;
extern inline bool                   _bson_realloc(bson_doc* doc, uint32_t new_size) mlib_noexcept;
extern inline int32_t                bson_doc_reserve(bson_doc* d, uint32_t size) mlib_noexcept;
extern inline bson_doc _bson_new(uint32_t reserve, mlib_allocator allocator) mlib_noexcept;
extern inline bson_doc _bson_copy_doc(bson_doc) mlib_noexcept;
extern inline bson_doc _bson_new_with_alloc(mlib_allocator a) mlib_noexcept;
extern inline bson_doc _bson_copy_view_with_default_allocator(bson_view view) mlib_noexcept;
extern inline bson_doc
_bson_copy_array_view_with_default_allocator(bson_array_view view) mlib_noexcept;
extern inline bson_doc     _bson_copy_mut_with_default_allocator(bson_mut m) mlib_noexcept;
extern inline bson_doc     _bson_new_reserve_with_default_alloc(uint32_t n) mlib_noexcept;
extern inline bson_doc     _bson_copy_doc_with_allocator(bson_doc       doc,
                                                         mlib_allocator alloc) mlib_noexcept;
extern inline bson_doc     _bson_copy_view_with_allocator(bson_view      view,
                                                          mlib_allocator alloc) mlib_noexcept;
extern inline bson_doc     _bson_copy_array_view_with_allocator(bson_array_view view,
                                                                mlib_allocator  alloc) mlib_noexcept;
extern inline bson_doc     _bson_copy_mut_with_allocator(bson_mut       m,
                                                         mlib_allocator alloc) mlib_noexcept;
extern mlib_constexpr void bson_delete(bson_doc d) mlib_noexcept;

extern inline const bson_byte*   _bson_get_global_empty_doc_data(void) mlib_noexcept;
extern mlib_constexpr bson_byte* _bson_doc_buffer_ptr(bson_doc) mlib_noexcept;
