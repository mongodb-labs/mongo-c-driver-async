#include <mlib/alloc.h>
#include <mlib/unique.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string_view>

using namespace std::literals;

struct my_aggregate {
    int   a;
    int   b;
    char* string;
};

static void del_aggregate(my_aggregate agg) { free(agg.string); }

#define T my_aggregate
#define VecDestroyElement del_aggregate
// Init 'b' and 'string', but leave 'a' as a zero
#define VecInitElement(Agg, ...) (Agg->b = 42, Agg->string = strdup("default string"))
#include <mlib/vec.t.h>

TEST_CASE("mlib/vec/Value-init") {
    my_aggregate_vec vec{};
    my_aggregate_vec_delete(vec);  // Safe no-op
}

TEST_CASE("mlib/vec/Unique") {
    mlib::unique vec = my_aggregate_vec_new(::mlib_default_allocator);
    // Will des
    my_aggregate* agptr = my_aggregate_vec_push(&vec.get());
    CHECK(vec->size == 1);
    CHECK(agptr->a == 0);
    CHECK(agptr->b == 42);
    CHECK(agptr->string == "default string"sv);
    // Will be deleted by unique<> at the end
}

TEST_CASE("mlib/vec/resize") {
    mlib::unique vec = my_aggregate_vec_new(::mlib_default_allocator);
    REQUIRE(my_aggregate_vec_resize(&vec.get(), 512));
}
