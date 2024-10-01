#include <bson/doc.h>
#include <bson/view.h>

#include <mlib/config.h>

extern inline uint32_t               bson_doc_capacity(bson_doc d) mlib_noexcept;
extern mlib_constexpr mlib_allocator bson_doc_get_allocator(bson_doc m) mlib_noexcept;
extern inline bool                   _bson_realloc(bson_doc* doc, uint32_t new_size) mlib_noexcept;
extern mlib_constexpr int32_t        bson_doc_reserve(bson_doc* d, uint32_t size) mlib_noexcept;
extern inline bson_doc bson_new_ex(mlib_allocator allocator, uint32_t reserve) mlib_noexcept;
extern inline bson_doc bson_new(void) mlib_noexcept;
extern inline bson_doc bson_copy_view(bson_view view, mlib_allocator alloc) mlib_noexcept;
extern inline void     bson_delete(bson_doc d) mlib_noexcept;

extern inline const bson_byte* _bson_get_global_empty_doc_data(void) mlib_noexcept;
extern inline bson_byte*       _bson_doc_buffer_ptr(bson_doc) mlib_noexcept;
