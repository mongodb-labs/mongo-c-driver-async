#pragma once

#include <mlib/alloc.h>
#include <mlib/config.h>

#include <assert.h>
#include <inttypes.h>
#include <limits.h> /* INT_MAX */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if mlib_is_cxx()
#include <string_view>
#endif

mlib_extern_c_begin();

// .d8888. d888888b d8888b.         db    db d888888b d88888b db   d8b   db
// 88'  YP `~~88~~' 88  `8D         88    88   `88'   88'     88   I8I   88
// `8bo.      88    88oobY'         Y8    8P    88    88ooooo 88   I8I   88
//   `Y8b.    88    88`8b           `8b  d8'    88    88~~~~~ Y8   I8I   88
// db   8D    88    88 `88.          `8bd8'    .88.   88.     `8b d8'8b d8'
// `8888Y'    YP    88   YD C88888D    YP    Y888888P Y88888P  `8b8' `8d8'

/**
 * @brief A simple non-owning string-view type.
 *
 * The viewed string can be treated as an array of char. It's pointed-to data
 * must not be freed or manipulated.
 *
 * @note The viewed string is NOT guaranteed to be null-terminated. It WILL
 * be null-terminated if: Directly created from a string literal, a C string,
 * an `mlib_str`, an `mlib_str_mut`, or other string data that is null-terminated.
 */
typedef struct mlib_str_view {
    /**
     * @brief Pointer to the beginning of the code unit array.
     *
     * @note DO NOT MODIFY
     */
    const char* data;
    /// The length of the viewed string, in bytes
    size_t len;

#if mlib_is_cxx()
    constexpr static mlib_str_view from(std::string_view sv) noexcept {
        return mlib_str_view{sv.data(), sv.length()};
    }

    bool operator==(std::string_view sv) const noexcept { return sv == std::string_view(*this); }
    constexpr operator std::string_view() const noexcept { return std::string_view(data, len); }
#endif
} mlib_str_view;

/**
 * @brief A null @ref mlib_str_view
 */
#define mlib_str_view_null (mlib_init(mlib_str_view){NULL, 0})

/**
 * @brief Create a non-owning @ref mlib_str_view from the given character array and length
 *
 * @param s A pointer to the beginning of a character array.
 * @param len The length of the character array, in code units
 * @return mlib_str_view A non-owning string.
 */
inline mlib_str_view mlib_str_view_data(const char* s, size_t len) mlib_noexcept {
    return mlib_init(mlib_str_view){s, len};
}

/// @internal Dup an mlib_str_view
inline mlib_str_view _mlib_str_view_self(mlib_str_view s) mlib_noexcept { return s; }
/// @internal Create an mlib_str_view from a C string
inline mlib_str_view _mlib_str_view_cstr(const char* s) mlib_noexcept {
    return mlib_str_view_data(s, strlen(s));
}

/**
 * @brief Return the longest prefix of `str` that does not contain embedded null
 * characters.
 *
 * @param str A UTF-8 string, possibly containing embedded null characters.
 */
mlib_constexpr mlib_str_view mlib_str_view_chopnulls(mlib_str_view str) mlib_noexcept {
    size_t len = strnlen(str.data, str.len);
    str.len    = len;
    return str;
}

// .d8888. d888888b d8888b.
// 88'  YP `~~88~~' 88  `8D
// `8bo.      88    88oobY'
//   `Y8b.    88    88`8b
// db   8D    88    88 `88.
// `8888Y'    YP    88   YD

/**
 * @brief A simple string utility type.
 *
 * This string type has the following semantics:
 *
 * The member `data` is a pointer to the beginning of a read-only array of code
 * units. This array will always be null-terminated, but MAY contain
 * intermittent null characters. Use `mlib_str_length` to get the known length
 * of the string.
 *
 * If you create an @ref mlib_str, it MUST eventually be passed to @ref mlib_str_delete()
 *
 * The pointed-to code units of an mlib_str are immutable. To initialize the
 * contents of an mlib_str, @ref mlib_str_new returns an @ref mlib_str_mut, which can then
 * be "sealed" by converting it to an @ref mlib_str through the @ref mlib_str_mut::str
 * union member.
 *
 * By convention, passing/returning an `mlib_str` to/from a function should
 * relinquish ownership of that `mlib_str` to the callee/caller, respectively.
 * Passing or returning an `mlib_str_view` is non-owning.
 */
