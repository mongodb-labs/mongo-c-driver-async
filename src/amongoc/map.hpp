#pragma once

#include <mlib/alloc.h>

#include <map>

namespace amongoc {

template <typename Key, typename Value, typename Compare = std::less<>>
using map = std::map<Key, Value, Compare, mlib::allocator<std::pair<const Key, Value>>>;

}  // namespace amongoc
