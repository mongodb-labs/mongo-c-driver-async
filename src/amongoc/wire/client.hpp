#pragma once

#include <amongoc/loop.hpp>
#include <amongoc/status.h>
#include <amongoc/wire/error.hpp>
#include <amongoc/wire/message.hpp>
#include <amongoc/wire/proto.hpp>
#include <amongoc/wire/stream.hpp>

#include <mlib/alloc.h>

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
 * @tparam Stream The wrapped stream
 */
template <writable_stream Stream>
class client {
public:
    client() = default;
    explicit client(Stream&& strm, mlib::allocator<> a)
        : _stream(mlib_fwd(strm))
        , _alloc(a) {}

    mlib::allocator<> get_allocator() const noexcept { return _alloc; }

    template <message_type Message>
    co_task<any_message> request(Message&& msg) {
        return request(*this, mlib_fwd(msg));
    }

private:
    Stream            _stream;
    mlib::allocator<> _alloc;
    std::int32_t      _request_id = 0;

    static co_task<any_message> request(client& self, message_type auto msg) {
        co_await wire::send_message(self.get_allocator(),
                                    self._stream,
                                    self._request_id++,
                                    mlib_fwd(msg));
        co_return co_await wire::recv_message(self.get_allocator(), self._stream);
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