typedef struct mlib_str {
    /**
     * @brief Pointer to the beginning of the code unit array.
     *
     * @note DO NOT MODIFY
     */
    const char* data;
    // The allocator associated with this string
    mlib_allocator alloc;

#if mlib_is_cxx()
    operator std::string_view() const noexcept {
        std::size_t len;
        memcpy(&len, data - sizeof(size_t), sizeof len);
        return std::string_view(data, len);
    }
    bool operator==(std::string_view sv) const noexcept { return sv == std::string_view(*this); }
#endif
} mlib_str;

/**
 * @brief A null @ref mlib_str
 */
#define mlib_str_null (mlib_init(mlib_str){{{NULL, NULL}}})

/// @internal Get a pointer to the size cookie associated with the given string
inline char* _mlib_str_cookie(mlib_str s) mlib_noexcept { return (char*)s.data - sizeof(size_t); }

/// @internal Get the length of an mlib_str, as a count of bytes
inline size_t _mlib_str_length(mlib_str s) mlib_noexcept {
    size_t ret;
    memcpy(&ret, _mlib_str_cookie(s), sizeof ret);
    return ret;
}

/// @internal Convert an mlib_str to a view
inline mlib_str_view _mlib_str_as_view(mlib_str s) mlib_noexcept {
    return mlib_str_view_data(s.data, _mlib_str_length(s));
}

// .d8888. d888888b d8888b.         .88b  d88. db    db d888888b
// 88'  YP `~~88~~' 88  `8D         88'YbdP`88 88    88 `~~88~~'
// `8bo.      88    88oobY'         88  88  88 88    88    88
//   `Y8b.    88    88`8b           88  88  88 88    88    88
// db   8D    88    88 `88.         88  88  88 88b  d88    88
// `8888Y'    YP    88   YD C88888D YP  YP  YP ~Y8888P'    YP

/**
 * @brief An interface for initializing the contents of an mlib_str.
 *
 * Returned by @ref mlib_str_new(). Once initialization is complete, the result can
 * be used as an @ref mlib_str by accessing the @ref mlib_str_mut::mlib_str member.
 */
typedef struct mlib_str_mut {
    union {
        struct {
            /**
             * @brief Pointer to the beginning of the mutable code unit array.
             *
             * @note DO NOT MODIFY THE POINTER VALUE. Only modify the pointed-to
             * characters.
             */
            char* data;

            mlib_allocator alloc;
        };
        /// Convert the mutable string to an immutable string
        struct mlib_str str;
    };

#if mlib_is_cxx()
    bool operator==(std::string_view sv) const noexcept { return sv == std::string_view(*this); }
    operator std::string_view() const noexcept {
        return std::string_view(data, _mlib_str_length(str));
    }
#endif
} mlib_str_mut;

inline mlib_str_view _mlib_str_mut_as_view(mlib_str_mut m) mlib_noexcept {
    return mlib_str_view_data(m.data, _mlib_str_length(m.str));
}

#if mlib_is_cxx()
#define mlib_as_str_view(X) mlib_str_view::from((X))
#else
#define mlib_as_str_view(X)                                                                        \
    _Generic((X),                                                                                  \
        mlib_str: _mlib_str_as_view,                                                               \
        mlib_str_view: _mlib_str_view_self,                                                        \
        mlib_str_mut: _mlib_str_mut_as_view,                                                       \
        char*: _mlib_str_view_cstr,                                                                \
        const char*: _mlib_str_view_cstr)((X))
#endif

// Get a fixed pointer to an empty string buffer valid for use with an mlib_str
inline const char* _mlib_string_empty(void) mlib_noexcept {
    static const char buf[sizeof(size_t) + 1] = {0};
    return buf + sizeof(size_t);
}

