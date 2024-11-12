/**
 * @file loop.hpp
 * @brief High-level abstractions around amongoc_loop
 * @date 2024-08-14
 *
 */
#pragma once

#include "./nano/result.hpp"
#include "./nano/simple.hpp"

#include <amongoc/box.hpp>
#include <amongoc/emitter_result.hpp>
#include <amongoc/handler.h>
#include <amongoc/loop.h>
#include <amongoc/wire/buffer.hpp>

#include <mlib/algorithm.hpp>
#include <mlib/alloc.h>
#include <mlib/object_t.hpp>

#include <asio/buffer.hpp>
#include <boost/container/static_vector.hpp>

#include <cstddef>

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
                                unique_handler::from(loop->get_allocator(),
                                                     [f = mlib_fwd(fn)](emitter_result&&) mutable {
                                                         static_cast<F&&>(f)();
                                                     })
                                    .release());
    }
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

    // Obtain an allocator for the stream. Pulls the allocator from the event loop
    mlib::allocator<> get_allocator() const noexcept { return loop->get_allocator(); }

    // A completion handler for a unique_handle that calls an Asio callback
    template <typename C>
    struct transfer_completer {
        C cb;

        void operator()(emitter_result&& res_nbytes) {
            std::move(cb)(res_nbytes.status.as_error_code(), res_nbytes.value.as<std::size_t>());
        }
    };

    /**
     * @brief Implement reading for Asio AsyncReadStream
     *
     * @param buf The mutable destination buffers
     * @param cb The completion callback
     *
     * NOTE: This doesn't fully conform to the AsyncReadStream, which requires genericity
     * around the completion token. Here, we only accept a regular completion handler (no
     * CompletionToken support here).
     *
     * This works with the current Asio behavior and how we use the Asio APIs, but could potentially
     * break if Asio changes significantly or we use an API that sends us a CompletionToken. The
     * failure-mode in this case is a hard compile error, not runtime misbehavior.
     *
     * Because this is part of a private API, as long as it compiles, it is good enough for us.
     */
    template <wire::mutable_buffer_sequence Bufs, typename C>
    void async_read_some(Bufs&& bufs, C&& cb) {
        boost::container::static_vector<::amongoc_mutable_buffer, max_vec_bufs> buf_vec;
        mlib::copy_to_capacity(wire::buffer_sequence_as_range(bufs)
                                   | std::views::transform(_asio_buf_to_amc_buf),
                               buf_vec);
        loop->vtable->tcp_read_some(loop,
                                    conn,
                                    buf_vec.data(),
                                    buf_vec.size(),
                                    unique_handler::from(get_allocator(),
                                                         transfer_completer<C>{mlib_fwd(cb)})
                                        .release());
    }

    /**
     * @brief Implement writing for Asio AsyncWriteStream
     *
     * @param buf The buffers to be written to the socket
     * @param cb The completion callback
     *
     * NOTE: See the NOTE above on async_read_some for caveats
     */
    template <wire::const_buffer_sequence Bufs, typename C>
    void async_write_some(Bufs&& bufs, C&& cb) {
        boost::container::static_vector<::amongoc_const_buffer, max_vec_bufs> buf_vec;
        mlib::copy_to_capacity(wire::buffer_sequence_as_range(bufs)
                                   | std::views::transform(_asio_buf_to_amc_buf),
                               buf_vec);
        loop->vtable->tcp_write_some(loop,
                                     conn,
                                     buf_vec.data(),
                                     buf_vec.size(),
                                     unique_handler::from(get_allocator(),
                                                          transfer_completer<C>(mlib_fwd(cb)))
                                         .release());
    }

private:
    static constexpr std::size_t max_vec_bufs = 16;

    static inline constexpr struct {
        ::amongoc_const_buffer operator()(asio::const_buffer cb) noexcept {
            return ::amongoc_const_buffer{cb.data(), cb.size()};
        }

        ::amongoc_mutable_buffer operator()(asio::mutable_buffer mb) noexcept {
            return ::amongoc_mutable_buffer{mb.data(), mb.size()};
        }
    } _asio_buf_to_amc_buf{};
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
        return simple_operation(
            [r = mlib::as_object(mlib_fwd(recv)), name, svc, &loop] mutable -> void {
                loop.vtable->getaddrinfo(  //
                    &loop,
                    name,
                    svc,
                    as_handler(atop(mlib::unwrap_object(std::move(r)),
                                    result_fmap<construct<address_info>>{}))
                        .release());
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
        [ai = std::move(ai),
         &loop]<nanoreceiver_of<result<tcp_connection_rw_stream>> R>(R&& recv) mutable {
            return simple_operation(
                [ai = std::move(ai), r = mlib::as_object(mlib_fwd(recv)), &loop] mutable {
                    loop.vtable->tcp_connect(  //
                        &loop,
                        ai.box,
                        as_handler(atop(mlib::unwrap_object(std::move(r)),
                                        result_fmap([&loop](unique_box b) {
                                            return tcp_connection_rw_stream(&loop, std::move(b));
                                        })))
                            .release());
                });
        });
}

}  // namespace amongoc
