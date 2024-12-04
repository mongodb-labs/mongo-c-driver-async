#pragma once

#include <mlib/config.h>
#include <mlib/integer.h>

#include <stdint.h>

/**
 * @brief A duration type
 */
typedef struct mlib_duration {
    // @internal A count of microseconds in the duration
    int64_t _usec;
} mlib_duration;

mlib_extern_c_begin();

mlib_constexpr int64_t mlib_microseconds_count(mlib_duration dur) mlib_noexcept {
    return dur._usec;
}

mlib_constexpr int64_t mlib_milliseconds_count(mlib_duration dur) mlib_noexcept {
    return mlib_microseconds_count(dur) / 1000;
}

mlib_constexpr int64_t mlib_seconds_count(mlib_duration dur) mlib_noexcept {
    return mlib_milliseconds_count(dur) / 1000;
}

mlib_constexpr mlib_duration mlib_microseconds(int64_t n) mlib_noexcept {
    return mlib_init(mlib_duration){n};
}

mlib_constexpr mlib_duration mlib_milliseconds(int64_t n) mlib_noexcept {
    return mlib_microseconds(mlibMath(mulSat(I(n), 1000)).i64);
}

mlib_constexpr mlib_duration mlib_seconds(int64_t n) mlib_noexcept {
    return mlib_milliseconds(mlibMath(mulSat(I(n), 1000)).i64);
}

mlib_constexpr mlib_duration mlib_duration_add(mlib_duration a, mlib_duration b) mlib_noexcept {
    const int64_t sum = mlibMath(addSat(I(a._usec), I(b._usec))).i64;
    return mlib_microseconds(sum);
}

mlib_extern_c_end();
