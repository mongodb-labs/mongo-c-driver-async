#pragma once

#include <mlib/alloc.h>

#include <vector>

namespace amongoc {

template <typename T>
using vector = std::vector<T, mlib::allocator<T>>;

}  // namespace amongoc
