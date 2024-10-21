#include <mlib/config.h>

static_assert(MLIB_IS_EMPTY());
static_assert(not MLIB_IS_EMPTY(1));
static_assert(not MLIB_IS_EMPTY(1, 21));
static_assert(MLIB_IS_EMPTY(MLIB_NOTHING()));
static_assert(not MLIB_IS_EMPTY(MLIB_NOTHING));
static_assert(MLIB_BOOLEAN(true));
static_assert(not MLIB_BOOLEAN(false));
static_assert(not MLIB_BOOLEAN(0));

static_assert(MLIB_IF_ELSE(0)(0)(41) == 41);
static_assert(MLIB_IF_ELSE(1)(0)(41) == 0);
static_assert(MLIB_IF_ELSE(1)(0)() == 0);
static_assert(MLIB_IF_ELSE(0)()(3) == 3);
static_assert(MLIB_IF_ELSE(false)()(3) == 3);
