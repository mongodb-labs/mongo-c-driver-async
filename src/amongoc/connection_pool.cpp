#include "./connection_pool.hpp"

#include <amongoc/handshake.hpp>
#include <amongoc/loop.h>
#include <amongoc/loop.hpp>
#include <amongoc/tcp_conn.hpp>
#include <amongoc/uri.hpp>
#include <amongoc/wire/client.hpp>

#include <bson/doc.h>
#include <bson/view.h>

#include <mlib/allocate_unique.hpp>
#include <mlib/config.h>
#include <mlib/lazy_init.hpp>

#include <asio/ssl/context.hpp>
#include <asio/ssl/host_name_verification.hpp>
#include <asio/ssl/stream_base.hpp>
#include <asio/ssl/verify_mode.hpp>

#include <variant>

using namespace amongoc;

static_assert(wire::client_interface<connection_pool::member>);

template co_task<wire::any_message> pool_client::request(wire::one_bson_view_op_msg);

template struct wire::checking_client<amongoc::pool_client>;
template co_task<wire::any_message>
wire::checking_client<amongoc::pool_client>::request(one_bson_view_op_msg&&);
template co_task<bson::document> wire::simple_request(amongoc::pool_client, bson_view);
template co_task<bson::document> wire::simple_request(wire::checking_client<amongoc::pool_client>,
                                                      bson_view);

namespace {

/**
 * @brief Invocable that creates an asio::ssl::context, used with `lazy_threadsafe`
 */
struct asio_ssl_init {
    // The connection URI parameters
    connection_uri uri;

    /**
     * @brief Create an OpenSSL Asio TLS client context using the URI parameters
     */
    asio::ssl::context operator()() {
        asio::ssl::context ctx{asio::ssl::context::method::tls_client};
        // Load an alternative CA file
        if (uri.params.tlsCAFile.has_value()) {
            ctx.load_verify_file(std::string(*uri.params.tlsCAFile));
        }
        // Toggle cert verification
        const bool allow_invalid_certs
            = uri.params.tlsInsecure == true or uri.params.tlsAllowInvalidCertificates == true;
        const bool verify_hostname = uri.params.tlsAllowInvalidHostnames != true;
        if (not allow_invalid_certs) {
            ctx.set_verify_mode(asio::ssl::verify_peer);
            auto first_host = uri.hosts.front().host;
            if (verify_hostname and std::holds_alternative<amongoc::string>(first_host)) {
                auto hn = std::get<amongoc::string>(first_host);
                // XXX: Verify non-first host, and verify IP hosts?
                // May need to set the callback on a per-conn basis, since each
                // conn may go to a different host.
                ctx.set_verify_callback(asio::ssl::host_name_verification(std::string(hn)));
            }
        }
        // If there is a password, set it now
        if (uri.params.tlsCertificateKeyFilePassword) {
            ctx.set_password_callback(
                [passwd = std::string(*uri.params.tlsCertificateKeyFilePassword)]  //
                (std::size_t maxlen, asio::ssl::context::password_purpose purpose) -> std::string {
                    (void)maxlen;  // XXX: Is this a hard restriction?
                    if (purpose == asio::ssl::context::for_reading) {
                        return passwd;
                    }
                    return "";
                });
        }
        // Load the certificate, if present
        if (uri.params.tlsCertificateKeyFile) {
            ctx.use_certificate_file(std::string(*uri.params.tlsCertificateKeyFile),
                                     asio::ssl::context::pem);
        }
        return ctx;
    }
};

}  // namespace

// TODO: Pool options: minPoolSize, maxPoolSize, maxIdleTimeMS, maxConnecting
struct connection_pool::pool_impl {
    // The event loop associated with this pool
    amongoc_loop& loop;
    // The URI from which this pool was constructed
    connection_uri uri;
    // Connection ID counter
    std::atomic_int _conn_ids{1};
    // Pool generate number
    std::atomic_int _generation{1};

    std::mutex _checkout_mutex{};

    member_impl_list _idle_connections{loop.get_allocator()};

    mlib::lazy_threadsafe<asio::ssl::context, asio_ssl_init> _ssl_context{asio_ssl_init{uri}};

    auto get_allocator() const noexcept { return loop.get_allocator(); }

