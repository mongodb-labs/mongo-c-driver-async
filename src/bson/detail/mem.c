#include <bson/detail/mem.h>

extern mlib_constexpr bson_byte* _bson_write_u32le(bson_byte*, uint32_t) mlib_noexcept;
extern mlib_constexpr bson_byte* _bson_write_u64le(bson_byte* out, uint64_t u64) mlib_noexcept;
extern mlib_constexpr uint32_t   _bson_read_u32le(const bson_byte* bytes) mlib_noexcept;
extern mlib_constexpr uint64_t   _bson_read_u64le(const bson_byte* bytes) mlib_noexcept;

extern mlib_constexpr bson_byte*
_bson_memcpy(bson_byte* dst, const bson_byte* src, size_t len) mlib_noexcept;
extern mlib_constexpr bson_byte*
_bson_memmove(bson_byte* dst, const bson_byte* src, size_t len) mlib_noexcept;
extern mlib_constexpr bson_byte* _bson_memset(bson_byte* dst, char v, size_t len) mlib_noexcept;
extern mlib_constexpr bson_byte*
_bson_memcpy_chr(bson_byte* dst, const char* src, size_t len) mlib_noexcept;
extern mlib_constexpr bson_byte*
_bson_memcpy_u8(bson_byte* dst, const uint8_t* src, size_t len) mlib_noexcept;
