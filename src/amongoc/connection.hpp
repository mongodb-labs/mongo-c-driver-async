#pragma once

#include "./asio/as_sender.hpp"
#include "./asio/read_write.hpp"
#include "./buffer.hpp"
#include "./nano/concepts.hpp"
#include "./nano/just.hpp"
#include "./nano/let.hpp"
#include "./nano/result.hpp"
#include "./nano/then.hpp"
#include "./string.hpp"

#include <amongoc/alloc.h>
#include <amongoc/bson/build.h>
#include <amongoc/bson/view.h>
#include <amongoc/connection.h>

#include <asio/buffer.hpp>
#include <asio/buffered_write_stream.hpp>
#include <asio/buffers_iterator.hpp>
#include <asio/completion_condition.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <ranges>
#include <string>
#include <system_error>
#include <type_traits>

namespace amongoc {

template <typename T, typename Alloc = std::allocator<void>>
    requires readable_stream<T> and writable_stream<T>
class raw_connection {
public:
    explicit raw_connection(Alloc alloc, T&& sock)
        : _alloc(alloc)
        , _socket(mlib_fwd(sock)) {}

    template <typename BSON>
    constexpr nanosender_of<result<bson::document>> auto send_op_msg(BSON doc)
        requires requires {
            doc.byte_size();
            doc.data();
        }
    {
        const auto req_id = _req_id.fetch_add(1);
        // Build the operation:
        std::array<char, _op_msg_prefix_size> op_msg_prefix;
        generic_dynamic_buffer_v1             dbuf{op_msg_prefix};
        _build_msg_header(dbuf, req_id, doc);
        assert(dbuf.size() == _op_msg_prefix_size);
        // Use a just() to barrier the initiation of the write operation
        return amongoc::just(mlib::unit{})
            // Write the send-buffer
            | amongoc::let([this, h = op_msg_prefix, doc = mlib_fwd(doc)](auto) {
                   // Send two buffers: The message prefix and the BSON body
                   std::array bufs{asio::buffer(h), asio::buffer(doc.data(), doc.byte_size())};
                   return asio::async_write(socket(),
                                            bufs,
                                            asio::transfer_all(),
                                            asio_as_nanosender);
               })
            // Read back a message header's worth of data.
            // XXX: The recv_buffer is stored in this closure and passed by reference
            // through the chain using pair_append()
            | amongoc::let(result_fmap{[this, recv_buffer = string(_alloc)](std::size_t n) mutable {
                   return asio::async_read(socket(),
                                           asio::dynamic_buffer(recv_buffer),
                                           asio::transfer_exactly(_msg_header_size),
                                           asio_as_nanosender)
                       | then(result_fmap(pair_append(recv_buffer)));
               }})
            // Read a full message
            | amongoc::then(result_join)
            | amongoc::let(result_fmap{unpack_args{[this](std::size_t n, string& recv_buffer) {
                   assert(n == _msg_header_size);
                   // Read a LE-uint32 from the beginning of the receive buffer. This is the
                   // total message size
                   const auto sz = _read_int_le<std::uint32_t>(asio::buffer(recv_buffer));
                   // The message must be at least as large as the message header
                   assert(sz >= _msg_header_size);
                   // Read the remainder of the message.
                   auto remaining = sz - _msg_header_size;
                   return asio::async_read(socket(),
                                           asio::dynamic_buffer(recv_buffer),
                                           asio::transfer_exactly(remaining),
                                           asio_as_nanosender)
                       | then(result_fmap(pair_append(recv_buffer)));
               }}})
            // Parse the message into a bson::document
            | amongoc::then(result_join)
            | amongoc::then(result_fmap{unpack_args{[this](std::size_t n, string& recv_buffer) {
                   auto dbuf         = asio::dynamic_buffer(recv_buffer);
                   auto section_data = dbuf.data() + _msg_header_size + sizeof(std::uint32_t);
                   auto k            = asio::buffers_begin(section_data)[0];
                   assert(k == 0);  // We only handle Body kind sections yet
                   // Read the document header that specifies the document size
                   auto doc_size = _read_int_le<std::uint32_t>(section_data + 1);
                   // Create the document.
                   bson::document body{_alloc};
                   body.resize_and_overwrite(doc_size, [&](auto out) {
                       // TODO: The document content should be validated
                       asio::buffer_copy(asio::buffer(out, doc_size), section_data + 1);
                   });
                   return std::move(body);
               }}})  //
            | amongoc::then(
                   [](result<bson::document, asio::error_code>&& r) -> result<bson::document> {
                       if (r.has_error()) {
                           return amongoc::error(status::from(r.error()));
                       } else {
                           return amongoc::success(std::move(r.value()));
                       }
                   });
    }