    static co_task<member> checkout(pool_impl& self) {
        // The single-item list that we will populate with the checked out connection
        member_impl_list ret_one{self.get_allocator()};
        // Take a lock on the pool
        {
            // NOTE: Do not hold the lock during any `await`!
            std::unique_lock lk{self._checkout_mutex};
            if (not self._idle_connections.empty()) {
                // We have an item. Splice to the output list:
                ret_one.splice_after(ret_one.cbefore_begin(),
                                     self._idle_connections,
                                     self._idle_connections.before_begin());
                // Release the lock now that we are done with the shared list
                lk.unlock();
                co_return member(std::move(ret_one));
            }
            // We aren't going to go back to the list, so unlock it now
        }
        // TODO: This needs to handle multiples hosts correctly
        auto& ep  = self.uri.hosts.front();
        auto  svc = std::to_string(ep.port);
        // TODO: Connect to hosts by IP
        assert(std::holds_alternative<string>(ep.host)
               && "Unimplemented: IP-address connections. Use a hostname for now.");
        auto addr
            = *co_await async_resolve(self.loop, std::get<string>(ep.host).data(), svc.data());

        tcp_connection_rw_stream socket = *co_await async_connect(self.loop, std::move(addr));
        wire::client<tcp_connection_rw_stream> client
            = co_await prepare_client(self, mlib_fwd(socket));
        auto hs = co_await amongoc::handshake(client, self.uri.params.appname);
        ret_one.emplace_front(&self, mlib_fwd(client), mlib_fwd(hs));
        co_return member(mlib_fwd(ret_one));
    }

    static co_task<wire::client<tcp_connection_rw_stream>>
    prepare_client(pool_impl& self, tcp_connection_rw_stream raw_socket) {
        if (self.uri.params.tls != true) {
            // No TLS
            co_return wire::client<tcp_connection_rw_stream>(mlib_fwd(raw_socket),
                                                             self.get_allocator());
        }
        amongoc::tls::stream<tcp_connection_rw_stream> tls_stream{mlib_fwd(raw_socket),
                                                                  *self._ssl_context};
        // Do the TLS handshake, throwing on error
        *co_await tls_stream.connect();
        co_return wire::client<tcp_connection_rw_stream>(mlib_fwd(tls_stream),
                                                         self.get_allocator());
    }

    void return_(member_impl_list&& mem) {
        std::unique_lock lk{_checkout_mutex};
        _idle_connections.splice_after(_idle_connections.cbefore_begin(), mem);
    }
};

connection_pool::connection_pool(amongoc_loop& loop, connection_uri uri)
    : _impl(mlib::allocate_unique<pool_impl>(loop.get_allocator(), loop, uri)) {}

connection_pool::~connection_pool() = default;

amongoc_loop& connection_pool::loop() const noexcept { return _impl->loop; }

connection_pool::member::member(connection_pool::member_impl_list&& impl) noexcept
    : _single_impl(mlib_fwd(impl)) {}
connection_pool::member::member(member&&)                             = default;
connection_pool::member& connection_pool::member::operator=(member&&) = default;

co_task<connection_pool::member> connection_pool::checkout() {
    // XXX: GCC 14 bug: GCC does not properly call the coroutine promise
    // `operator new` and constructors for non-static coroutine member
    // functions. Implement as a static function so that the coroutine
    // gets our allocator.
    return pool_impl::checkout(*_impl);
}

struct connection_pool::member_impl {
    // The connection pool that owns this object
    pool_impl* _owner;
    // The wrapped raw connection
    wire::client<tcp_connection_rw_stream> _client;
    // The handshake response from when this connection was initialized
    handshake_response handshake;

    int  _id         = _owner->_conn_ids.fetch_add(1);
    int  _generation = _owner->_generation.load();
    bool _perished   = false;
};

connection_pool::member_impl* connection_pool::member::_impl() noexcept {
    if (_single_impl.empty()) {
        return nullptr;
    }
    return &_single_impl.front();
}

const connection_pool::member_impl* connection_pool::member::_impl() const noexcept {
    if (_single_impl.empty()) {
        return nullptr;
    }
    return &_single_impl.front();
}

wire::client<tcp_connection_rw_stream>& connection_pool::member::_wire_client() noexcept {
    return _single_impl.front()._client;
}

mlib::allocator<> connection_pool::member::get_allocator() const noexcept {
    return _impl()->handshake.get_allocator();
}

void connection_pool::member::_perish() noexcept { _impl()->_perished = true; }

connection_pool::member::~member() {
    if (_impl() and not _impl()->_perished) {
        auto& pool = *_impl()->_owner;
        pool.return_(std::move(_single_impl));
    }
}
