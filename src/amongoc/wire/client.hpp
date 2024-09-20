#pragma once

#include <amongoc/wire/proto.hpp>
#include <amongoc/wire/stream.hpp>

#include <mlib/alloc.h>

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
    explicit client(Stream&& strm, allocator<> a)
        : _stream(mlib_fwd(strm))
        , _alloc(a) {}

    allocator<> get_allocator() const noexcept { return _alloc; }

    template <message_type Message>
    co_task<any_message> request(Message&& msg) {
        return request(*this, mlib_fwd(msg));
    }

private:
    Stream       _stream;
    allocator<>  _alloc;
    std::int32_t _request_id = 0;

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

}  // namespace amongoc::wire