#define mlib_str_length(S) mlib_as_str_view((S)).len

/**
 * @brief Resize the given mutable string, maintaining the existing content, and
 * zero-initializing any added characters.
 *
 * @param s The @ref mlib_str_mut to update
 * @param new_len The new length of the string
 */
inline bool mlib_str_mut_resize(mlib_str_mut* s, size_t new_len) mlib_noexcept {
    const size_t old_len = _mlib_str_length(s->str);
    if (new_len == old_len) {
        return true;
    }
    if (new_len > (SSIZE_MAX - sizeof(size_t)) - 1) {
        return false;
    }
    // Allocate enough room for the cookie and the nul
    size_t alloc_size = new_len + sizeof(size_t) + 1;
    char*  buf_ptr    = _mlib_str_cookie(s->str);
    if (s->data == _mlib_string_empty()) {
        buf_ptr = NULL;
    }
    // Try to allocate
    char* new_data
        = (char*)mlib_reallocate(s->alloc, buf_ptr, alloc_size, 1, old_len + 4 + 1, NULL);
    if (!new_data) {
        // Allocation failed
        return false;
    }
    // Update the pointer:
    s->data = new_data + sizeof(size_t);
    // Update the size cookie:
    memcpy(_mlib_str_cookie(s->str), &new_len, sizeof new_len);
    ptrdiff_t tail_len = (ptrdiff_t)new_len - (ptrdiff_t)old_len;
    if (tail_len > 0) {
        memset(s->data + old_len, 0, (size_t)tail_len);
    }
    // Add a null terminator
    s->data[new_len] = (char)0;
    return true;
}

/**
 * @brief Create a new mutable code-unit array of the given length,
 * zero-initialized. The caller can then modify the code units in the array via
 * the @ref mlib_str_mut::data member. Once finished modifying, can be converted to
 * an immutable mlib_str by taking the @ref mtsr_mut::mlib_str union member.
 *
 * @param len The length of the new string.
 * @return mlib_str_mut A new mlib_str_mut
 *
 * @note The @ref mlib_str_mut::str member MUST eventually be given to
 * @ref mlib_str_delete().
 */
#define mlib_str_new(...) MLIB_PASTE(_mlibStrNewArgc_, MLIB_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)
#define _mlibStrNewArgc_0() _mlib_str_new(0, mlib_default_allocator)
#define _mlibStrNewArgc_1(N) _mlib_str_new(N, mlib_default_allocator)
#define _mlibStrNewArgc_2(N, Alloc) _mlib_str_new(N, Alloc)

inline mlib_str_mut _mlib_str_new(size_t len, mlib_allocator alloc) mlib_noexcept {
    mlib_str_mut ret;
    ret.alloc = alloc;
    ret.data  = (char*)_mlib_string_empty();
    mlib_str_mut_resize(&ret, len);
    return ret;
}

/**
 * @brief Create an @ref mlib_str from the given character array and length.
 *
 * @param s A pointer to a character array
 * @param len The length of the string to create
 * @return mlib_str A new null-terminated string with the contents copied from the
 * pointed-to array.
 *
 * @note The resulting string will be null-terminated.
 */
inline mlib_str mlib_str_copy_data(const char* s, size_t len) mlib_noexcept {
    mlib_str_mut r = mlib_str_new(len);
    memcpy(r.data, s, len);
    return r.str;
}

/**
 * @brief Copy the contents of the given string view
 *
 * @param s A string view to copy from
 * @return mlib_str A new string copied from the given view
 */
#define mlib_str_copy(S) _mlib_str_copy(mlib_as_str_view((S)))
inline mlib_str _mlib_str_copy(mlib_str_view s) mlib_noexcept {
    return mlib_str_copy_data(s.data, s.len);
}

/**
 * @brief Free the resources of the given string
 *
 * @param s The string to free
 */
inline void mlib_str_delete(mlib_str s) mlib_noexcept {
    char* d = (char*)s.data;
    if (!d || d == _mlib_string_empty()) {
        return;
    }
    mlib_deallocate(s.alloc, d - sizeof(size_t), mlib_str_length(s) + sizeof(size_t) + 1);
}

