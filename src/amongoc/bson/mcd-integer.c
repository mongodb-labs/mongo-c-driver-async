#include <amongoc/bson/mcd-integer.h>

extern inline mcd_integer _mc_math_from_i64(int64_t val);
extern inline mcd_integer _mc_math_from_u64(uint64_t val);
extern inline mcd_integer
_mc_math_check_bounds(mcd_integer min, mcd_integer max, mcd_integer value);
extern inline mcd_integer _mc_math_check_or_jump(jmp_buf*                           jmp,
                                                 volatile struct mc_math_fail_info* info,
                                                 mcd_integer                        v,
                                                 const char*                        file,
                                                 int                                line);
extern inline struct mc_math_fail_info
_mc_math_fail_unvolatile(const volatile struct mc_math_fail_info* f);

extern inline mcd_integer _mc_math_strnlen(const char* string, mcd_integer maxlen);

extern inline mcd_integer _mc_math_sub(mcd_integer l, mcd_integer r);
extern inline mcd_integer _mc_math_add(mcd_integer l, mcd_integer r);

extern inline bool _mcd_i64_sub_would_overflow(int64_t left, int64_t right);
extern inline bool _mcd_i64_add_would_overflow(int64_t left, int64_t right);

extern inline mcd_integer _mc_math_assert_not_flags(enum mcd_integer_flags flags,
                                                    const char*            bits_str,
                                                    mcd_integer            v,
                                                    const char* const      expr_str,
                                                    const char*            file,
                                                    int                    line);
