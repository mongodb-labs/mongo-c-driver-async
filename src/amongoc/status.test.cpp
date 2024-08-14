#include <amongoc/status.h>

#include <asm-generic/errno-base.h>
#include <catch2/catch.hpp>
#include <system_error>

TEST_CASE("Status/Okay") {
    amongoc_status st = amongoc_okay;
    CHECK(st.code == 0);
    CHECK(st.category == amongoc_status_category_generic);
}

TEST_CASE("Status/From an Error") {
    auto st = amongoc_status::from(std::make_error_code(std::errc::io_error));
    CHECK(st.category == amongoc_status_category_generic);
    CHECK(st.code == EIO);
}

TEST_CASE("Status/As an Error") {
    amongoc_status st;
    st.category = 0;
    st.code     = ENOMEM;
    auto ec     = st.as_error_code();
    CHECK(ec == std::errc::not_enough_memory);
}
