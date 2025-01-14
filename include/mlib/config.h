/**
 * @file config.h
 * @brief Provides utility macros
 * @version 0.1
 * @date 2024-08-29
 *
 * @copyright Copyright (c) 2024
 */
#pragma once

#include <sys/param.h>

#if __cplusplus
#if __has_include(<version>)
#include <version>
#endif
#endif

/**
 * @brief A function-like macro that always expands to nothing
 */
#define MLIB_NOTHING(...)

#define MLIB_PASTE(A, ...) _mlibPaste1(A, __VA_ARGS__)
#define MLIB_PASTE_3(A, B, ...) MLIB_PASTE(A, MLIB_PASTE(B, __VA_ARGS__))
#define MLIB_PASTE_4(A, B, C, ...)                                             \
  MLIB_PASTE(A, MLIB_PASTE_3(B, C, __VA_ARGS__))
#define MLIB_PASTE_5(A, B, C, D, ...)                                          \
  MLIB_PASTE(A, MLIB_PASTE_4(B, C, D, __VA_ARGS__))
#define _mlibPaste1(A, ...) A##__VA_ARGS__

#define MLIB_STR(...) _mlibStr(__VA_ARGS__)
#define _mlibStr(...) #__VA_ARGS__

/**
 * @brief Expands to the first macro argument unconditionally
 */
#define MLIB_FIRST_ARG(A, ...) A
/**
 * @brief If the argument list if empty, expands to 1. Otherwise, expands to 0
 */
#define MLIB_IS_EMPTY(...) MLIB_FIRST_ARG(__VA_OPT__(0, ) 1, ~)
#define MLIB_IS_NOT_EMPTY(...) MLIB_FIRST_ARG(__VA_OPT__(1, ) 0, ~)

#define MLIB_EVAL_32(...) MLIB_EVAL_16(MLIB_EVAL_16(__VA_ARGS__))
#define MLIB_EVAL_16(...) MLIB_EVAL_8(MLIB_EVAL_8(__VA_ARGS__))
#define MLIB_EVAL_8(...) MLIB_EVAL_4(MLIB_EVAL_4(__VA_ARGS__))
#define MLIB_EVAL_4(...) MLIB_EVAL_2(MLIB_EVAL_2(__VA_ARGS__))
#define MLIB_EVAL_2(...) MLIB_EVAL_1(MLIB_EVAL_1(__VA_ARGS__))
#define MLIB_EVAL_1(...) __VA_ARGS__

#if defined(__cpp_concepts) && __cpp_concepts >= 201907L &&                    \
    defined(__cpp_impl_three_way_comparison) &&                                \
    __cpp_impl_three_way_comparison >= 201907L
#define mlib_have_cxx20() 1
#else
#define mlib_have_cxx20() 0
#endif

/**
 * @brief Expands to the given argument list, suppressing macro argument
 * expansion during expansion.
 */
#define MLIB_JUST(...) __VA_ARGS__ MLIB_NOTHING(#__VA_ARGS__)

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
 * @brief If the argument expands to `0`, `false`, or nothing, expands to `0`.
 * Otherwise expands to `1`.
 */
#define MLIB_BOOLEAN(...)                                                      \
  MLIB_IS_NOT_EMPTY(_mlibPaste1(_mlibBool_, __VA_ARGS__))
#define _mlibBool_0
#define _mlibBool_false
#define _mlibBool_

/**
 * @brief A ternary macro. Expects three parenthesized argument lists in
 * sequence.
 *
 * If the first argument list is a truthy value, expands to the second argument
 * list. Otherwise, expands to the third argument list. The unused argument list
 * is not expanded and is discarded.
 */
#define MLIB_IF_ELSE(...)                                                      \
  MLIB_PASTE(_mlibIfElseBranch_, MLIB_BOOLEAN(__VA_ARGS__))
#define _mlibIfElseBranch_1(...) __VA_ARGS__ _mlibNoExpandNothing
#define _mlibIfElseBranch_0(...) MLIB_NOTHING(#__VA_ARGS__) MLIB_JUST
#define _mlibNoExpandNothing(...) MLIB_NOTHING(#__VA_ARGS__)

#ifdef __cplusplus
#define mlib_is_cxx() 1
#define mlib_is_not_cxx() 0
#define MLIB_IF_CXX(...) __VA_ARGS__
#define MLIB_IF_NOT_CXX(...)
#else
#define mlib_is_cxx() 0
#define mlib_is_not_cxx() 1
#define MLIB_IF_CXX(...)
#define MLIB_IF_NOT_CXX(...) __VA_ARGS__
#endif

#define MLIB_LANG_PICK MLIB_IF_ELSE(mlib_is_not_cxx())

/**
 * @brief Expands to `noexcept` when compiled as C++, otherwise expands to
 * an empty attribute
 */
#define mlib_noexcept MLIB_LANG_PICK([[]])(noexcept)

/**
 * @brief Expands to `constexpr` when compiled as C++, otherwise `inline`
 */
#define mlib_constexpr MLIB_LANG_PICK(inline)(constexpr)

/**
 * @brief Expands to an `alignas()` attribute for the current language
 */
