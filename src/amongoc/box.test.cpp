#include <amongoc/box.compress.hpp>
#include <amongoc/box.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace amongoc;

struct simple_empty {};

static_assert(box_inlinable_type<simple_empty>);
static_assert(box_inlinable_type<std::string_view>);
static_assert(box_inlinable_type<int*>);

struct too_big {
    double a[5];
};

static_assert(not box_inlinable_type<too_big>);

struct not_trivial {
    ~not_trivial() {}
};

static_assert(not box_inlinable_type<not_trivial>);

struct explicitly_relocatable {
    ~explicitly_relocatable() {}

    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true, explicitly_relocatable);
};

static_assert(box_inlinable_type<explicitly_relocatable>);

TEST_CASE("Box/Store an Object") {
    amongoc_box b;
    *amongoc_box_init(b, int) = 42;
    CHECK(amongoc_box_cast(int, b) == 42);
    CHECK(::amongoc_box_cast(int, b) == 42);
}

TEST_CASE("Box/Destroy Nothing") {
    amongoc_box b = amongoc_nil;
    amongoc_box_destroy(b);  // Nothing bad will happen
}

TEST_CASE("Box/Value-Init to Nothing") {
    amongoc_box b{};
    amongoc_box_destroy(b);  // Nothing bad will happen
}

void set_to_true(void* bptr) noexcept { **(bool**)bptr = true; }

TEST_CASE("Box/Simple Destructor") {
    amongoc_box bx;
    bool        did_destroy                   = false;
    *amongoc_box_init(bx, bool*, set_to_true) = &did_destroy;
    CHECK_FALSE(did_destroy);
    amongoc_box_destroy(bx);
    CHECK(did_destroy);
}

TEST_CASE("Box/Unique nil") {
    // Just construct the nil box and destroy it.
    auto b = amongoc::nil();
    b.data();
}

TEST_CASE("Box/With C++ Object") {
    auto b = unique_box::from(mlib::allocator<>{mlib_default_allocator},
                              std::string("Hello, box world! I am a very long string that needs to "
                                          "be dynamically allocated."))
                 .release();
    amongoc_box_destroy(b);  // Will release the string memory
}

TEST_CASE("Box/Compress trivial") {
    auto b = unique_box::from(mlib::terminating_allocator, 42);
    mlib_fwd(b).compress<1, 2, 3, 4, 5, 6, 7, 8>([](auto c) { CHECK(sizeof(c) == sizeof(int)); });
}

TEST_CASE("Box/Compress dynamic") {
    struct very_large {
        char f[256];
    };
    auto b = unique_box::from(mlib::allocator<>(::mlib_default_allocator), very_large{});
    mlib_fwd(b).compress([](auto c) {
        CHECK(sizeof(c) == sizeof(void*));
        (void)mlib_fwd(c).recover();  // Recover the box to destroy it
    });
}
