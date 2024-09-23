#pragma once

#include "./error.hpp"
#include "./integer.hpp"
#include "./stream.hpp"

#include <amongoc/asio/as_sender.hpp>
#include <amongoc/bson/build.h>
#include <amongoc/coroutine.hpp>
#include <amongoc/loop.hpp>
#include <amongoc/string.hpp>
#include <amongoc/vector.hpp>
#include <amongoc/wire/buffer.hpp>

#include <mlib/alloc.h>

#include <asio/buffer.hpp>
#include <asio/buffers_iterator.hpp>
#include <asio/completion_condition.hpp>
#include <asio/read.hpp>
#include <neo/views/concat.hpp>

#include <ranges>
#include <utility>
#include <variant>

namespace amongoc::wire {

/**
 * @brief A type that provides an interface for wire messages
 */
template <typename T>
concept message_type = requires(const T& req, allocator<> a) {
    // The object must provide an opcode
    { req.opcode() } -> std::convertible_to<std::int32_t>;
    // The object must provide a buffer sequence to be attached to the message
    { req.buffers(a) } -> const_buffer_sequence;
};

/**
 * @brief A type that provides section content for OP_MSG messages
 */
template <typename T>
concept section_type = requires(const T& sec, allocator<> const a) {
    // Must provide a kind byte to be attached to the section. This is a reference because it will
    // be passed by-address through the stream
    { sec.kind() } noexcept -> std::same_as<std::uint8_t const&>;
    // Must provide a buffer sequence to attach to the section
    { sec.buffers(a) } -> const_buffer_sequence;
};

/**
 * @brief A wire protocol section that includes a single BSON document
 *
 * @tparam BSON The document type. Must have `data()` and `byte_size()`
 */
template <typename BSON>
struct body_section {
    // The BSON document that goes in the section
    BSON body;

    // Kind byte for body messages
    static constexpr std::uint8_t kind_byte = 0;

    asio::const_buffers_1 buffers(allocator<>) const noexcept {
        // The rest of the message here
        return asio::buffer(body.data(), body.byte_size());
    }

    std::uint8_t const& kind() const noexcept {
        return kind_byte;  // Kind has byte value zero
    }
};

/**
 * @brief Create an OP_MSG message
 *
 * @tparam Sections A range of `section_type` objects that provide sections for the message
 */
template <typename Sections>
    requires std::ranges::forward_range<Sections>
    and section_type<std::ranges::range_reference_t<Sections>>
class op_msg_message {
public:
    op_msg_message() = default;
    explicit op_msg_message(Sections&& s)
        : _sections(mlib_fwd(s)) {}

private:
    // Flag bits are always zero (For now)
    char _flag_bits[4] = {0};

    Sections _sections;

public:
    static std::int32_t opcode() noexcept {
        return 2013;  // OP_MSG
    }

    const_buffer_sequence auto buffers(allocator<> a) const noexcept {
        return _buffers(a, _sections);
    }

    Sections&       sections() noexcept { return _sections; }
    const Sections& sections() const noexcept { return _sections; }

private:
    const_buffer_sequence auto _buffers(allocator<> a, auto& sections) const noexcept {
        vector<asio::const_buffer> bufs{a};
        auto                       flag_bits_buf = asio::buffer(_flag_bits);
        bufs.push_back(flag_bits_buf);
        for (section_type auto& sec : _sections) {
            auto k = asio::const_buffer(&sec.kind(), 1);
            bufs.push_back(k);
            auto more = sec.buffers(a);
            bufs.insert(bufs.end(), more.begin(), more.end());
        }
        return bufs;
    }

    // Optimize: We are attaching a fixed number of body sections, so we don't
    // need to dynamically allocate our buffers
    template <typename BSON, std::size_t N>
    const_buffer_sequence auto
    _buffers(allocator<> a, const std::array<body_section<BSON>, N>& sections) const noexcept {
        return _buffers_1(a, sections, std::make_index_sequence<N>{});
    }

    template <typename BSON, std::size_t N, std::size_t... Ns>
    const_buffer_sequence auto _buffers_1(allocator<>                              a,
                                          const std::array<body_section<BSON>, N>& sections,
                                          std::index_sequence<Ns...>) const noexcept {
        std::array<asio::const_buffer, 1 + (N * 2)> buffers{};
        buffers[0] = asio::buffer(_flag_bits);
        ((buffers[1 + (Ns * 2)] = asio::const_buffer(&std::get<Ns>(sections).kind(), 1)), ...);
        ((buffers[2 + (Ns * 2)] = std::get<Ns>(sections).buffers(a)), ...);
        return buffers;
    }
};

template <typename S>
explicit op_msg_message(S&&) -> op_msg_message<S>;

/**
 * @brief Dynamically typed OP_MSG section type
 */
class any_section {
    using var_type = std::variant<body_section<bson::document>>;

public:
    explicit any_section(std::convertible_to<var_type> auto&& sec)
        : _variant(mlib_fwd(sec)) {}