#define mlib_alignas(T) MLIB_LANG_PICK(_Alignas)(alignas)(T)
#define mlib_alignof(T) MLIB_LANG_PICK(_Alignof)(alignof)(T)

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
#define mlib_is_little_endian()                                                \
  mlib_parenthesized_expression(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#elif defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN)
#define mlib_is_little_endian()                                                \
  mlib_parenthesized_expression(__BYTE_ORDER == __LITTLE_ENDIAN)
#else
#error "Do not know how to detect endianness on this platform."
#endif

/**
 * @brief Expands to a `thread_local` specifier for the current language
 */
#define mlib_thread_local MLIB_LANG_PICK(_Thread_local)(thread_local)

/**
 * @brief Expands to a `static_assert` for the current language
 */
#define mlib_static_assert MLIB_LANG_PICK(_Static_assert)(static_assert)

#define mlib_extern_c_begin() MLIB_IF_CXX(extern "C" {) mlib_static_assert(1, "")
#define mlib_extern_c_end() MLIB_IF_CXX(                                       \
  }) mlib_static_assert(1, "")

/**
 * @brief Use as the prefix of a braced initializer within C headers, allowing
 * the initializer to appear as a compound-init in C and an equivalent braced
 * aggregate-init in C++
 */
#define mlib_init(T) MLIB_LANG_PICK((T))(T)

#define mlib_is_consteval() MLIB_LANG_PICK(0)(mlib::is_constant_evaluated())

/**
 * @brief (C++ only) Expands to a `static_cast` expression that perfect-forwards
 * the argument.
 */
#define mlib_fwd(...) (static_cast<decltype(__VA_ARGS__) &&>(__VA_ARGS__))

#ifdef __has_include
#if __has_include(<mlib.tweaks.h>)
#include <mlib.tweaks.h>
#endif // __has_include()
#endif // __has_include

#ifndef mlib_audit_allocator_passing
/**
 * @brief Macro that should be used to toggle convenience APIs that will
 * pass default allocators.
 */
#define mlib_audit_allocator_passing() 1
#endif // mlib_audit_allocator_passing

#ifdef __GNUC__
#define mlib_is_gnu_like() 1
#ifdef __clang__
#define mlib_is_gcc() 0
#define mlib_is_clang() 1
#else
#define mlib_is_gcc() 1
#define mlib_is_clang() 0
#endif
#define mlib_is_msvc() 0
#elif _MSC_VER
#define mlib_is_gnu_like() 0
#define mlib_is_msvc() 1
#endif

