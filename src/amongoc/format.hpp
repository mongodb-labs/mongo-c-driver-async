#pragma once

#include <amongoc/string.hpp>

#include <mlib/alloc.h>

#include <fmt/base.h>
#include <fmt/core.h>
#include <fmt/format.h>

namespace amongoc {

/**
 * @brief Generate a formatted string using an allocator.
 *
 * Uses `libfmt` to do string formatter.
 *
 * @param alloc The allocator for the string
 * @param fstr The format string to be used
 * @param args Arguments to interpolate
 */
template <typename... Args>
string format(mlib::allocator<> alloc, fmt::format_string<Args...> fstr, Args&&... args) {
    string ret(alloc);
    fmt::vformat_to(std::back_inserter(ret), fstr, fmt::make_format_args(args...));
    return ret;
}

}  // namespace amongoc
