#pragma once

#include "./asio/as_sender.hpp"
#include "./asio/read_write.hpp"
#include "./nano/concepts.hpp"
#include "./nano/just.hpp"
#include "./nano/let.hpp"
#include "./nano/result.hpp"
#include "./nano/then.hpp"

#include <amongoc/bson/build.h>
#include <amongoc/bson/view.h>
#include <amongoc/client.h>

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
#include <system_error>
#include <type_traits>
#include <variant>

namespace amongoc {

template <typename T>
    requires readable_stream<T> and writable_stream<T>
class client {
public:
    explicit client(T&& sock)
        : _socket(NEO_FWD(sock)) {}

    constexpr nanosender_of<result<bson_doc>> auto send_op_msg(bson_view doc) {
        // Building the message in this string:
        std::string snd_buf;
        auto        dbuf   = asio::dynamic_buffer(snd_buf);
        const auto  req_id = _req_id.fetch_add(1);
        // Build the operation:
        _build_op_msg(dbuf, req_id, doc);
        // Use a just() to barrier the initiation of the write operation
        return amongoc::just(std::monostate{})
            // Write the send-buffer
            | amongoc::let([this, s = NEO_MOVE(snd_buf)](auto) {
                   return asio::async_write(socket(), asio::buffer(s), asio_as_nanosender);
               })
            // Read back a message header's worth of data
            | amongoc::let(result_fmap{std::move([this](std::size_t) {
                   return asio::async_read(socket(),
                                           asio::dynamic_buffer(_recv_buf),
                                           asio::transfer_exactly(_msg_header_size),
                                           asio_as_nanosender);
               })})
            // Read a full message
            | amongoc::then(result_join)
            | amongoc::let(result_fmap{[this](std::size_t n) {
                   assert(n == _msg_header_size);
                   assert(_recv_buf.size() == _msg_header_size);
                   // Read a LE-uint32 from the beginning of the receive buffer. This is the
                   // total message size
                   const auto sz = _read_int_le<std::uint32_t>(asio::buffer(_recv_buf));
                   // The message must be at least as large as the message header
                   assert(sz >= _msg_header_size);
                   // Read the remainder of the message.
                   auto remaining = sz - _msg_header_size;
                   return asio::async_read(socket(),
                                           asio::dynamic_buffer(_recv_buf),
                                           asio::transfer_exactly(remaining),
                                           asio_as_nanosender);
               }})
            // Parse the message into a bson_doc
            | amongoc::then(result_join) | amongoc::then(result_fmap{[this](std::size_t) {
                   auto dbuf         = asio::dynamic_buffer(_recv_buf);
                   auto section_data = dbuf.data() + _msg_header_size + sizeof(std::uint32_t);
                   auto k            = asio::buffers_begin(section_data)[0];
                   assert(k == 0);  // We only handle Body kind sections yet
                   // Read the document header that specifies the document size
                   auto doc_size = _read_int_le<std::uint32_t>(section_data + 1);
                   // Create the document.
                   bson_doc body;
                   body.resize_and_overwrite(doc_size, [&](auto out) {
                       // TODO: The document content should be validated
                       asio::buffer_copy(asio::buffer(out, doc_size), section_data + 1);
                   });
                   return NEO_MOVE(body);
               }})  //
            | amongoc::then([](result<bson_doc, asio::error_code>&& r) -> result<bson_doc> {
                   if (r.has_error()) {
                       return amongoc::error(status::from(r.error()));
                   } else {
                       return amongoc::success(NEO_MOVE(r.value()));
                   }
               });
    }

    T&       socket() noexcept { return _socket; }
    const T& socket() const noexcept { return _socket; }

public:
    std::string _recv_buf;

    // Size of the MsgHeader (four 32-bit integers)
    static constexpr std::size_t _msg_header_size = sizeof(std::uint32_t) * 4;

    // opcode types for messages
    enum opcode_t { OP_MSG = 2013 };

    // Build an OP_MSG into `dbuf`
    void _build_op_msg(auto& dbuf, std::int32_t req_id, bson_view doc) {
        // Compute the total message size (no checksum)
        const auto message_total_size  //
            = _msg_header_size         // Message header
            + sizeof(uint32_t)         // flag bits
            + 1u                       // Section kind byte
            + doc.byte_size();         // Size of section body
        // Add a message header with a new request ID
        _append_msg_header(dbuf, message_total_size, req_id, 0, OP_MSG);
        // Add flag bits. (none set yet)
        std::uint32_t flag_bits = 0;
        _append_int_le(dbuf, flag_bits);
        // Add the body section
        _append_body_section(dbuf, doc);
    }

    // Append a message header to `dbuf`
    static void _append_msg_header(auto&         dbuf,
                                   std::uint32_t message_size,
                                   std::uint32_t request_id,
                                   std::uint32_t response_id,
                                   opcode_t      op) {
        auto mbuf = dbuf.prepare(_msg_header_size);
        _append_int_le(dbuf, message_size);       // Message size
        _append_int_le(dbuf, request_id);         // Request ID counter
        _append_int_le(dbuf, response_id);        // Response ID (unused for requests)
        _append_int_le(dbuf, std::uint32_t(op));  // Opcode
    }

    // Append a message "Body" section to `dbuf`
    void _append_body_section(auto& dbuf, bson_view doc) {
        const auto section_size = doc.byte_size() + 1u;
        const auto mbuf         = dbuf.prepare(section_size);
        // Set the first kind byte
        asio::buffers_begin(mbuf)[0] = char{0};  // kind 0 → "Body" (bson doc)
        // Append document
        asio::buffer_copy(mbuf + 1, asio::const_buffer(doc.data(), doc.byte_size()));
        // Finish this section:
        dbuf.commit(section_size);
    }

    // The socket to which we read/write messages
    T _socket;

    // Buffer of data that is waiting to be written
    std::string _pending_buf;
    // Buffer of data that is currently being written
    std::string _writing_buf;

    // Lock for modifing pending_buf
    std::mutex _mtx;

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

template <typename T>
explicit client(T&&) -> client<T>;

}  // namespace amongoc
