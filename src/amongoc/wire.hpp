#pragma once

#include <amongoc/asio/as_sender.hpp>
#include <amongoc/asio/read_write.hpp>
#include <amongoc/bson/build.h>
#include <amongoc/buffer.hpp>
#include <amongoc/coroutine.hpp>
#include <amongoc/string.hpp>
#include <amongoc/vector.hpp>

#include <mlib/alloc.h>

#include <asio/buffer.hpp>
#include <asio/buffers_iterator.hpp>
#include <asio/completion_condition.hpp>
#include <asio/read.hpp>
#include <neo/views/concat.hpp>

#include <ranges>
#include <system_error>
#include <variant>

namespace amongoc {

struct tcp_connection_rw_stream;

}  // namespace amongoc

namespace amongoc::wire {

[[noreturn]]
inline void throw_protocol_error(const char* msg) {
    throw std::system_error(std::make_error_code(std::errc::protocol_error), msg);
}

/**
 * @brief Write a little-endian encoded integer to the given output range
 *
 * @param out The destination for the encoded integer
 * @param value The integral value to be encoded
 * @return O The new position of the output iterator
 */
template <std::integral I, std::ranges::output_range<char> O>
constexpr std::ranges::iterator_t<O> write_int_le(O&& out_rng, I value) {
    // Make unsigned (ensures two's-complement for valid bit-shifting)
    const auto  uv   = static_cast<std::make_unsigned_t<I>>(value);
    auto        out  = std::ranges::begin(out_rng);
    const auto  stop = std::ranges::end(out_rng);
    std::size_t n    = 0;
    // Copy each byte of the integer in LE-order
    for (; n < sizeof uv and out != stop; ++n, ++out) {
        *out = static_cast<char>((uv >> (8 * n)) & 0xff);
    }
    if (n < sizeof uv) {
        throw_protocol_error("truncated buffer");
    }
    return out;
}

/**
 * @brief Write a little-endian encoded integer to the given dynamic buffer output
 *
 * @param dbuf A dynamic buffer which will receive the encoded integer
 * @param value The value to be encoded
 */
template <std::integral I>
void write_int_le(dynamic_buffer_v1 auto&& dbuf, I value) {
    auto out     = dbuf.prepare(sizeof value);
    auto out_rng = buffers_subrange(out);
    write_int_le(out_rng, value);
    dbuf.commit(sizeof value);
}

/**
 * @brief Result of decoding an integer from an input range
 *
 * @tparam I The integer value type
 * @tparam Iter The iterator type
 */
template <typename I, typename Iter>
struct decoded_integer {
    // The decoded integer value
    I value;
    // The input iterator position after decoding is complete
    Iter in;
};

/**
 * @brief Read a little-endian encoded integer from the given byte range
 *
 * @tparam Int The integer type to be read
 * @param it The input iterator from which we will read
 * @return A pair of the integer value and final iterator position
 */
template <typename Int, byte_range R>
constexpr decoded_integer<Int, std::ranges::iterator_t<R>> read_int_le(R&& rng) {
    using U          = std::make_unsigned_t<Int>;
    U           u    = 0;
    auto        it   = std::ranges::begin(rng);
    const auto  stop = std::ranges::end(rng);
    std::size_t n    = 0;
    for (; n < sizeof u and it != stop; ++it, ++n) {
        // Cast to unsigned byte first to prevent a sign-extension
        U b = static_cast<std::uint8_t>(*it);
        b <<= (8 * n);
        u |= b;
    }
    if (n < sizeof u) {
        // We had to stop before reading the full data
        throw_protocol_error("short read");
    }
    return {static_cast<Int>(u), it};
}

/**
 * @brief Read a little-endian encoded integer from a dynamic buffer.
 *
 * @tparam I The integer type to be read
 * @param dbuf A dynamic buffer to be used. The bytes of the integer will be consumed
 */
template <typename I>
I read_int_le(dynamic_buffer_v1 auto&& dbuf) {
    const_buffer_sequence auto data = dbuf.data();
    auto                       subr = buffers_subrange(data);
    I                          v    = read_int_le<I>(subr).value;
    dbuf.consume(sizeof(I));
    return v;
}

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
    static constexpr char kind_byte = 0;

    const_buffer_sequence auto buffers(allocator<>) const noexcept {
        // One byte for the kind
        asio::const_buffer b1 = asio::buffer(&kind_byte, 1);
        // The rest of the message here
        asio::const_buffer doc = asio::buffer(body.data(), body.byte_size());
        return std::array{b1, doc};
    }
};

