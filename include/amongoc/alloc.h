#pragma once

#include <mlib/alloc.h>

#if mlib_is_cxx()

namespace amongoc {

using mlib::allocator;
using mlib::get_allocator;
using mlib::has_allocator;

}  // namespace amongoc

#endif  // C++
