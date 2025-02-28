#pragma once

#include <amongoc/loop.hpp>
#include <amongoc/status.h>
#include <amongoc/tls/stream.hpp>
#include <amongoc/wire/error.hpp>
#include <amongoc/wire/message.hpp>
#include <amongoc/wire/proto.hpp>
#include <amongoc/wire/stream.hpp>

#include <mlib/alloc.h>
#include <mlib/object_t.hpp>

#include <system_error>

namespace amongoc::wire {

/**
 * @brief Match a type that provides the basic interface for a wire-protocol client
 */
template <typename T>
concept client_interface
    = mlib::has_mlib_allocator<T> and requires(T client, any_message msg) { client.request(msg); };

/**
 * @brief A wire client that wraps a write-stream and tracks request IDs.
 *
 * @tparam Stream The wrapped stream type
 *
 * A client wrapper can be TLS-enabled if it is constructed with a `tls::stream` that
 * holds the underlying stream type
 */
template <writable_stream Stream>
class client {
public:
    client() = default;
    /**
     * @brief Construct a new client that uses a plaintext stream
     *
     * @param strm The stream to be adopted
     * @param a The allocator for the client
     */
    explicit client(Stream&& strm, mlib::allocator<> a)
        : _stream_var(std::in_place_index<0>, mlib_fwd(strm))
        , _alloc(a) {}

    /**
     * @brief Construct a new client that uses a TLS-enabled stream
     *
     * @param strm A TLS wrapper around the underlying stream
     * @param a The allocator for the client
     */
    explicit client(tls::stream<Stream>&& strm, mlib::allocator<> a)
        : _stream_var(std::in_place_index<1>, mlib_fwd(strm))
        , _alloc(a) {}

    // Get the associated allocator
    mlib::allocator<> get_allocator() const noexcept { return _alloc; }

    /**
     * @brief Issue an asynchronous request on the client
     *
     * @param msg The message to be transmitted
     * @return co_task<any_message> The response from the peer
     */
    template <message_type Message>
    co_task<any_message> request(Message&& msg) {
        // Unpack the variant stream and issue the request. This is the point where
        // we branch between plaintext/TLS streams.
        return std::visit(
            [&](auto& stream) -> co_task<any_message> {
                return _request(*this, mlib::unwrap_object(stream), mlib_fwd(msg));
            },
            _stream_var);
    }

private:
    /**
     * @brief Variant holding either a plaintext stream, or a TLS-wrapper around a plaintext stream.
     */
    std::variant<mlib::object_t<Stream>, amongoc::tls::stream<Stream>> _stream_var;

    // The associated allocator
    mlib::allocator<> _alloc;
    // The request ID. Monotonically increasing with each request
    std::int32_t _request_id = 0;

    /**
     * @brief Issue a request on the given associated stream
     *
     * @param self Reference to myself
     * @param stream Reference to the stream. Either plaintext, or the TLS wrapper
     * @param msg The message to be issued
     */
    static co_task<any_message> _request(client& self, auto& stream, message_type auto msg) {
        co_await wire::send_message(self.get_allocator(),
                                    stream,
                                    self._request_id++,
                                    mlib_fwd(msg));
        co_return co_await wire::recv_message(self.get_allocator(), stream);
    }
};

template <writable_stream S>
explicit client(S&&, auto&&...) -> client<S>;

/**
 * @brief Pass a wire client by reference
 */
template <typename C>
struct client_ref {
    C& _client;

    co_task<any_message> request(message_type auto&& m) const {
        return _client.request(mlib_fwd(m));
    }

    mlib::allocator<> get_allocator() const noexcept { return mlib::get_allocator(_client); }
};

template <typename C>
explicit client_ref(const C&) -> client_ref<C>;

/**
 * @brief Issue a single wire request on a client
 *
 * @param cl The client to issue the request
 * @param body A BSON document containing the command to be sent
 * @return co_task<bson::document>
 */
co_task<bson::document> simple_request(client_interface auto cl, auto body) {
    any_message r = co_await cl.request(op_msg_message{std::array{body_section(body)}});
    co_return mlib_fwd(r).expect_one_body_section_op_msg();
}

/**
 * @brief A client adaptor that automatically retries requests when they throw an exception
 *
 * @tparam Client The wrapped client
 */
template <typename Client>
class retrying_client {
public:
    retrying_client() = default;
    // Wrap a client with the given number of retries on each request
    explicit retrying_client(Client&& cl, int n_tries = 5)
        : _client(mlib_fwd(cl))
        , _n_tries(n_tries) {}

    mlib::allocator<> get_allocator() const noexcept { return _client.get_allocator(); }

    co_task<wire::any_message> request(message_type auto&& msg) {
        return _request(*this, mlib_fwd(msg));
    }

private:
    Client _client;
    int    _n_tries = 5;

    static co_task<wire::any_message> _request(retrying_client& self, message_type auto msg) {
        int tries_remaining = self._n_tries;
        for (;;) {
            try {
                co_return co_await self._client.request(mlib_fwd(msg));
            } catch (const std::system_error& err) {
                if (--tries_remaining < 1) {
                    throw;
                }
            }
        }
    }
};

template <typename C>
explicit retrying_client(C&&, int = 0) -> retrying_client<C>;

/**
 * @brief A client adaptor that checks whether the given server response contains
 * an error result.
 *
 * If the response contains an error, raises `std::system_error` with the `amongoc::server_category`
 */
template <typename C>
struct checking_client {
    C _client;

    mlib::allocator<> get_allocator() const noexcept { return _client.get_allocator(); }

    co_task<wire::any_message> request(message_type auto&& msg) {
        return _request(*this, mlib_fwd(msg));
    }

private:
    static co_task<wire::any_message> _request(checking_client& self, message_type auto msg) {
        auto resp = co_await self._client.request(mlib_fwd(msg));
        throw_if_message_error(resp);
        co_return resp;
    }
};

template <typename C>
explicit checking_client(C&&) -> checking_client<C>;

}  // namespace amongoc::wire