/**
 * @brief Create an OP_MSG message
 *
 * @tparam Sections A range of section objects that must provide buffers
 *
 * The sections must provide a `.buffer(allocator<>)` function to return
 * a buffer sequence to be attached to the outgoing message
 */
template <typename Sections>
    requires std::ranges::forward_range<Sections>
    and requires(std::ranges::range_reference_t<Sections> sec, allocator<> a) {
            { sec.buffers(a) } -> const_buffer_sequence;
        }
class op_msg_content {
public:
    op_msg_content() = default;
    explicit op_msg_content(Sections&& s)
        : _sections(mlib_fwd(s)) {}

private:
    // Flag bits are always zero (For now)
    char _flag_bits[4] = {0};

    Sections _sections;

public:
    static std::int32_t opcode() noexcept {
        return 2013;  // OP_MSG
    }

    vector<asio::const_buffer> buffers(allocator<> a) const noexcept {
        // XXX: This could be made non-allocating if we know the exact number of buffers we will
        // generate
        vector<asio::const_buffer> bufs{a};
        auto                       flag_bits_buf = asio::buffer(_flag_bits);
        bufs.push_back(flag_bits_buf);
        for (auto& sec : _sections) {
            auto more = sec.buffers(a);
            bufs.insert(bufs.end(), more.begin(), more.end());
        }
        return bufs;
    }

    Sections&       sections() noexcept { return _sections; }
    const Sections& sections() const noexcept { return _sections; }
};

template <typename S>
explicit op_msg_content(S&&) -> op_msg_content<S>;

/**
 * @brief Dynamically typed message section type
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

    vector<asio::const_buffer> buffers(allocator<> a) {
        vector<asio::const_buffer> bufs{a};
        this->visit([&](auto&& sec) {
            const_buffer_sequence auto bs = sec.buffers(a);
            bufs.insert(bufs.begin(), bs.begin(), bs.end());
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

using any_op_msg_content = op_msg_content<vector<any_section>>;

/**
 * @brief A message with a dynamic content type
 */
class any_message {
    using var_type = std::variant<any_op_msg_content>;

public:
    explicit any_message(std::int32_t                         request_id,
                         std::int32_t                         response_to,
                         std::convertible_to<var_type> auto&& content)
        : _req_id(request_id)
        , _resp_to(response_to)
        , _content(mlib_fwd(content)) {}

    const var_type& content() const noexcept { return _content; }

    template <typename F>
    decltype(auto) visit_content(F&& fn) {
        return std::visit(mlib_fwd(fn), content());
    }

    bson_view expect_one_body_section_op_msg() const noexcept;

private:
    std::int32_t _req_id;
    std::int32_t _resp_to;
    var_type     _content;
};

/**
 * @brief Send a request on a writable stream
 *
 * @param a An allocator for the operation
 * @param strm An Asio writable stream
 * @param req_id The request ID to include
 * @param cont The content spec object
 *
 * The content `cont` must present an `opcode` that specifies the opcode integer, and must
 * have a `buffers(allocator<>)` member function that returns an Asio buffer sequence
 * that represents the message content.
 */
template <writable_stream Stream, typename Content>
co_task<mlib::unit> send_request(allocator<> a, Stream& strm, int req_id, Content cont) {
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
}

/**
 * @brief Send an OP_MSG on the given stream with a single BSON document body
 *
 * @param a An allocator for the operation
 * @param strm The stream that will send the request
 * @param req_id A request ID
 * @param doc The document to be transmitted
 */
template <writable_stream Stream>
co_task<mlib::unit>
send_op_msg_one_section(allocator<> a, Stream&& strm, int req_id, bson_view doc) {
    auto one_section = std::array{body_section{doc}};
    auto op_msg      = op_msg_content{mlib_fwd(one_section)};
    return send_request(a, strm, req_id, mlib_fwd(op_msg));
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
    std::size_t                     nread          = *co_await asio::async_read(strm,
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
    const auto opCode        = read_int_le<std::uint32_t>(MsgHeader_dbuf);
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
        co_return any_message(requestID, responseTo, op_msg_content(mlib_fwd(sections)));
    }
    // OP_MSG
    default:
        throw_protocol_error("unknown MsgHeader.opCode");
    }
}

// Externally compile these specializations
extern template co_task<mlib::unit>
send_op_msg_one_section(allocator<> a, tcp_connection_rw_stream&, int req_id, bson_view doc);
extern template co_task<any_message> recv_message(allocator<>, tcp_connection_rw_stream&);

}  // namespace amongoc::wire
