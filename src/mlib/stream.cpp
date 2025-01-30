#include <mlib/str.h>
#include <mlib/stream.h>

#include <cstdio>

mlib_ostream mlib_ostream_from_fileptr(FILE* out) noexcept {
    mlib_ostream ret;
    ret.userdata = out;
    ret.write    = [](void* userdata, const char* data, size_t buflen) -> std::size_t {
        // TODO: Write error?
        return std::fwrite(data, 1, buflen, static_cast<FILE*>(userdata));
    };
    return ret;
}

mlib_ostream mlib_ostream_from_strptr(mlib_str* out) noexcept {
    mlib_ostream ret;
    ret.userdata = out;
    ret.write    = [](void* userdata, const char* data, size_t buflen) -> std::size_t {
        auto out = static_cast<mlib_str*>(userdata);
        // TODO: Alloc failure
        *out = mlib_str_append(*out, mlib_str_view_data(data, buflen));
        return buflen;
    };
    return ret;
}
