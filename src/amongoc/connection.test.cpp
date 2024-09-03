#include "./connection.hpp"
#include "amongoc/alloc.h"
#include "amongoc/async.h"
#include "amongoc/box.h"
#include "amongoc/bson/build.h"
#include "amongoc/bson/view.h"
#include "amongoc/connection.h"
#include "amongoc/default_loop.h"
#include "amongoc/loop.h"
#include "amongoc/nano/concepts.hpp"
#include "amongoc/nano/just.hpp"
#include "amongoc/operation.h"

#include <asio/error.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/address_v4.hpp>
#include <asio/ip/tcp.hpp>
#include <catch2/catch.hpp>

using namespace amongoc;

TEST_CASE("C++ Connection/Simple") {
    asio::io_context      ioc;
    asio::ip::tcp::socket sock{ioc};

    auto ep = asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), 27017);
    sock.connect(ep);
    raw_connection cl{std::allocator<void>{}, sock};

    bson::document doc;
    doc.emplace_back("hello", 1.0);
    doc.emplace_back("$db", "test");
    auto op = cl.send_op_msg(doc).connect([&](auto r) { doc = r.value(); });
    op.start();
    ioc.run();
    auto it = bson_view(doc).find("ok");
    it.throw_if_error();
    CHECK(it.has_value());
    CHECK(it->key() == "ok");
    CHECK(it->as_double() == 1);
}

TEST_CASE("C Connection/Good") {
    amongoc_loop loop;
    amongoc_default_loop_init(&loop);
    auto   s = amongoc_conn_connect(&loop, "localhost", "27017").as_unique();
    status got_ec;
    bool   did_run = false;
    auto   op = std::move(s).connect(mlib::terminating_allocator, [&](status ec, unique_box b) {
        got_ec  = ec;
        did_run = true;
    });
    op.start();
    amongoc_default_loop_run(&loop);
    CHECK(got_ec.code == 0);
    CHECK(did_run);
    amongoc_default_loop_destroy(&loop);
}

TEST_CASE("C Connection/Invalid hostname") {
    amongoc_loop loop;
    amongoc_default_loop_init(&loop);
    // Connecting to an invalid TLD will fail
    auto   s = amongoc_conn_connect(&loop, "asdfasdfaczxv.invalidtld", "27017").as_unique();
    status got_ec;
    auto   op = std::move(s).connect(mlib::terminating_allocator,
                                   [&](status ec, unique_box b) { got_ec = ec; });
    op.start();
    amongoc_default_loop_run(&loop);
    REQUIRE(got_ec.code == asio::error::netdb_errors::host_not_found);
    amongoc_default_loop_destroy(&loop);
}

TEST_CASE("C Connection/Timeout") {
    amongoc_loop loop;
    amongoc_default_loop_init(&loop);
    // Connecting to a host that will drop our TCP request. Timeout after 500ms
    auto   conn = amongoc_conn_connect(&loop, "example.com", "27017");
    auto   s    = amongoc_timeout(&loop, conn, timespec{0, 500'000'000}).as_unique();
    status got_ec;
    auto   op = std::move(s).connect(mlib::terminating_allocator,
                                   [&](status ec, unique_box b) { got_ec = ec; });
    op.start();
    amongoc_default_loop_run(&loop);
    INFO(got_ec.message());
    REQUIRE(got_ec.code == (int)std::errc::timed_out);
    amongoc_default_loop_destroy(&loop);
}

TEST_CASE("C Connection/Simple request") {
    amongoc_loop loop;
    amongoc_default_loop_init(&loop);
    auto                      s = amongoc_conn_connect(&loop, "localhost", "27017").as_unique();
    std::optional<unique_box> client_box;
    status                    req_ec;
    unique_operation          req_op;
    bool                      did_run = false;
    auto                      op
        = std::move(s).connect(allocator<>{mlib_default_allocator}, [&](status ec, unique_box cl) {
              if (!ec.code) {
                  client_box = std::move(cl);
                  bson::document doc;
                  doc.emplace_back("hello", 1.0);
                  doc.emplace_back("$db", "test");
                  auto s1 = client_box->as<amongoc_connection>().command(doc).as_unique();
                  req_op  = std::move(s1).connect(  //
                      mlib::terminating_allocator,
                      [&](status ec, unique_box b) {
                          CHECK_FALSE(ec.is_error());
                          if (not ec.code) {
                              req_ec         = ec;
                              bson_view resp = b.as<bson::document>();
                              auto      ok   = resp.find("ok");
                              CHECK(ok->as_boolean());
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
