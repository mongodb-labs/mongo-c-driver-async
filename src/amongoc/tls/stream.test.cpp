#include <amongoc/tls/stream.hpp>
//
#include <amongoc/asio/as_sender.hpp>
#include <amongoc/nano/concepts.hpp>
#include <amongoc/nano/let.hpp>
#include <amongoc/nano/result.hpp>
#include <amongoc/nano/then.hpp>
#include <amongoc/nano/tie.hpp>
#include <amongoc/status.h>
#include <amongoc/wire/stream.hpp>

#include <asio/connect.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/ssl/context.hpp>
#include <asio/ssl/error.hpp>
#include <asio/ssl/host_name_verification.hpp>
#include <asio/ssl/stream.hpp>
#include <asio/ssl/verify_mode.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <openssl/sslerr.h>
#include <openssl/tls1.h>

#include <system_error>

// Assert that our stream adaptor acts as a writable stream
static_assert(amongoc::wire::writable_stream<amongoc::tls::stream<asio::ip::tcp::socket>>);

TEST_CASE("amongoc/tls/stream") {
    // Use a regular Asio socket as the wrapped stream
    asio::io_context      ioc;
    asio::ip::tcp::socket sock{ioc};

    // Default regular TLS clent
    asio::ssl::context ctx{asio::ssl::context::method::tls_client};
    ctx.set_default_verify_paths();

    // Connect to `example.com`
    asio::ip::tcp::resolver r{ioc};
    auto                    ep = r.resolve("example.com", "443");
    asio::connect(sock, ep);

    // Adapt the socket
    amongoc::tls::stream<asio::ip::tcp::socket&> strm{sock, ctx};
    // Enable peer verification
    auto ec = strm.set_verify_mode(asio::ssl::verify_peer);
    REQUIRE_FALSE(ec);
    // Add host verification
    ec = strm.set_verify_callback(asio::ssl::host_name_verification("example.com"));
    REQUIRE_FALSE(ec);
    // Required for SNI
    ec = strm.set_server_name("example.com");
    REQUIRE_FALSE(ec);

    // Issue a very simple HTTP request
    std::string req
        = "GET / HTTP/1.1\r\n"
          "Connection: close\r\n"
          "Host: example.com\r\n\r\n";
    amongoc::result<std::size_t, std::error_code> res{amongoc::error()};
    CHECK(res.has_error());
    auto op               //
        = strm.connect()  //
        | amongoc::let(amongoc::result_fmap{[&](auto) -> amongoc::nanosender auto {
              // Write the request
              return asio::async_write(strm, asio::buffer(req), amongoc::asio_as_nanosender);
          }})
        | amongoc::then(amongoc::result_join)
        | amongoc::let(amongoc::result_fmap{[&](auto) -> amongoc::nanosender auto {
              // Read a response
              return asio::async_read(strm, asio::buffer(req), amongoc::asio_as_nanosender);
          }})
        | amongoc::then(amongoc::result_join)  //
        | amongoc::tie(res);
    op.start();

    ioc.run();
    REQUIRE(res.has_value());
    CHECK(res.value() > 0);
    CHECK_THAT(req, Catch::Matchers::StartsWith("HTTP/1.1 200 OK\r\n"));
}

TEST_CASE("amongoc/tls/Bad hostname") {
    asio::io_context      ioc;
    asio::ip::tcp::socket sock{ioc};

    asio::ssl::context ctx{asio::ssl::context::method::tls_client};
    ctx.set_default_verify_paths();

    asio::ip::tcp::resolver r{ioc};
    auto                    res = r.resolve("example.com", "443");
    asio::connect(sock, res);

    amongoc::tls::stream<asio::ip::tcp::socket&> strm{sock, ctx};
    auto                                         ec = strm.set_verify_mode(asio::ssl::verify_peer);
    REQUIRE_FALSE(ec);
    // NOTE: Wrong host name!
    ec = strm.set_verify_callback(asio::ssl::host_name_verification("google.com"));
    REQUIRE_FALSE(ec);
    ec = strm.set_server_name("example.com");
    REQUIRE_FALSE(ec);

    // Result for connecting
    amongoc::result<mlib::unit, std::error_code> conn_res;
    // Do the connect
    auto op = strm.connect() | amongoc::tie(conn_res);
    op.start();
    ioc.run();
    // Expect failure
    REQUIRE(conn_res.has_error());
    CAPTURE(conn_res.error().message());
    CHECK(::amongoc_status_tls_reason(amongoc_status::from(conn_res.error()))
          == ::amongoc_tls_errc_ossl_certificate_verify_failed);
}
