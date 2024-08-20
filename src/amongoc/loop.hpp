/**
 * @file loop.hpp
 * @brief High-level abstractions around amongoc_loop
 * @date 2024-08-14
 *
 */
#pragma once

#include "./nano/concepts.hpp"
#include "./nano/first.hpp"
#include "./nano/result.hpp"
#include "./nano/simple.hpp"

#include <amongoc/box.h>
#include <amongoc/handler.h>
#include <amongoc/loop.h>

#include <asio/awaitable.hpp>
#include <asio/buffer.hpp>
#include <neo/object_box.hpp>
#include <neo/unit.hpp>

#include <chrono>
#include <cstddef>
#include <memory>
#include <system_error>
#include <variant>

namespace amongoc {

// Provide an Asio executor. This is needed for tcp_connect_rw_stream to be a valid Asio stream
struct amongoc_loop_asio_executor {
    amongoc_loop* loop;

    bool operator==(const amongoc_loop_asio_executor&) const noexcept = default;

    template <typename F>
    void execute(F&& fn) const {
        loop->vtable->call_soon(loop,
                                amongoc_okay,
                                amongoc_nil,
                                unique_handler::from([f = NEO_FWD(fn)](auto, auto) mutable {
                                    static_cast<F&&>(f)();
                                }).release());
    }
};

/**
 * @brief A C++ Allocator that allocates using an event loop's allocation functions
 */
template <typename T = void>
class loop_allocator {
public:
    using value_type = T;
    using pointer    = value_type*;

    explicit loop_allocator(amongoc_loop& loop)
        : _loop(&loop) {}

    template <typename U>
    loop_allocator(loop_allocator<U> other) noexcept
        : _loop(&other.get_loop()) {}

    amongoc_loop& get_loop() const noexcept { return *_loop; }

    pointer allocate(std::size_t n) {
        if (_loop->vtable->allocate) {
            return static_cast<pointer>(_loop->vtable->allocate(_loop, n * sizeof(T)));
        } else {
            std::allocator<T> a;
            return std::allocator_traits<std::allocator<T>>::allocate(a, n);
        }
    }

    void deallocate(pointer p, std::size_t n) {
        if (_loop->vtable->deallocate) {
            _loop->vtable->deallocate(_loop, p);
        } else {
            std::allocator<T> a;
            std::allocator_traits<std::allocator<T>>::deallocate(a, p, n);
        }
    }

    bool operator==(loop_allocator o) const noexcept { return _loop == o._loop; }

private:
    amongoc_loop* _loop;
};

/**
 * @brief An Asio AsyncWriteStream object based on a `amongoc_loop` and a `unique_box`
 * obtained from `tcp_connect`.
 *
 * This object allows using our own TCP async I/O interfaces with the Asio I/O algorithms,
 * no `asio::io_context` required!
 */
struct tcp_connection_rw_stream {
    amongoc_loop* loop;
    unique_box    conn;

    // XXX: Required for Asio to accept us as a real AsyncWriteStream, although I'm not sure it ever
    // uses this executor since it never tries to construct one?
    using executor_type = amongoc_loop_asio_executor;

    /**
     * @brief Implement reading for Asio AsyncReadStream
     *
     * @param buf The mutable buffer destination for data
     * @param cb The completion callback
     *
     * NOTE: This doesn't fully conform to the AsyncReadStream, which requires genericity
     * around the accepted buffer and the completion callback. Here, we only accept a single
     * buffer and expect a regular completion handler (no CompletionToken support here).
     *
     * This works with the current Asio behavior and how we use the Asio APIs, but could potentially
     * break if Asio changes significantly or we use an API that sends us a CompletionToken. The
     * failure-mode in this case is a hard compile error, not runtime misbehavior.
     *
     * Because this is part of a private API, as long as it compiles, it is good enough for us.
     */
    void async_read_some(asio::mutable_buffer buf, auto cb) {
        loop->vtable->tcp_read_some(loop,
                                    conn,
                                    static_cast<char*>(buf.data()),
                                    buf.size(),
                                    unique_handler::from([cb](status     st,
                                                              unique_box nbytes) mutable {
                                        cb(st.as_error_code(), nbytes.as<std::size_t>());
                                    }).release());
    }

    /**
     * @brief Implement writing for Asio AsyncWriteStream
     *
     * @param buf The buffer to be written to the socket
     * @param cb The completion callback
     *
     * NOTE: See the NOTE above on async_read_some for caveats
     */
    void async_write_some(asio::const_buffer buf, auto cb) {
        loop->vtable->tcp_write_some(loop,
                                     conn,
                                     static_cast<const char*>(buf.data()),
                                     buf.size(),
                                     unique_handler::from([cb](status     st,
                                                               unique_box nwritten) mutable {
                                         cb(st.as_error_code(), nwritten.as<std::size_t>());
                                     }).release());
    }
};

struct address_info {
    unique_box box;
};

/**
 * @brief High-level name resolution around an amongoc_loop
 *
 * @param loop The event loop
 * @param name The host name to resolve
 * @param svc The service/port hint
 * @return nanosender_of<result<address_info>> A nanosender that resolves with
 * an `address_info`
 */
inline nanosender_of<result<address_info>> auto
async_resolve(amongoc_loop& loop, const char* name, const char* svc) {
    return make_simple_sender<result<address_info>>([=, &loop](auto&& recv) {
        return simple_operation([r = neo::object_box(NEO_FWD(recv)), name, svc, &loop] mutable {
            loop.vtable->getaddrinfo(  //
                &loop,
                name,
                svc,
                as_handler(atop(r.forward(), result_fmap(construct<address_info>))).release());
        });
    });
}

/**
 * @brief High-level TCP connecting with an amongoc_loop
 *
 * @param loop The event loop
 * @param ai The `address_info` object sent by async_resolve
 * @return nanosender_of<result<tcp_connection_rw_stream>> A nanosender that resolves
 * with a new TCP connection handle.
 */
inline nanosender_of<result<tcp_connection_rw_stream>> auto async_connect(amongoc_loop&  loop,
                                                                          address_info&& ai) {
    return make_simple_sender<result<tcp_connection_rw_stream>>(
        [ai = NEO_MOVE(ai),
         &loop](nanoreceiver_of<result<tcp_connection_rw_stream>> auto&& recv) mutable {
            return simple_operation(
                [ai = NEO_MOVE(ai), r = neo::object_box(NEO_FWD(recv)), &loop] mutable {
                    loop.vtable->tcp_connect(  //
                        &loop,
                        ai.box,
                        as_handler(atop(r.forward(), result_fmap([&loop](unique_box b) {
                                            return tcp_connection_rw_stream(&loop, NEO_MOVE(b));
                                        })))
                            .release());
                });
        });
}

}  // namespace amongoc