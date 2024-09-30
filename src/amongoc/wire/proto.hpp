#pragma once

#include "./error.hpp"
#include "./integer.hpp"
#include "./stream.hpp"

#include <amongoc/asio/as_sender.hpp>
#include <amongoc/coroutine.hpp>
#include <amongoc/loop.hpp>
#include <amongoc/string.hpp>
#include <amongoc/vector.hpp>
#include <amongoc/wire/buffer.hpp>
#include <amongoc/wire/message.hpp>

#include <bson/doc.h>

#include <mlib/alloc.h>

#include <asio/buffer.hpp>
#include <asio/buffers_iterator.hpp>
#include <asio/completion_condition.hpp>
#include <asio/read.hpp>
#include <neo/views/concat.hpp>

namespace amongoc::wire {

/**
 * @brief Message tracing APIs
 */
namespace trace {

// Global toggle for enabling message tracing
constexpr bool enabled = false;

// Print information for a message header
void message_header(std::string_view prefix,
                    std::size_t      messageLength,
                    int              requestID,
                    std::int32_t     opcode);

// Print the BSON document that contains a message body
void message_body_section(int nth, bson_view body);

// Print a message body section
template <typename B>
void message_section(int nth, const body_section<B>& body) {
    trace::message_body_section(nth, bson_view(body.body));
}

// Print a dynamically typed section
void message_section(int nth, const any_section& sec);

// Print the content of a message. Must expose a range of printable sections
template <typename C>
void message_content(const C& c)
    requires requires {
        c.sections();
        { *c.sections().begin() } -> section_type;
    }
{
    int nth = 1;
    for (section_type auto const& sec : c.sections()) {
        trace::message_section(nth, sec);
    }
}

// Print the content of a dynamic message type
void message_content(const any_message& msg);

// Print information about a message begin sent
template <typename C>
void message_send(std::size_t messageLength, int requestID, const C& content) {
    auto opcode = content.opcode();
    trace::message_header("send", messageLength, requestID, opcode);
    trace::message_content(content);
}

// Print information about a message that was received
template <typename C>
void message_recv(std::size_t messageLength, int responseTo, const C& content) {
    auto opcode = content.opcode();
    trace::message_header("recv", messageLength, responseTo, opcode);
    trace::message_content(content);
}

void message_exception(const char* msg);

}  // namespace trace

/**
 * @brief Send a request message on a writable stream
 *
 * @param a An allocator for the operation
 * @param strm An Asio writable stream
 * @param req_id The request ID to include
 * @param cont The content spec object
 */
template <writable_stream Stream, message_type Content>
co_task<mlib::unit> send_message(allocator<> a, Stream& strm, int req_id, const Content cont) {
    // Get the buffers for the message
    const_buffer_sequence auto content_buffers = cont.buffers(a);
    const std::size_t          content_size    = asio::buffer_size(content_buffers);
    // Build the header buffer

    std::array<std::uint8_t, 4 * 4> MsgHeader_chars;
    auto                            MsgHeader_dbuf = generic_dynamic_buffer_v1(MsgHeader_chars);
    // MsgHeader.messageLength
    const auto total_size = sizeof MsgHeader_chars + content_size;
    write_int_le(MsgHeader_dbuf, static_cast<std::int32_t>(total_size));
    // MsgHeader.requestID
    write_int_le(MsgHeader_dbuf, static_cast<std::int32_t>(req_id));
    // MsgHeader.responseTo (zero for requests)
    write_int_le(MsgHeader_dbuf, static_cast<std::int32_t>(0));
    // MsgHeader.opCode
    const auto opcode = cont.opcode();
    write_int_le(MsgHeader_dbuf, static_cast<std::int32_t>(opcode));
    // Create a buffer for the message header
    auto MsgHeader_buf = asio::buffer(MsgHeader_chars);
    // Join the message header with the buffers for the message content
    const_buffer_sequence auto all_buffers = neo::views::concat(MsgHeader_buf, content_buffers);
    // Perform the write
    if (trace::enabled) {
        trace::message_send(content_size, req_id, cont);
    }
    *co_await asio::async_write(strm, all_buffers, asio::transfer_all(), asio_as_nanosender);
    co_return {};
}

/**
 * @brief Receive a wire protocol message from the given readable stream
 *
 * @param a An allocator for the operation
 * @param strm The stream from which to read
 * @return An `any_message` object with the contents of the message
 */
template <readable_stream Stream>
co_task<any_message> recv_message(allocator<> a, Stream& strm) try {
    std::array<std::uint8_t, 4 * 4> MsgHeader_chars;
    auto                            MsgHeader_dbuf = generic_dynamic_buffer_v1(MsgHeader_chars);

    std::size_t nread = *co_await asio::async_read(strm,
                                                   asio::buffer(MsgHeader_chars),
                                                   asio::transfer_all(),
                                                   asio_as_nanosender);
    if (nread < sizeof MsgHeader_chars) {
        throw_protocol_error("short read");
    }
    MsgHeader_dbuf.commit(nread);  // Adjust internal pointers to fit the data that was read
    const auto messageLength = read_int_le<std::uint32_t>(MsgHeader_dbuf);
    const auto requestID     = read_int_le<std::int32_t>(MsgHeader_dbuf);
    const auto responseTo    = read_int_le<std::int32_t>(MsgHeader_dbuf);
    const auto opCode        = read_int_le<std::int32_t>(MsgHeader_dbuf);
    if (messageLength < sizeof MsgHeader_chars) {
        throw_protocol_error("invalid MsgHeader.messageLength");
    }
    switch (opCode) {
    case 2013: {
        string            content{a};
        auto              content_dbuf = asio::dynamic_buffer(content);
        const std::size_t remaining    = messageLength - sizeof MsgHeader_chars;

        std::size_t nread = *co_await asio::async_read(strm,
                                                       content_dbuf,
                                                       asio::transfer_exactly(remaining),
                                                       asio_as_nanosender);
        if (nread < remaining) {
            throw_protocol_error("short read");
        }

        auto                flagBits     = read_int_le<std::int32_t>(content_dbuf);
        const bool          has_checksum = bool(flagBits & 1);
        const std::size_t   tail_size    = has_checksum ? 4 : 0;
        vector<any_section> sections{a};
        while (content_dbuf.size() > tail_size) {
            auto sec = any_section::read(content_dbuf, a);
            sections.push_back(mlib_fwd(sec));
        }
        // TODO: Checksum validation
        auto r = any_message(requestID, responseTo, op_msg_message(mlib_fwd(sections)));
        if (trace::enabled) {
            trace::message_recv(messageLength, responseTo, r);
        }
        co_return r;
    }
    // OP_MSG
    default:
        throw_protocol_error("unknown MsgHeader.opCode");
    }
} catch (...) {
    if (trace::enabled) {
        trace::message_exception("Failure while reading message");
    }
    throw;
}

// Externally compile this common specialization
extern template co_task<any_message> recv_message(allocator<>, tcp_connection_rw_stream&);

}  // namespace amongoc::wire
