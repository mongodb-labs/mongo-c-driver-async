#include <amongoc/alloc.h>
#include <amongoc/async.h>
#include <amongoc/box.h>
#include <amongoc/client.h>
#include <amongoc/default_loop.h>
#include <amongoc/emitter.h>
#include <amongoc/loop.h>
#include <amongoc/loop_fixture.test.hpp>
#include <amongoc/nano/concepts.hpp>
#include <amongoc/nano/just.hpp>
#include <amongoc/operation.h>

#include <bson/doc.h>
#include <bson/mut.h>
#include <bson/view.h>

#include <mlib/alloc.h>

#include <asio/error.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/address_v4.hpp>
#include <asio/ip/tcp.hpp>
#include <catch2/catch_test_macros.hpp>

#include <test_params.test.hpp>

using namespace amongoc;

TEST_CASE_METHOD(testing::loop_fixture, "Client/Good") {
    unique_emitter em
        = amongoc_just(amongoc_okay, amongoc_nil, ::mlib_terminating_allocator).as_unique();
    SECTION("Static URI string") {
        em = amongoc_client_new(&loop.get(), testing::parameters.require_uri().data()).as_unique();
    }
    SECTION("Dynamic URI string") {
        std::string s = testing::parameters.require_uri();
        em            = amongoc_client_new(&loop.get(), s.data()).as_unique();
    }
    status got_ec;
    bool   did_run = false;
    auto   op      = std::move(em).connect(
        unique_handler::from(mlib::terminating_allocator, [&](emitter_result&& r) {
            got_ec  = r.status;
            did_run = true;
        }));
    op.start();
    loop.run();
    CHECK(got_ec.code == 0);
    CHECK(did_run);
}

TEST_CASE_METHOD(testing::loop_fixture, "Client/Invalid hostname") {
    // Connecting to an invalid TLD will fail
    auto s
        = amongoc_client_new(&loop.get(), "mongodb://asdfasdfaczxv.invalidtld:27017").as_unique();
    status got_ec;
    auto   op = std::move(s).connect(
        unique_handler::from(mlib::terminating_allocator,
                             [&](emitter_result&& r) { got_ec = r.status; }));
    op.start();
    loop.run();
    REQUIRE(got_ec.code == asio::error::netdb_errors::host_not_found);
}

TEST_CASE_METHOD(testing::loop_fixture, "Client/Timeout") {
    // Connecting to a host that will drop our TCP request. Timeout after 500ms
    auto   conn = amongoc_client_new(&loop.get(), "mongodb://example.com:27017");
    auto   s    = amongoc_timeout(&loop.get(), conn, timespec{0, 500'000'000}).as_unique();
    status got_ec;
    auto   op = std::move(s).bind_allocator_connect(mlib::terminating_allocator,
                                                  [&](emitter_result&& r) { got_ec = r.status; });
    op.start();
    loop.run();
    INFO(got_ec.message());
    REQUIRE(got_ec.code == (int)std::errc::timed_out);
}

TEST_CASE_METHOD(testing::loop_fixture, "Client/Simple request") {
    auto s = amongoc_client_new(&loop.get(), testing::parameters.require_uri().data()).as_unique();
    std::optional<unique_box> client_box;
    status                    req_ec;
    unique_operation          req_op;
    bool                      did_run = false;
    auto op = std::move(s).bind_allocator_connect(mlib_default_allocator, [&](emitter_result&& r) {
        if (!r.status.is_error()) {
            client_box = std::move(r).value;
            bson::document doc{mlib_default_allocator};
            bson::mutator(doc).emplace_back("hello", 1.0);
            bson::mutator(doc).emplace_back("$db", "test");
            auto s1 = amongoc_client_command(client_box->as<amongoc_client>(), doc).as_unique();
            req_op  = std::move(s1).bind_allocator_connect(  //
                mlib::allocator<>{mlib_default_allocator},
                [&](emitter_result&& res) {
                    CHECK_FALSE(res.status.is_error());
                    if (not res.status.is_error()) {
                        req_ec         = res.status;
                        bson_view resp = res.value.as<bson::document>();
                        auto      ok   = resp.find("ok");
                        CHECK(ok->value().as_bool());
                        did_run = true;
                    }
                });
            req_op.start();
        }
    });
    op.start();
    loop.run();
    CHECK(did_run);
    client_box.reset();
}
