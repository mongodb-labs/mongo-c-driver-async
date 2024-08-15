#include "./client.hpp"
#include "amongoc/async.h"
#include "amongoc/box.h"
#include "amongoc/bson/build.h"
#include "amongoc/bson/view.h"
#include "amongoc/client.h"
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

TEST_CASE("C++ Client/Simple") {
    asio::io_context      ioc;
    asio::ip::tcp::socket sock{ioc};

    auto ep = asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), 27017);
    sock.connect(ep);
    client cl{sock};

    bson_doc doc;
    doc.push_back("hello", 1.0);
    doc.push_back("$db", "test");
    auto op = cl.send_op_msg(doc).connect([&](auto r) { doc = r.value(); });
    op.start();
    ioc.run();
    auto it = bson_view(doc).find("ok");
    it.throw_if_error();
    CHECK(it.has_value());
    CHECK(it->key() == "ok");
    CHECK(it->as_double() == 1);
}

TEST_CASE("C Client/Good") {
    amongoc_loop loop;
    amongoc_init_default_loop(&loop);
    auto   s = amongoc_client_connect(&loop, "localhost", "27017").as_unique();
    status got_ec;
    bool   did_run = false;
    auto   op      = std::move(s).connect([&](status ec, unique_box b) {
        got_ec  = ec;
        did_run = true;
    });
    op.start();
    amongoc_run_default_loop(&loop);
    CHECK(got_ec.code == 0);
    CHECK(did_run);
    amongoc_destroy_default_loop(&loop);
}

TEST_CASE("C Client/Invalid hostname") {
    amongoc_loop loop;
    amongoc_init_default_loop(&loop);
    // Connecting to an invalid TLD will fail
    auto   s = amongoc_client_connect(&loop, "asdfasdfaczxv.invalidtld", "27017").as_unique();
    status got_ec;
    auto   op = std::move(s).connect([&](status ec, unique_box b) { got_ec = ec; });
    op.start();
    amongoc_run_default_loop(&loop);
    REQUIRE(got_ec.code == asio::error::netdb_errors::host_not_found);
    amongoc_destroy_default_loop(&loop);
}

TEST_CASE("C Client/Timeout") {
    amongoc_loop loop;
    amongoc_init_default_loop(&loop);
    // Connecting to a host that will drop our TCP request. Timeout after 500ms
    auto conn = amongoc_client_connect(&loop, "example.com", "27017");
    // auto   s = amongoc_connect_client_ex(&loop, "example.com", "27017", 500);
    auto   s = amongoc_timeout_us(&loop, conn, 500 * 1000).as_unique();
    status got_ec;
    auto   op = std::move(s).connect([&](status ec, unique_box b) { got_ec = ec; });
    op.start();
    amongoc_run_default_loop(&loop);
    INFO(got_ec.message());
    REQUIRE(got_ec.code == (int)std::errc::timed_out);
    amongoc_destroy_default_loop(&loop);
}

TEST_CASE("C Client/Simple request") {
    amongoc_loop loop;
    amongoc_init_default_loop(&loop);
    auto                      s = amongoc_client_connect(&loop, "localhost", "27017").as_unique();
    std::optional<unique_box> client_box;
    status                    req_ec;
    unique_operation          req_op;
    bool                      did_run = false;
    auto                      op      = std::move(s).connect([&](status ec, unique_box cl) {
        if (!ec.code) {
            client_box = std::move(cl);
            bson_doc doc;
            doc.push_back("hello", 1.0);
            doc.push_back("$db", "test");
            auto s1 = client_box->as<amongoc_client>().command(doc).as_unique();
            req_op = std::move(s1).connect([&](status ec, unique_box b) {
                CHECK_FALSE(ec.is_error());
                if (not ec.code) {
                    req_ec         = ec;
                    bson_view resp = b.as<bson_doc>();
                    auto      ok   = resp.find("ok");
                    CHECK(ok->as_boolean());
                    did_run = true;
                }
            });
            req_op.start();
        }
    });
    op.start();
    amongoc_run_default_loop(&loop);
    CHECK(did_run);
    client_box.reset();
    amongoc_destroy_default_loop(&loop);
}
