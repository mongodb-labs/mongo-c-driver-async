#include <mlib/time.h>

extern mlib_constexpr int64_t       mlib_microseconds_count(mlib_duration dur) mlib_noexcept;
extern mlib_constexpr int64_t       mlib_milliseconds_count(mlib_duration dur) mlib_noexcept;
extern mlib_constexpr int64_t       mlib_seconds_count(mlib_duration dur) mlib_noexcept;
extern mlib_constexpr mlib_duration mlib_microseconds(int64_t n) mlib_noexcept;
extern mlib_constexpr mlib_duration mlib_milliseconds(int64_t n) mlib_noexcept;
extern mlib_constexpr mlib_duration mlib_seconds(int64_t n) mlib_noexcept;
extern mlib_constexpr mlib_duration mlib_duration_add(mlib_duration a,
                                                      mlib_duration b) mlib_noexcept;