/**
 * @brief Free and re-assign the given @ref mlib_str
 *
 * @param s Pointer to an @ref mlib_str. This will be freed, then updated to the
 * value of @ref from
 * @param from An @ref mlib_str to take from.
 *
 * @note Ownership of the resource is handed to the pointed-to @ref s.
 * Equivalent to:
 *
 * ```c
 * mlib_str s = some_string();
 * mlib_str another = get_another_string();
 * mlib_str_delete(s);
 * s = another;
 * ```
 *
 * Intended as a convenience for rebinding an @ref mlib_str in a single statement
 * from an expression returning a new @ref mlib_str, which may itself use @ref s,
 * without requiring a temporary variable, for example:
 *
 * ```c
 * mlib_str s = get_string();
 * mlib_str_assign(&s, convert_to_uppercase(s.view));
 * ```
 */
inline void mlib_str_assign(mlib_str* s, mlib_str from) mlib_noexcept {
    mlib_str_delete(*s);
    *s = from;
}

/**
 * @brief Find the index of the first occurrence of the given "needle" as a
 * substring of another string.
 *
 * @param given A string to search within
 * @param needle The substring to search for
 * @return int The zero-based index of the first instance of `needle` in
 * `given`, or -1 if no substring is found.
 */
#define mlib_str_find(Given, Needle)                                                               \
    _mlib_str_find(mlib_as_str_view(Given), mlib_as_str_view(Needle))
inline int _mlib_str_find(mlib_str_view given, mlib_str_view needle) mlib_noexcept {
    const char* const scan_end   = given.data + given.len;
    const char* const needle_end = needle.data + needle.len;
    for (const char* scan = given.data; scan != scan_end; ++scan) {
        size_t remain = (size_t)(scan_end - scan);
        if (remain < needle.len) {
            break;
        }
        const char* subscan = scan;
        for (const char* nscan = needle.data; nscan != needle_end; ++nscan, ++subscan) {
            if (*nscan == *subscan) {
                continue;
            } else {
                goto skip;
            }
        }
        // Got through the whole loop of scanning the needle
        return (int)(scan - given.data);
    skip:
        (void)0;
    }
    return -1;
}

/**
 * @brief Find the index of the last occurrence of the given "needle" as a
 * substring of another string.
 *
 * @param given A string to search within
 * @param needle The substring to search for
 * @return int The zero-based index of the last instance of `needle` in
 * `given`, or -1 if no substring is found.
 */
#define mlib_str_rfind(Given, Needle)                                                              \
    _mlib_str_rfind(mlib_as_str_view(Given), mlib_as_str_view(Needle))
inline int _mlib_str_rfind(mlib_str_view given, mlib_str_view needle) mlib_noexcept {
    if (needle.len > given.len) {
        return -1;
    }
    const char*       scan       = given.data + given.len - needle.len;
    const char* const needle_end = needle.data + needle.len;
    for (; scan >= given.data; --scan) {
        const char* subscan = scan;
        for (const char* nscan = needle.data; nscan != needle_end; ++nscan, ++subscan) {
            if (*nscan == *subscan) {
                continue;
            } else {
                goto skip;
            }
        }
        // Got through the whole loop of scanning the needle
        return (int)(scan - given.data);
    skip:
        (void)0;
    }
    return -1;
}

/**
 * @brief Modify a string by deleting data and/or inserting another string.
 *
 * @param s The string to modify
 * @param at The position at which to insert and delete characters
 * @param del_count The number of characters to delete. Clamped to the string
 * length.
 * @param insert The string to insert at `at`.
 * @return mlib_str A new string that is the result of the splice
 */
#define mlib_str_splice(String, Pos, DelCount, Insert)                                             \
    _mlib_str_splice(mlib_as_str_view(String), Pos, DelCount, mlib_as_str_view(Insert))
