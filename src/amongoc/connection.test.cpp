#include <amongoc/alloc.h>
#include <amongoc/async.h>
#include <amongoc/box.h>
#include <amongoc/bson/build.h>
#include <amongoc/bson/view.h>
#include <amongoc/connection.h>
#include <amongoc/default_loop.h>
#include <amongoc/emitter.h>
#include <amongoc/loop.h>
#include <amongoc/nano/concepts.hpp>
#include <amongoc/nano/just.hpp>
#include <amongoc/operation.h>

#include <mlib/alloc.h>

#include <asio/error.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/address_v4.hpp>
#include <asio/ip/tcp.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace amongoc;

TEST_CASE("Client/Good") {
    amongoc_loop loop;
    amongoc_default_loop_init(&loop);
    unique_emitter em
        = amongoc_just(amongoc_okay, amongoc_nil, ::mlib_terminating_allocator).as_unique();
    SECTION("Static URI string") {
        em = amongoc_client_new(&loop, "mongodb://localhost:27017").as_unique();
    }
    SECTION("Dynamic URI string") {
        std::string s = "mongodb://localhost:27017";
        em            = amongoc_client_new(&loop, s.data()).as_unique();
    }
    status got_ec;
    bool   did_run = false;
    auto   op      = std::move(em).connect(
        unique_handler::from(mlib::terminating_allocator, [&](emitter_result&& r) {
            got_ec  = r.status;
            did_run = true;
        }));
    op.start();
    amongoc_default_loop_run(&loop);
    CHECK(got_ec.code == 0);
    CHECK(did_run);
    amongoc_default_loop_destroy(&loop);
}

TEST_CASE("Client/Invalid hostname") {
    amongoc_loop loop;
    amongoc_default_loop_init(&loop);
    // Connecting to an invalid TLD will fail
    auto   s = amongoc_client_new(&loop, "mongodb://asdfasdfaczxv.invalidtld:27017").as_unique();
    status got_ec;
    auto   op = std::move(s).connect(
        unique_handler::from(mlib::terminating_allocator,
                             [&](emitter_result&& r) { got_ec = r.status; }));
    op.start();
    amongoc_default_loop_run(&loop);
    REQUIRE(got_ec.code == asio::error::netdb_errors::host_not_found);
    amongoc_default_loop_destroy(&loop);
}

TEST_CASE("Client/Timeout") {
    amongoc_loop loop;
    amongoc_default_loop_init(&loop);
    // Connecting to a host that will drop our TCP request. Timeout after 500ms
    auto   conn = amongoc_client_new(&loop, "mongodb://example.com:27017");
    auto   s    = amongoc_timeout(&loop, conn, timespec{0, 500'000'000}).as_unique();
    status got_ec;
    auto   op = std::move(s).bind_allocator_connect(mlib::terminating_allocator,
                                                  [&](emitter_result&& r) { got_ec = r.status; });
    op.start();
    amongoc_default_loop_run(&loop);
    INFO(got_ec.message());
    REQUIRE(got_ec.code == (int)std::errc::timed_out);
    amongoc_default_loop_destroy(&loop);
}

TEST_CASE("Client/Simple request") {
    amongoc_loop loop;
    amongoc_default_loop_init(&loop);
    auto s = amongoc_client_new(&loop, "mongodb://localhost:27017").as_unique();
    std::optional<unique_box> client_box;
    status                    req_ec;
    unique_operation          req_op;
    bool                      did_run = false;
    auto op = std::move(s).bind_allocator_connect(mlib_default_allocator, [&](emitter_result&& r) {
        if (!r.status.is_error()) {
            client_box = std::move(r).value;
            bson::document doc{mlib_default_allocator};
            doc.emplace_back("hello", 1.0);
            doc.emplace_back("$db", "test");
            auto s1 = amongoc_client_command(client_box->as<amongoc_client>(), doc).as_unique();
            req_op  = std::move(s1).bind_allocator_connect(  //
                mlib::allocator<>{mlib_default_allocator},
                [&](emitter_result&& res) {
                    CHECK_FALSE(res.status.is_error());
                    if (not res.status.is_error()) {
                        req_ec         = res.status;
                        bson_view resp = res.value.as<bson::document>();
                        auto      ok   = resp.find("ok");
                        CHECK(ok->as_bool());
                        did_run = true;
                    }
                });
            req_op.start();
        }
    });
    op.start();
    amongoc_default_loop_run(&loop);
    CHECK(did_run);
    client_box.reset();
    amongoc_default_loop_destroy(&loop);
}