    T&       socket() noexcept { return _socket; }
    const T& socket() const noexcept { return _socket; }

public:
    // Size of the MsgHeader (four 32-bit integers)
    static constexpr std::size_t _msg_header_size    = sizeof(std::int32_t) * 4;
    static constexpr std::size_t _op_msg_prefix_size =  //
        _msg_header_size                                // Message header
        + sizeof(std::uint32_t)                         // flag bits
        + 1                                             // Section kind byte
        ;

    // opcode types for messages
    enum opcode_t { OP_MSG = 2013 };

    // Build an OP_MSG into `dbuf`
    void _build_msg_header(auto& dbuf, std::int32_t req_id, bson_view doc) {
        // Compute the total message size (no checksum)
        const auto message_total_size  //
            = _op_msg_prefix_size      // Message header, flags, and section byte
            + doc.byte_size();         // Size of section body
        // Add a message header with a new request ID
        _append_msg_header(dbuf, message_total_size, req_id, 0, OP_MSG);
        // Add flag bits. (none set yet)
        std::uint32_t flag_bits = 0;
        _append_int_le(dbuf, flag_bits);
        // Add a single zero byte to be the section header
        auto one                    = dbuf.prepare(1);
        asio::buffers_begin(one)[0] = 0;
        dbuf.commit(1);
    }

    // Append a message header to `dbuf`
    static void _append_msg_header(auto&         dbuf,
                                   std::uint32_t message_size,
                                   std::uint32_t request_id,
                                   std::uint32_t response_id,
                                   opcode_t      op) {
        _append_int_le(dbuf, message_size);       // Message size
        _append_int_le(dbuf, request_id);         // Request ID counter
        _append_int_le(dbuf, response_id);        // Response ID (unused for requests)
        _append_int_le(dbuf, std::uint32_t(op));  // Opcode
    }

    Alloc _alloc;

    // The socket to which we read/write messages
    T _socket;

    // Request ID counter
    std::atomic_uint32_t _req_id{1};

    /**
     * @brief Append a little-endian encoded integer
     *
     * @param dbuf The output buffer
     * @param val The integer value to append. If signed, encoded as two's completement.
     * The width of this integer will be the same as is appended to the buffer
     */
    template <typename I>
    static void _append_int_le(auto& dbuf, I val) {
        // Make unsigned (ensures two's-complement for valid bit-shifting)
        auto uv = static_cast<std::make_unsigned_t<I>>(val);
        // Output buffer
        char buf[sizeof(uv)];
        // Copy each byte of the integer in LE-order
        for (auto n : std::views::iota(0u, sizeof(uv))) {
            buf[n] = static_cast<char>((uv >> (8 * n)) & 0xff);
        }
        // Write into the final buf
        asio::buffer_copy(dbuf.prepare(sizeof buf), asio::buffer(buf));
        dbuf.commit(sizeof buf);
    }

    template <typename I>
    static I _read_int_le(auto&& buf) {
        using U = std::make_unsigned_t<I>;
        U u     = 0;
        // byte-wize iterator:
        auto bi = asio::buffers_begin(buf);
        for (auto n : std::views::iota(size_t(0), sizeof u)) {
            u <<= 8;
            u |= static_cast<U>(bi[(sizeof u) - 1 - n]);
        }
        return static_cast<I>(u);
    }
};

template <typename A, typename T>
explicit raw_connection(A, T&&) -> raw_connection<T, A>;

}  // namespace amongoc
