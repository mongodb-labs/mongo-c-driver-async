#pragma once

#include <amongoc/vector.hpp>
#include <amongoc/wire/buffer.hpp>
#include <amongoc/wire/integer.hpp>

#include <bson/doc.h>
#include <bson/view.h>

#include <mlib/alloc.h>

#include <asio/buffer.hpp>

#include <utility>
#include <variant>

namespace amongoc::wire {

/**
 * @brief A type that provides an interface for wire messages
 */
template <typename T>
concept message_type = requires(const T& req, mlib::allocator<> a) {
    // The object must provide an opcode
    { req.opcode() } -> std::convertible_to<std::int32_t>;
    // The object must provide a buffer sequence to be attached to the message
    { req.buffers(a) } -> const_buffer_sequence;
};

/**
 * @brief A type that provides section content for OP_MSG messages
 */
template <typename T>
concept section_type = requires(const T& sec, mlib::allocator<> const a) {
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

    asio::const_buffers_1 buffers(mlib::allocator<>) const noexcept {
        return asio::buffer(body.data(), body.byte_size());
    }

    std::uint8_t const& kind() const noexcept { return kind_byte; }
};

// Common body section type:
extern template struct body_section<bson_view>;
using bson_view_body_section = body_section<bson_view>;

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

    const_buffer_sequence auto buffers(mlib::allocator<> a) const noexcept {
        return _buffers(a, _sections);
    }

    Sections&       sections() noexcept { return _sections; }
    const Sections& sections() const noexcept { return _sections; }

private:
    vector<asio::const_buffer> _buffers(mlib::allocator<> a, auto& sections) const noexcept {
        vector<asio::const_buffer> bufs{a};
        auto                       flag_bits_buf = asio::buffer(_flag_bits);
        bufs.push_back(flag_bits_buf);
        for (section_type auto& sec : sections) {
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
    std::array<asio::const_buffer, 1 + (N * 2)>
    _buffers(mlib::allocator<>                        a,
             const std::array<body_section<BSON>, N>& sections) const noexcept {
        return _buffers_1(a, sections, std::make_index_sequence<N>{});
    }

    template <typename BSON, std::size_t N, std::size_t... Ns>
    std::array<asio::const_buffer, 1 + (N * 2)>
    _buffers_1(mlib::allocator<>                        a,
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

// Common case: An OP_MSG with a single BSON document body
// extern template class op_msg_message<std::array<bson_view_body_section, 1>>;
using one_bson_view_op_msg = op_msg_message<std::array<bson_view_body_section, 1>>;

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

    vector<asio::const_buffer> buffers(mlib::allocator<> a) const;

    /**
     * @brief Read an unknown-typed message section from a dynamic buffer
     *
     * @param dbuf The buffer which will be consumed
     * @param a
     * @return any_section
     */
    static any_section read(dynamic_buffer_v1 auto&& dbuf, mlib::allocator<> a) {
        const_buffer_sequence auto data = dbuf.data();
        if (asio::buffer_size(data) < 1) {
            throw std::system_error(std::make_error_code(std::errc::protocol_error), "short read");
        }
        auto kind = static_cast<std::uint8_t>(*wire::unbounded_bytes_of_buffers(data).begin());
        dbuf.consume(1);
        data = dbuf.data();
        switch (kind) {
            // Regular body
        case 0: {
            if (asio::buffer_size(data) < 5) {  // min doc length
                throw std::system_error(std::make_error_code(std::errc::protocol_error),
                                        "short read");
            }
            const auto bson_len
                = mlib::read_int_le<std::uint32_t>(wire::unbounded_bytes_of_buffers(data)).value;
            if (asio::buffer_size(data) < bson_len) {
                throw std::system_error(std::make_error_code(std::errc::protocol_error),
                                        "short read");
            }
            bson::document ret{a};
            ret.resize_and_overwrite(bson_len, [&](bson_byte* out) {
                asio::buffer_copy(asio::buffer(out, bson_len), dbuf.data());
            });
            dbuf.consume(bson_len);
            return any_section(body_section(mlib_fwd(ret)));
        }
        default:
            throw std::system_error(std::make_error_code(std::errc::protocol_error),
                                    "unknown section kind");
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

    std::int32_t opcode() const noexcept;
    std::int32_t request_id() const noexcept { return _req_id; }
    std::int32_t response_to() const noexcept { return _resp_to; }

    vector<asio::const_buffer> buffers(mlib::allocator<> a) const;

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

}  // namespace amongoc::wire
