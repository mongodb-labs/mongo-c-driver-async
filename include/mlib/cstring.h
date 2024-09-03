#pragma once

#include "./config.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

mlib_extern_c_begin();

mlib_constexpr size_t mlib_strnlen(const char* s, size_t maxlen) mlib_noexcept {
    if (mlib_is_consteval()) {
        size_t n = 0;
        while (n < maxlen && s[n] != 0)
            ++n;
        return n;
    } else {
        return strnlen(s, maxlen);
    }
}

mlib_constexpr size_t mlib_strlen(const char* s) mlib_noexcept { return mlib_strnlen(s, SIZE_MAX); }

mlib_extern_c_end();
