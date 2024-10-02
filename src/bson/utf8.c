#include <bson/utf8.h>

extern mlib_constexpr bson_utf8_view _bson_already_utf8_view(bson_utf8_view u8) mlib_noexcept;
extern mlib_constexpr bson_utf8_view bson_utf8_view_from_data(const char* s,
                                                              uint32_t    len) mlib_noexcept;
extern mlib_constexpr bson_utf8_view bson_utf8_view_from_cstring(const char* s) mlib_noexcept;
extern mlib_constexpr bson_utf8_view bson_utf8_view_autolen(const char* s,
                                                            int32_t     len) mlib_noexcept;
