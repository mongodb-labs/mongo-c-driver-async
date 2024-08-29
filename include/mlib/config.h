/**
 * @file config.h
 * @brief Provides utility macros
 * @version 0.1
 * @date 2024-08-29
 *
 * @copyright Copyright (c) 2024
 */
#pragma once

#ifdef __cplusplus
#define MLIB_IF_CXX(...) __VA_ARGS__
#define MLIB_IF_NOT_CXX(...)
#else
#define MLIB_IF_CXX(...)
#define MLIB_IF_NOT_CXX(...) __VA_ARGS__
#endif

// Expands to `1` when compiled as C++, otherwise `0`
#define mlib_is_cxx() MLIB_IF_CXX(1) MLIB_IF_NOT_CXX(0)
// Expands to `0` when compiled as C++, otherwise `1`
#define mlib_is_not_cxx() MLIB_IF_CXX(0) MLIB_IF_NOT_CXX(1)

/**
 * @brief Expands to `noexcept` when compiled as C++, otherwise expands to
 * an empty attribute
 */
#define mlib_noexcept MLIB_IF_CXX(noexcept) MLIB_IF_NOT_CXX([[]])

#define mlib_alignas(T) MLIB_IF_CXX(alignas(T)) MLIB_IF_NOT_CXX(_Alignas(T))

/**
 * @brief Expands to a `static_assert` for the current language
 */
#define mlib_static_assert                                                     \
  MLIB_IF_CXX(static_assert) MLIB_IF_NOT_CXX(_Static_assert)

#define mlib_extern_c_begin() MLIB_IF_CXX(extern "C" {) mlib_static_assert(1)
#define mlib_extern_c_end() MLIB_IF_CXX(                                       \
  }) mlib_static_assert(1)

/**
 * @brief Use as the prefix of a braced initializer within C headers, allowing
 * the initializer to appear as a compound-init in C and an equivalent braced
 * aggregate-init in C++
 */
#define mlib_init(T) MLIB_IF_CXX(T) MLIB_IF_NOT_CXX((T))

/**
 * @brief (C++ only) Expands to a `static_cast` expression that perfect-forwards
 * the argument.
 */
#define mlib_fwd(...) (static_cast<decltype(__VA_ARGS__) &&>(__VA_ARGS__))

#define MLIB_NOTHING(...)
#define MLIB_PASTE(A, ...) _mlibPaste1(A, __VA_ARGS__)
#define _mlibPaste1(A, ...) A##__VA_ARGS__

#define MLIB_FIRST_ARG(A, ...) A
#define MLIB_IS_EMPTY(...) MLIB_FIRST_ARG(__VA_OPT__(0, ) 1, ~)
#define MLIB_IS_NOT_EMPTY(...) MLIB_FIRST_ARG(__VA_OPT__(1, ) 0, ~)

/**
 * @brief Expands to an integer literal corresponding to the number of macro
 * arguments. Supports up to fifteen arguments.
 */
#define MLIB_ARG_COUNT(...)                                                    \
  _mlibPickSixteenth(__VA_ARGS__ __VA_OPT__(, ) 15, 14, 13, 12, 11, 10, 9, 8,  \
                     7, 6, 5, 4, 3, 2, 1, 0)
#define _mlibPickSixteenth(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12,  \
                           _13, _14, _15, _16, ...)                            \
  _16

/**
 * @brief Expand to the first argument if `Cond` is 1, the second argument if
 * `Cond` is 0
 */
#define MLIB_IF_ELSE(Cond, IfTrue, IfFalse)                                    \
  /* Suppress expansion of the two branches by using the '#' operator */       \
  MLIB_NOTHING(#IfTrue, #IfFalse)                                              \
  /* Concat the cond 1/0 with a prefix macro: */                               \
  MLIB_PASTE(_mlibIfElsePick_, Cond)(IfTrue, IfFalse)

#define _mlibIfElsePick_1(IfTrue, IfFalse)                                     \
  /* Expand the first operand, throw away the second */                        \
  IfTrue MLIB_NOTHING(#IfFalse)
#define _mlibIfElsePick_0(IfTrue, IfFalse)                                     \
  /* Expand to the second operand, throw away the first */                     \
  IfFalse MLIB_NOTHING(#IfTrue)
