#include <mlib/config.h>
#include <mlib/cstring.h>
#include <mlib/str.h>

extern mlib_constexpr size_t mlib_strnlen(const char* s, size_t maxlen) mlib_noexcept;
extern mlib_constexpr size_t mlib_strlen(const char* s) mlib_noexcept;

extern inline const char*           _mlib_string_empty(void) mlib_noexcept;
extern inline mlib_str_view         mlib_str_view_data(const char* s, size_t len) mlib_noexcept;
extern inline mlib_str_view         _mlib_str_view_self(mlib_str_view s) mlib_noexcept;
extern inline mlib_str_view         _mlib_str_view_cstr(const char* s) mlib_noexcept;
extern mlib_constexpr mlib_str_view mlib_str_view_chopnulls(mlib_str_view str) mlib_noexcept;
extern inline char*                 _mlib_str_cookie(mlib_str s) mlib_noexcept;
extern inline size_t                _mlib_str_length(mlib_str s) mlib_noexcept;
extern inline mlib_str_view         _mlib_str_as_view(mlib_str s) mlib_noexcept;
extern inline mlib_str_view         _mlib_str_mut_as_view(mlib_str_mut m) mlib_noexcept;
extern inline bool         mlib_str_mut_resize(mlib_str_mut* s, size_t new_len) mlib_noexcept;
extern inline mlib_str_mut _mlib_str_new(size_t len, mlib_allocator alloc) mlib_noexcept;
extern inline mlib_str     mlib_str_copy_data(const char* s, size_t len) mlib_noexcept;
extern inline mlib_str     _mlib_str_copy(mlib_str_view s) mlib_noexcept;
extern inline void         mlib_str_delete(mlib_str s) mlib_noexcept;

extern inline void mlib_str_assign(mlib_str* s, mlib_str from) mlib_noexcept;
extern inline int  _mlib_str_find(mlib_str_view given, mlib_str_view needle) mlib_noexcept;
extern inline int  _mlib_str_rfind(mlib_str_view given, mlib_str_view needle) mlib_noexcept;
extern inline mlib_str
_mlib_str_splice(mlib_str_view s, size_t at, size_t del_count, mlib_str_view insert) mlib_noexcept;
extern inline mlib_str      _mlib_str_append(mlib_str_view s, mlib_str_view suffix) mlib_noexcept;
extern inline mlib_str      _mlib_str_remove_suffix(mlib_str_view s, size_t len) mlib_noexcept;
extern inline mlib_str      _mlib_substr(mlib_str_view s, size_t at, size_t len) mlib_noexcept;
extern inline mlib_str_view _mlib_str_subview(mlib_str_view s, size_t at, size_t len) mlib_noexcept;
