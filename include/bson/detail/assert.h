#pragma once

#include <mlib/config.h>

// clang-format off
#pragma push_macro("BSON_VIEW_CHECKED")
#ifdef BSON_VIEW_CHECKED
   #if BSON_VIEW_CHECKED == 1
      #define BSON_VIEW_CHECKED_DEFAULT 1
   #elif BSON_VIEW_CHECKED == 0
      #define BSON_VIEW_CHECKED_DEFAULT 0
   #else
      #define BSON_VIEW_CHECKED_DEFAULT 0
   #endif
   // Undef the macro, we'll re-define it later
   #undef BSON_VIEW_CHECKED
#else
   #define BSON_VIEW_CHECKED_DEFAULT 1
#endif
// clang-format on

enum {
    /// Toggle on/off to enable/disable debug assertions
    BSON_VIEW_CHECKED = BSON_VIEW_CHECKED_DEFAULT
};

/**
 * @brief Assert the truth of the given expression. In checked mode, this is a
 * runtime assertion. In unchecked mode, this becomes an optimization hint only.
 */
#define BV_ASSERT(Cond)                                                                            \
    if (BSON_VIEW_CHECKED && !(Cond)) {                                                            \
        _bson_assert_fail(#Cond, __FILE__, __LINE__);                                              \
        abort();                                                                                   \
    } else if (!(Cond)) {                                                                          \
        __builtin_unreachable();                                                                   \
    } else                                                                                         \
        ((void)0)

mlib_extern_c_begin();

/// Fire an assertion failure. This function will abort the program and will not
/// return to the caller.
extern void _bson_assert_fail(const char* message, const char* file, int line);

mlib_extern_c_end();
