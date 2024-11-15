#include <mlib/integer.h>

extern mlib_constexpr mlib_integer _mlib_math_from_i64(int64_t val) mlib_noexcept;
extern mlib_constexpr mlib_integer _mlib_math_from_u64(uint64_t val) mlib_noexcept;
extern mlib_constexpr mlib_integer _mlib_math_check_bounds(mlib_integer min,
                                                           mlib_integer max,
                                                           mlib_integer value) mlib_noexcept;
extern mlib_constexpr mlib_integer
_mlibMathFillFailureInfo(volatile struct mlib_math_fail_info* info,
                         mlib_integer                         v,
                         const char*                          file,
                         int                                  line) mlib_noexcept;

extern mlib_constexpr mlib_integer _mlib_math_strnlen(const char*  string,
                                                      mlib_integer maxlen) mlib_noexcept;

extern mlib_constexpr mlib_integer _mlib_math_sub(mlib_integer l, mlib_integer r) mlib_noexcept;
extern mlib_constexpr mlib_integer _mlib_math_mul(mlib_integer l,
                                                  mlib_integer r,
                                                  bool         saturate) mlib_noexcept;
extern mlib_constexpr mlib_integer _mlib_math_add(mlib_integer l,
                                                  mlib_integer r,
                                                  bool         saturate) mlib_noexcept;

extern mlib_constexpr bool _mlib_i64_sub_would_overflow(int64_t left, int64_t right);
extern mlib_constexpr bool _mlib_i64_add_would_overflow(int64_t left, int64_t right);
extern mlib_constexpr bool _mlib_i64_mul_would_overflow(int64_t left, int64_t right);

extern mlib_constexpr mlib_integer _mlib_math_assert_not_flags(enum mlib_integer_flags flags,
                                                               const char*             bits_str,
                                                               mlib_integer            v,
                                                               const char* const       expr_str,
                                                               const char*             file,
                                                               int line) mlib_noexcept;

extern mlib_constexpr mlib_integer
_mlib_math_set_flags(mlib_integer v, enum mlib_integer_flags flags) mlib_noexcept;
extern mlib_constexpr mlib_integer
_mlib_math_clear_flags(mlib_integer v, enum mlib_integer_flags flags) mlib_noexcept;
