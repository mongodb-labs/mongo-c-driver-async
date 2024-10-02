#include <amongoc/connection_pool.hpp>
#include <amongoc/default_loop.h>
#include <amongoc/handshake.hpp>
#include <amongoc/loop.h>
#include <amongoc/loop_fixture.test.hpp>
#include <amongoc/nano/tie.hpp>
#include <amongoc/uri.hpp>
#include <amongoc/wire/client.hpp>

#include <bson/doc.h>
#include <bson/make.hpp>

#include <mlib/alloc.h>

#include <catch2/catch_test_macros.hpp>

#include <test_params.test.hpp>

using namespace amongoc;

TEST_CASE_METHOD(testing::loop_fixture, "Pool/Create Simple") {
    auto uri = connection_uri::parse(testing::parameters.require_uri(), ::mlib_default_allocator)
                   .value();
    amongoc::connection_pool pool{loop.get(), uri};

    std::optional<connection_pool::member> mem;
    auto op = pool.checkout().as_sender().connect([&mem](auto&& m) { mem = *mlib_fwd(m); });
    op.start();
    loop.run();

    REQUIRE(mem.has_value());

    // Attempt to issue a handshake on our checked-out client
    auto               check_cl = wire::checking_client(*mem);
    auto               hs       = amongoc::handshake(check_cl, "testing");
    handshake_response got_resp{pool.get_allocator()};
    {
        auto op = std::move(hs).as_sender().connect([&](auto&& resp) { got_resp = *resp; });
        op.start();
        loop.run();
    }
}

co_task<mlib::unit> do_hangup_test(connection_pool& pool) {
    auto           conn = wire::retrying_client(amongoc::pool_client(pool), 5);
    bson::document cmd;
    using bson::make::array;
    using bson::make::doc;
    using std::pair;
    cmd = doc(pair("configureFailPoint", "failCommand"),
              pair("$db", "admin"),
              pair("mode", doc(pair("times", 3))),
              pair("data",
                   doc{pair("appName", "pool-hangup-test"),
                       pair("closeConnection", true),
                       pair("failCommands", array{"insert"})}))
              .build(::mlib_default_allocator);
    co_await wire::simple_request(wire::client_ref(conn), bson_view(cmd));
    auto insert = doc(pair("insert", "foo"), pair("$db", "main"), pair("documents", array(doc())))
                      .build(::mlib_default_allocator);
    co_await wire::simple_request(wire::client_ref(conn), insert);
    co_return {};
}

TEST_CASE_METHOD(testing::loop_fixture, "Pool/Hangup") {
    auto uri = connection_uri::parse(testing::parameters.require_uri(), ::mlib_default_allocator)
                   .value();
    amongoc::connection_pool               pool{loop.get(), uri};
    result<mlib::unit, std::exception_ptr> fin = error(std::exception_ptr());
    auto op = do_hangup_test(pool).as_sender() | amongoc::tie(fin);
    op.start();
    loop.run();
    CHECK(fin.has_value());
}
