#pragma once

#include <mlib/config.h>
#include <mlib/integer.h>

#include <stdint.h>

/**
 * @brief A duration type
 */
typedef struct mlib_duration {
    // A count of microseconds in the duration
    int64_t usec;
} mlib_duration;

mlib_extern_c_begin();

#define MLIB_MAX_USEC INT64_MAX
#define MLIB_MIN_USEC INT64_MIN
#define MLIB_MAX_MSEC (MLIB_MAX_USEC / 1000ll)
#define MLIB_MIN_MSEC (MLIB_MIN_USEC / 1000ll)
#define MLIB_MAX_SEC (MLIB_MAX_MSEC / 1000ll)
#define MLIB_MIN_SEC (MLIB_MIN_MSEC / 1000ll)

static mlib_constexpr int64_t mlib_count_milliseconds(mlib_duration dur) mlib_noexcept {
    return dur.usec / 1000;
}

static mlib_constexpr mlib_duration mlib_microseconds(int64_t n) mlib_noexcept {
    return mlib_init(mlib_duration){n};
}

static mlib_constexpr mlib_duration mlib_milliseconds(int64_t n) mlib_noexcept {
    return mlib_microseconds(mlib_clamp_i64(n, MLIB_MIN_MSEC, MLIB_MAX_MSEC) * 1000);
}

static mlib_constexpr mlib_duration mlib_seconds(int64_t n) mlib_noexcept {
    return mlib_milliseconds(mlib_clamp_i64(n, MLIB_MIN_SEC, MLIB_MAX_SEC) * 1000);
}

mlib_extern_c_end();
