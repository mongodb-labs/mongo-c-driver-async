#pragma once

#include <mlib/alloc.h>

#include <string>

namespace amongoc {

using string = std::basic_string<char, std::char_traits<char>, mlib::allocator<char>>;

}  // namespace amongoc
