#pragma once

#include <mlib/config.h>
#include <mlib/str.h>

#include <stdio.h>

/**
 * @brief An abstract writable stream interface.
 */
typedef struct mlib_ostream {
    /// Arbitrary userdata pointer associated with the stream
    void* userdata;
    /// The write callback that handles writing data to the stream.
    size_t (*write)(void* userdata, const char* bufs, size_t buflen);

#if mlib_is_cxx()
    // Copy an existing stream
    static mlib_ostream from(mlib_ostream o) { return o; }
    // Create a stream that appends to an mlib_str
    inline static mlib_ostream from(mlib_str* s) noexcept;
    // Create a stream that writes to a FILE*
    inline static mlib_ostream from(FILE* s) noexcept;

    // Create a stream that appends to an object with a `.append()` method
    template <typename T,
              typename = decltype(std::declval<T&>().append(std::declval<const char*>(),
                                                            std::declval<const char*>()))>
    static mlib_ostream from(T& into) {
        mlib_ostream ret;
        ret.userdata = &into;
        ret.write    = [](void* userdata, const char* buf, size_t buflen) -> std::size_t {
            T& into = *static_cast<T*>(userdata);
            into.append(buf + 0, buf + buflen);
            return buflen;
        };
        return ret;
    }
#endif  // C++
} mlib_ostream;

mlib_extern_c_begin();

mlib_ostream        mlib_ostream_from_fileptr(FILE* out) mlib_noexcept;
mlib_ostream        mlib_ostream_from_strptr(mlib_str* out) mlib_noexcept;
inline mlib_ostream _mlib_ostream_copy(mlib_ostream o) mlib_noexcept { return o; }

#define mlib_ostream_from(Tgt)                                                                     \
    mlib_generic(mlib_ostream::from,                                                               \
                 _mlib_ostream_copy,                                                               \
                 (Tgt),                                                                            \
                 mlib_ostream : _mlib_ostream_copy,                                                \
                 mlib_str* : mlib_ostream_from_strptr,                                             \
                 FILE* : mlib_ostream_from_fileptr)(Tgt)

inline size_t mlib_write(mlib_ostream out, mlib_str_view str) mlib_noexcept {
    return out.write(out.userdata, str.data, str.len);
}

#define mlib_write(Into, Str) mlib_write(mlib_ostream_from(Into), mlib_str_view_from(Str))

mlib_extern_c_end();

#if mlib_is_cxx()
mlib_ostream mlib_ostream::from(mlib_str* s) noexcept { return ::mlib_ostream_from_strptr(s); }
mlib_ostream mlib_ostream::from(FILE* s) noexcept { return ::mlib_ostream_from_fileptr(s); }
#endif  // C++
