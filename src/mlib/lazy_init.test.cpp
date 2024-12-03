#include <mlib/lazy_init.hpp>

#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <stdexcept>

struct returns_42_once {
    bool called = false;
    int  operator()() {
        if (called) {
            FAIL_CHECK("Called more than once");
            return 1729;
        }
        called = true;
        return 42;
    }
};

TEST_CASE("mlib/lazy_init/simple") {
    mlib::lazy_threadsafe<int, returns_42_once> i;
    int                                         got = *i;
    CHECK(got == 42);
    // Calling again will not change the value
    got = *i;
    CHECK(got == 42);
}

struct throws_on_first_call {
    bool called = false;
    int  operator()() {
        if (not called) {
            called = true;
            throw std::runtime_error("ouch");
        }
        return 1729;
    }
};

TEST_CASE("mlib/lazy_init/Throw during init") {
    mlib::lazy_threadsafe<int, throws_on_first_call> i;
    // Throwing during init will throw
    CHECK_THROWS_AS(*i, std::runtime_error);
    // Next call with succeed
    int got = *i;
    CHECK(got == 1729);
}

TEST_CASE("mlib/lazy_init/const init") {
    // We can get from a `const` object
    mlib::lazy_threadsafe<int, returns_42_once> const ci;
    // Getting returns a ref-to-const
    std::same_as<int const&> decltype(auto) iv = *ci;
    CHECK(iv == 42);
}