    template <typename F>
    decltype(auto) visit(F&& fn) {
        return std::visit(mlib_fwd(fn), _variant);
    }

    template <typename F>
    decltype(auto) visit(F&& fn) const {
        return std::visit(mlib_fwd(fn), _variant);
    }

    std::uint8_t const& kind() const noexcept {
        return visit([&](auto&& x) -> decltype(auto) { return x.kind(); });
    }

    vector<asio::const_buffer> buffers(allocator<> a) const {
        vector<asio::const_buffer> bufs{a};
        this->visit([&](auto&& sec) {
            auto k = asio::const_buffer(&sec.kind(), 1);
            bufs.push_back(k);
            auto bs = sec.buffers(a);
            bufs.insert(bufs.begin(),
                        asio::buffer_sequence_begin(bs),
                        asio::buffer_sequence_end(bs));
        });
        return bufs;
    }

    /**
     * @brief Read an unknown-typed message section from a dynamic buffer
     *
     * @param dbuf The buffer which will be consumed
     * @param a
     * @return any_section
     */
    static any_section read(dynamic_buffer_v1 auto&& dbuf, allocator<> a) {
        const_buffer_sequence auto data = dbuf.data();
        if (asio::buffer_size(data) < 1) {
            throw_protocol_error("short read");
        }
        auto kind = static_cast<std::uint8_t>(*buffers_unbounded(data).begin());
        dbuf.consume(1);
        data = dbuf.data();
        switch (kind) {
            // Regular body
        case 0: {
            if (asio::buffer_size(data) < 5) {  // min doc length
                throw_protocol_error("short read");
            }
            const auto bson_len = read_int_le<std::int32_t>(buffers_unbounded(data)).value;
            if (asio::buffer_size(data) < bson_len) {
                throw_protocol_error("short read");
            }
            bson::document ret{a};
            ret.resize_and_overwrite(bson_len, [&](bson_byte* out) {
                asio::buffer_copy(asio::buffer(out, bson_len), dbuf.data());
            });
            dbuf.consume(bson_len);
            return any_section(body_section(mlib_fwd(ret)));
        }
        default:
            throw_protocol_error("unknown section kind");
        }
    }

private:
    var_type _variant;
};

using any_op_msg_message = op_msg_message<vector<any_section>>;

/**
 * @brief A message with a dynamic content type
 */
class any_message {
    using var_type = std::variant<any_op_msg_message>;

public:
    explicit any_message(std::int32_t                         request_id,
                         std::int32_t                         response_to,
                         std::convertible_to<var_type> auto&& content)
        : _req_id(request_id)
        , _resp_to(response_to)
        , _content(mlib_fwd(content)) {}

    const var_type& content() const noexcept { return _content; }

    decltype(auto) visit_content(auto&& fn) { return std::visit(mlib_fwd(fn), content()); }
    decltype(auto) visit_content(auto&& fn) const { return std::visit(mlib_fwd(fn), content()); }

    std::int32_t opcode() const noexcept {
        return visit_content([&](const auto& c) { return c.opcode(); });
    }

    const_buffer_sequence auto buffers(allocator<> a) const {
        return visit_content(
            [&](const auto& c) -> const_buffer_sequence decltype(auto) { return c.buffers(a); });
    }

    const bson::document& expect_one_body_section_op_msg() const& noexcept;
    bson::document&       expect_one_body_section_op_msg() & noexcept {
        return const_cast<bson::document&>(std::as_const(*this).expect_one_body_section_op_msg());
    }
    bson::document&& expect_one_body_section_op_msg() && noexcept {
        return std::move(this->expect_one_body_section_op_msg());
    }

private:
    std::int32_t _req_id;
    std::int32_t _resp_to;
    var_type     _content;
};

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
    // Create a bufffer for the message header
    auto MsgHeader_buf = asio::buffer(MsgHeader_chars);
    // Join the message header with the buffers for the message content
    const_buffer_sequence auto all_buffers = neo::views::concat(MsgHeader_buf, content_buffers);
    // Perform the write
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
co_task<any_message> recv_message(allocator<> a, Stream& strm) {
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
    const auto messageLength = read_int_le<std::int32_t>(MsgHeader_dbuf);
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
        co_return any_message(requestID, responseTo, op_msg_message(mlib_fwd(sections)));
    }
    // OP_MSG
    default:
        throw_protocol_error("unknown MsgHeader.opCode");
    }
}

// Externally compile this common specialization
extern template co_task<any_message> recv_message(allocator<>, tcp_connection_rw_stream&);

}  // namespace amongoc::wire
