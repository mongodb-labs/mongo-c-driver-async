#pragma once

#include <mlib/config.h>
#include <mlib/cstring.h>

#include <stdint.h>

#if mlib_is_cxx()
#include <cstdint>
#include <string_view>
#endif

/**
 * @brief A pointer+length pair referring to a read-only array of `char`.
 *
 * For a well-formed bson_utf8_view `V`, if the `V.data` member is not NULL,
 * `V.data` points to the beginning of an array of `char` with a length of at
 * least `V.len`.
 *
 * @note The viewed array is NOT guaranteed to be null-terminated in general.
 */
typedef struct bson_utf8_view {
    /// A pointer to the beginning of a char array.
    const char* data;
    /**
     * @brief The length of the array pointed-to by `data`, if `data` is non
     * NULL.
     */
    uint32_t len;

#if mlib_is_cxx()
    constexpr static bson_utf8_view from_str(const std::string_view& sv) noexcept {
        return bson_utf8_view{sv.data(), static_cast<std::uint32_t>(sv.size())};
    }

    constexpr operator std::string_view() const noexcept {
        return data ? std::string_view(data, len) : "";
    }

    constexpr bool operator==(std::string_view sv) const noexcept {
        return std::string_view(*this) == sv;
    }
#endif
} bson_utf8_view;

#define BSON_UTF8_NULL (mlib_init(bson_utf8_view){NULL, 0})

/**
 * @brief Coerce a string-like object to a `bson_utf8_view`
 *
 * Supports C strings and existing UTF-8 view objects
 */
#define bson_as_utf8(S) _bsonAsUTF8View((S))

mlib_constexpr bson_utf8_view _bson_already_utf8_view(bson_utf8_view u8) mlib_noexcept {
    return u8;
}

#if mlib_is_cxx()
#define _bsonAsUTF8View(S) ::bson_utf8_view::from_str(S)
#else
#define _bsonAsUTF8View(S)                                                                         \
    _Generic((S), bson_utf8_view: _bson_already_utf8_view, char*: bson_utf8_view_from_cstring)((S))
#endif

mlib_extern_c_begin();

/**
 * @brief Create a bson_utf8_view from a pointer-to-array and a length
 *
 * @param s A pointer to an array of `char`
 * @param len The length of the new view. This value must not be greater than
 * the length of the array pointed-to by `s`
 * @return bson_utf8_view A view of the array at `s`, with `len` characters.
 *
 * @note The returned view MAY contain null characters, if the input array
 * contains null characters.
 */
mlib_constexpr bson_utf8_view bson_utf8_view_from_data(const char* s, uint32_t len) mlib_noexcept {
    return mlib_init(bson_utf8_view){s, len};
}

/**
 * @brief Create a bson_utf8_view from a null-terminated pointer-to-array of
 * char
 *
 * @param s A pointer to an array of `char`, which must include a null character
 * @return bson_utf8_view A UTF-8 view that views the longest prefix of the
 * array pointed-to by `s` that contains no null characters
 *
 * @note The returned bson_utf8_view is guaranteed to NOT contain embedded null
 * characters.
 */
mlib_constexpr bson_utf8_view bson_utf8_view_from_cstring(const char* s) mlib_noexcept {
    return bson_utf8_view_from_data(s, (uint32_t)mlib_strlen(s));
}

/**
 * @brief Create a bson_utf8_view, automatically determining the size if the
 * given size argument is less than zero.
 *
 * @param s A pointer-to-array of char
 * @param len The length of the view to return, or less-than-zero to request
 * an automatic length
 * @return bson_utf8_view A new bson_utf8_view for the given parameters
 *
 * If `len` is *less than* zero, this call is equivalent to
 * `bson_utf8_view_from_cstring(s)`, otherwise it is equivalent to
 * `bson_utf8_view_from_data(s, len)`
 */
mlib_constexpr bson_utf8_view bson_utf8_view_autolen(const char* s, int32_t len) mlib_noexcept {
    if (len < 0) {
        return bson_utf8_view_from_cstring(s);
    } else {
        return bson_utf8_view_from_data(s, (uint32_t)len);
    }
}

/**
 * @brief Return the longest prefix of `str` that does not contain embedded null
 * characters.
 *
 * @param str A UTF-8 string, possibly containing embedded null characters.
 * @return bson_utf8_view A string that begins at the same position as `str`,
 * but truncated at the first embedded null character, or equivalent to `str`
 * if `str` contains no null characters.
 */
mlib_constexpr bson_utf8_view bson_utf8_view_chopnulls(bson_utf8_view str) mlib_noexcept {
    uint32_t len = (uint32_t)mlib_strnlen(str.data, str.len);
    str.len      = len;
    return str;
}
mlib_extern_c_end();
