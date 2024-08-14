#pragma once

#ifdef __cplusplus
#define AMONGOC_IF_CXX(...) __VA_ARGS__
#define AMONGOC_IF_NOT_CXX(...)
#else
#define AMONGOC_IF_CXX(...)
#define AMONGOC_IF_NOT_CXX(...) __VA_ARGS__
#endif

#define AMONGOC_NOEXCEPT AMONGOC_IF_CXX(noexcept(true))

/**
 * @brief Use as the prefix of a braced initializer within C headers, allowing
 * the initializer to appears a compound-init in C and an equivalent  braced aggregate-init
 * in C++
 */
#define AMONGOC_INIT(T) AMONGOC_IF_CXX(T) AMONGOC_IF_NOT_CXX((T))

// Begin an `extern "C"` block
#define AMONGOC_EXTERN_C_BEGIN AMONGOC_IF_CXX(extern "C" {)
// End an `extern "C"` block
#define AMONGOC_EXTERN_C_END AMONGOC_IF_CXX(                                                       \
    })

// Expression-forwarding macro. Shorter and easier than std::forward/std::move
#define AM_FWD(...) (static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__))

/* Expands to nothing. Used to defer a function-like macro and to ignore arguments */
#define _amongoc_nothing(...)

#define _amongoc_paste(A, ...) _amongocPaste1(A, __VA_ARGS__)
#define _amongocPaste1(A, ...) A##__VA_ARGS__

// clang-format off
#define _amongocFirstArg(first, ...) first

/**
 * @brief Expand to 1 if given no arguments, otherwise 0.
 */
#define _amongoc_isEmpty(...) _amongocFirstArg(__VA_OPT__(0, ) 1, ~)
#define _amongoc_nonEmpty(...) _amongocFirstArg(__VA_OPT__(1, ) 0, ~)

/**
 * @brief Expand to the first argument if `Cond` is 1, the second argument if `Cond` is 0
 */
#define _amongoc_ifElse(Cond, IfTrue, IfFalse) \
    /* Suppress expansion of the two branches by using the '#' operator */ \
    _amongoc_nothing(#IfTrue, #IfFalse)  \
    /* Concat the cond 1/0 with a prefix macro: */ \
    _amongoc_paste(_amongoc_ifElse_PICK_, Cond)(IfTrue, IfFalse)

#define _amongoc_ifElse_PICK_1(IfTrue, IfFalse) \
   /* Expand the first operand, throw away the second */ \
   IfTrue _amongoc_nothing(#IfFalse)
#define _amongoc_ifElse_PICK_0(IfTrue, IfFalse) \
   /* Expand to the second operand, throw away the first */ \
   IfFalse _amongoc_nothing(#IfTrue)

// clang-format on
