#include <mlib/alloc.h>

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>

struct has_no_allocator {};

static_assert(not mlib::has_allocator<has_no_allocator>);
static_assert(not mlib::has_allocator<int>);
static_assert(mlib::has_allocator<std::vector<int>>);
static_assert(not mlib::has_mlib_allocator<std::vector<int>>);
static_assert(std::same_as<mlib::get_allocator_t<std::string>, std::allocator<char>>);

TEST_CASE("mlib/alloc/get_allocator with a default") {
    auto a = mlib::get_allocator(has_no_allocator{}, mlib::terminating_allocator);
    CHECK(a == mlib::terminating_allocator);
}

TEST_CASE("mlib/alloc/get_allocator with an associated allocator") {
    std::vector<int> v;
    auto             a = mlib::get_allocator(v);
    CHECK(a == std::allocator<int>{});
}
