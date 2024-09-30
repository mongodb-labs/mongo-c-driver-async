#include <bson/doc.h>
#include <bson/view.h>

#include <mlib/config.h>

extern mlib_constexpr bson_byte* _bson_write_u32le(bson_byte*, uint32_t) mlib_noexcept;
extern mlib_constexpr bson_byte* _bson_write_u64le(bson_byte*, uint64_t) mlib_noexcept;

extern mlib_constexpr bson_byte*
_bson_memcpy(bson_byte* dst, const bson_byte* src, size_t len) mlib_noexcept;
extern mlib_constexpr bson_byte*
_bson_memmove(bson_byte* dst, const bson_byte* src, size_t len) mlib_noexcept;
extern mlib_constexpr bson_byte* _bson_memset(bson_byte* dst, char v, size_t len) mlib_noexcept;
extern mlib_constexpr bson_byte*
_bson_memcpy_chr(bson_byte* dst, const char* src, size_t len) mlib_noexcept;
extern mlib_constexpr bson_byte*
_bson_memcpy_u8(bson_byte* dst, const uint8_t* src, size_t len) mlib_noexcept;

extern mlib_constexpr uint32_t       bson_doc_capacity(bson_doc d) mlib_noexcept;
extern mlib_constexpr mlib_allocator bson_doc_get_allocator(bson_doc m) mlib_noexcept;
extern mlib_constexpr bool     _bson_doc_realloc(bson_doc* doc, uint32_t new_size) mlib_noexcept;
extern mlib_constexpr int32_t  bson_reserve(bson_doc* d, uint32_t size) mlib_noexcept;
extern mlib_constexpr bson_doc bson_new_ex(mlib_allocator allocator,
                                           uint32_t       reserve) mlib_noexcept;
extern inline bson_doc         bson_new(void) mlib_noexcept;
extern mlib_constexpr bson_doc bson_copy_view(bson_view view, mlib_allocator alloc) mlib_noexcept;
extern mlib_constexpr void     bson_delete(bson_doc d) mlib_noexcept;
