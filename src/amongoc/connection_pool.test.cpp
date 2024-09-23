#include <amongoc/connection_pool.hpp>
#include <amongoc/default_loop.h>
#include <amongoc/handshake.hpp>
#include <amongoc/loop.h>
#include <amongoc/nano/tie.hpp>
#include <amongoc/uri.hpp>

#include <mlib/alloc.h>

#include <catch2/catch_test_macros.hpp>

using namespace amongoc;

TEST_CASE("Pool/Create Simple") {
    default_event_loop loop;
    auto uri = connection_uri::parse("mongodb://localhost:27017", ::mlib_default_allocator).value();
    amongoc::connection_pool pool{loop.get(), uri};

    std::optional<connection_pool::member> mem;
    auto op = pool.checkout().as_sender().connect([&mem](auto&& m) { mem = *mlib_fwd(m); });
    op.start();
    loop.run();

    REQUIRE(mem.has_value());

    // Attempt to issue a handshake on our checked-out client
    auto               hs = amongoc::handshake(*mem, "testing");
    handshake_response got_resp{pool.get_allocator()};
    {
        auto op = std::move(hs).as_sender().connect([&](auto&& resp) { got_resp = *resp; });
        op.start();
        loop.run();
    }
}