#define MLIB_IF_CLANG(...)                                                     \
  MLIB_IF_ELSE(mlib_is_clang())(__VA_ARGS__)(MLIB_NOTHING(#__VA_ARGS__))
#define MLIB_IF_GCC(...)                                                       \
  MLIB_IF_ELSE(mlib_is_gcc())(__VA_ARGS__)(MLIB_NOTHING(#__VA_ARGS__))
#define MLIB_IF_MSVC(...)                                                      \
  MLIB_IF_ELSE(mlib_is_msvc())(__VA_ARGS__)(MLIB_NOTHING(#__VA_ARGS__))

#define MLIB_IF_GNU_LIKE(...)                                                  \
  MLIB_IF_GCC(__VA_ARGS__) MLIB_IF_CLANG(__VA_ARGS__)

#if mlib_is_msvc()
#define mlib_no_unique_address [[msvc::no_unique_address]]
#else
#define mlib_no_unique_address [[no_unique_address]]
#endif

#define mlib_nodiscard(Msg) MLIB_LANG_PICK([[]])([[nodiscard(Msg)]])

#if defined __cpp_concepts
#define MLIB_RETURNS(...)                                                      \
  noexcept(noexcept(__VA_ARGS__))                                              \
      ->decltype(auto)                                                         \
    requires requires { (__VA_ARGS__); }                                       \
  {                                                                            \
    return __VA_ARGS__;                                                        \
  }                                                                            \
  static_assert(true)
#else
#define MLIB_RETURNS(...)                                                      \
  noexcept(noexcept(__VA_ARGS__))->decltype(auto) { return __VA_ARGS__; }      \
  static_assert(true)
#endif

#define mlib_always_inline                                                     \
  MLIB_IF_GNU_LIKE(__attribute__((always_inline)))                             \
  MLIB_IF_MSVC(__forceinline) inline

#define mlib_pragma(...) _Pragma(#__VA_ARGS__)

#define mlib_diagnostic_push()                                                 \
  MLIB_IF_GNU_LIKE(mlib_pragma(GCC diagnostic push))                           \
  MLIB_IF_MSVC(mlib_pragma(warning(push)))                                     \
  mlib_static_assert(true, "")

#define mlib_diagnostic_pop()                                                  \
  MLIB_IF_GNU_LIKE(mlib_pragma(GCC diagnostic pop))                            \
  MLIB_IF_MSVC(mlib_pragma(warning(pop)))                                      \
  mlib_static_assert(true, "")

#define mlib_gcc_warning_disable(Warning)                                      \
  MLIB_IF_GCC(mlib_pragma(GCC diagnostic ignored Warning))                     \
  mlib_static_assert(true, "")

#define mlib_gnu_warning_disable(Warning)                                      \
  MLIB_IF_GNU_LIKE(mlib_pragma(GCC diagnostic ignored Warning))                \
  mlib_static_assert(true, "")

#define mlib_extern_c MLIB_LANG_PICK([[]])(extern "C")

#define mlib_parenthesized_expression(...)                                     \
  MLIB_IF_CXX(mlib::identity{})(__VA_ARGS__)

/**
 * @brief Test whether the current compiler version is at least the given
 * version
 */
#define mlib_compiler_version_gte(Major, Minor, Patch)                         \
  MLIB_IF_GCC(((__GNUC__ > Major) ||                                           \
               (__GNUC__ >= Major && __GNUC_MINOR__ > Minor) ||                \
               (__GNUC__ > Major && __GNUC_MINOR__ >= Minor &&                 \
                __GNUC_PATCHLEVEL__ >= Patch)))                                \
  MLIB_IF_CLANG(((__clang_major__ > Major) ||                                  \
                 (__clang_major__ >= Major && __clang_minor__ > Minor) ||      \
                 (__clang_major__ > Major && __clang_minor__ >= Minor &&       \
                  __clang_patchlevel__ >= Patch)))                             \
  MLIB_IF_MSVC(((_MSC_VER / 100 > Major) ||                                    \
                (_MSC_VER / 100 >= Major && _MSC_VER % 100 > Minor) ||         \
                (_MSC_VER / 100 >= Major && _MSC_VER % 100 >= Minor &&         \
                 _MSC_FULL_VER % 100000 >= Patch)))

#define mlib_is_gcc_at_least(Major, Minor, Patch)                              \
  (mlib_is_gcc() && mlib_compiler_version_gte(Major, Minor, Patch))

#define mlib_is_clang_at_least(Major, Minor, Patch)                            \
  (mlib_is_clang() && mlib_compiler_version_gte(Major, Minor, Patch))

#define mlib_is_msvc_at_least(Major, Minor, Patch)                             \
  (mlib_is_msvc() && mlib_compiler_version_gte(Major, Minor, Patch))

#if mlib_is_cxx() || defined(MLIB_FORCE_DISABLE_GENERIC_SELECTION)
// Never use _Generic in C++
#define mlib_has_generic_selection() 0
#elif __STDC__ == 1 && __STDC_VERSION >= 201112L
// Declares C11 support
#define mlib_has_generic_selection() 1
#elif mlib_is_gcc_at_least(4, 9, 0) || mlib_is_clang_at_least(3, 0, 0) ||      \
    mlib_is_msvc_at_least(19, 28, 0)
// Other compilers that support _Generic() without full C11
#define mlib_has_generic_selection() 1
#else
#define mlib_has_generic_selection() 0
#endif // Check for _Generic() support

/**
 * @brief Create a generic selection expression with fallback compatibility for
 * C++
 *
 * @param CxxExpression The expression that will expand when compiled as C++
 * @param DefaultExpression The expression that will expand if _Generic() is
 * unsupported
 * @param SelectorExpression The Selector expression for _Generic()
 * @param __VA_ARGS__ All remaining arguments are the selectors for _Generic()
 */
#define mlib_generic(CxxExpression, DefaultExpression, SelectorExpression,     \
                     ...)                                                      \
  MLIB_LANG_PICK(MLIB_IF_ELSE(mlib_has_generic_selection())(                   \
      _Generic((SelectorExpression), __VA_ARGS__))(DefaultExpression))         \
  (CxxExpression)

/**
 * @brief For empty struct/union types, this must be the sole non-static
 * declaration.
 *
 * In C++, this expands to a no-op static assertion. For C, this expands to an
 * unspecified field of type `char`. This unspecified field should not be used
 * for any reason.
 */
#define mlib_empty_aggregate_c_compat                                          \
  MLIB_LANG_PICK(char _placeholder)(static_assert(true, ""))

/**
 * @brief Expand to a call expression `Prefix##_argc_N(...)`, where `N` is the
 * number of macro arguments.
 */
#define MLIB_ARGC_PICK(Prefix, ...)                                            \
  MLIB_PASTE_3(Prefix, _argc_, MLIB_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)

#if mlib_is_cxx()

namespace mlib {

/**
 * @retval `true` if evaluated at compile-time and checking is supported
 * @retval `false` otherwise
 */
mlib_constexpr bool is_constant_evaluated() noexcept {
#ifdef __cpp_if_consteval
  if consteval {
    return true;
  } else {
    return false;
  }
#endif
#if mlib_is_gnu_like() || mlib_is_msvc()
  // GNU and MSVC share the builtin
  return __builtin_is_constant_evaluated();
#endif
  // Otherwise we cannot check
  return false;
}

/**
 * @brief An invocable object that simply returns its argument unchanged
 */
struct identity {
  template <typename T>
  mlib_always_inline constexpr T &&operator()(T &&arg) const noexcept {
    return static_cast<T &&>(arg);
  }
};

} // namespace mlib

#endif // C++