inline mlib_str
_mlib_str_splice(mlib_str_view s, size_t at, size_t del_count, mlib_str_view insert) mlib_noexcept {
    assert(at <= s.len);
    const size_t remain = s.len - at;
    if (del_count > remain) {
        del_count = remain;
    }
    /* at this point, it is absolutely necessary that del_count <= s.len */
    assert(s.len - del_count <= SIZE_MAX - insert.len);
    const size_t new_size = s.len - del_count + insert.len;
    mlib_str_mut ret      = mlib_str_new(new_size);
    char*        p        = ret.data;
    memcpy(p, s.data, at);
    p += at;
    if (insert.data) {
        memcpy(p, insert.data, insert.len);
        p += insert.len;
    }
    /* 'at <= s.len' was already asserted earlier */
    assert(s.len - at >= del_count);
    memcpy(p, s.data + at + del_count, s.len - at - del_count);
    return ret.str;
}

/**
 * @brief Append the given suffix to the given string
 */
#define mlib_str_append(S, Suffix) _mlib_str_append(mlib_as_str_view(S), mlib_as_str_view(Suffix))
inline mlib_str _mlib_str_append(mlib_str_view s, mlib_str_view suffix) mlib_noexcept {
    return _mlib_str_splice(s, s.len, 0, suffix);
}

/**
 * @brief Prepend the given prefix to the given string
 */
#define mlib_str_prepend(S, Prefix) mlib_str_splice(S, 0, 0 Prefix)

/**
 * @brief Insert the given string into another string
 *
 * @param s The string to start with
 * @param at The position in `s` where `infix` will be inserted
 * @param infix The string to insert into `s`
 * @return mlib_str A new string with `infix` inserted
 */
#define mlib_str_insert(S, Pos, Infix) mlib_str_splice(S, PosPos, 0, Infix)

/**
 * @brief Erase characters from the given string
 *
 * @param s The string to start with
 * @param at The position at which to begin deleting characters
 * @param count The number of characters to remove
 * @return mlib_str A new string with the deletion result.
 */
#define mlib_str_erase(S, Pos, Count) mlib_str_splice((S), (Pos), (Count), mlib_as_str_view(""))

/**
 * @brief Erase `len` characters from the beginning of the string
 */
#define mlib_str_remove_prefix(S, Count) mlib_str_splice((S), 0, (Count), mlib_as_str_view(""))

/**
 * @brief Erase `len` characters from the end of the string
 */
#define mlib_str_remove_suffix(S, Count) _mlib_str_remove_suffix(mlib_as_str_view(S), (Count))
inline mlib_str _mlib_str_remove_suffix(mlib_str_view s, size_t len) mlib_noexcept {
    assert(s.len >= len);
    return mlib_str_erase(s, s.len - len, len);
}

/**
 * @brief Obtain a substring of the given string
 *
 * @param s The string to start with
 * @param at The beginning position of the new string
 * @param len The number of characters to include. Automatically clamped to the
 * remaining length.
 * @return mlib_str A new string that is a substring of `s`
 */
#define mlib_substr(S, Pos, Len) _mlib_substr(mlib_as_str_view((S)), (Pos), (Len))
inline mlib_str _mlib_substr(mlib_str_view s, size_t at, size_t len) mlib_noexcept {
    assert(at <= s.len);
    const size_t remain = s.len - at;
    if (len > remain) {
        len = remain;
    }
    mlib_str_mut r = mlib_str_new(len);
    memcpy(r.data, s.data + at, len);
    return r.str;
}

/**
 * @brief Obtain a view of a substring of another string.
 *
 * @param s The string to view
 * @param at The position at which the new view will begin
 * @param len The number of characters to view. Automatically clamped to the
 * remaining length.
 * @return mlib_str_view A view of `s`.
 */
#define mlib_str_subview(S, Pos, Len) _mlib_str_subview(mlib_as_str_view((S)), Pos, Len)
inline mlib_str_view _mlib_str_subview(mlib_str_view s, size_t at, size_t len) mlib_noexcept {
    assert(at <= s.len);
    const size_t remain = s.len - at;
    if (len > remain) {
        len = remain;
    }
    return mlib_str_view_data(s.data + at, len);
}

mlib_extern_c_end();
