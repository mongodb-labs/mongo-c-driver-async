#include <mlib/stream.h>

extern inline mlib_ostream _mlib_ostream_copy(mlib_ostream o) mlib_noexcept;
extern inline size_t(mlib_write)(mlib_ostream o, mlib_str_view sv) mlib_noexcept;
