#include <amongoc/status.h>

#include <catch2/catch_test_macros.hpp>

#include <system_error>

TEST_CASE("Status/Okay") {
    amongoc_status st = amongoc_okay;
    CHECK(st.code == 0);
    CHECK(st.category == &amongoc_generic_category);
}

TEST_CASE("Status/From an Error") {
    auto st = amongoc_status::from(std::make_error_code(std::errc::io_error));
    CHECK(st.category == &amongoc_generic_category);
    CHECK(st.code == EIO);
}

TEST_CASE("Status/As an Error") {
    amongoc_status st;
    st.code = ENOMEM;
    auto ec = st.as_error_code();
    CHECK(ec == std::errc::not_enough_memory);
}
