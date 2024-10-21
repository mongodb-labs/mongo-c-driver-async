#include "./message.hpp"

#include <amongoc/wire/error.hpp>

namespace amongoc::wire {

template struct body_section<bson_view>;
template class op_msg_message<std::array<bson_view_body_section, 1>>;

const bson::document& wire::any_message::expect_one_body_section_op_msg() const& noexcept {
    if (not std::holds_alternative<any_op_msg_message>(_content)) {
        throw_protocol_error("Expected a single OP_MSG message");
    }
    auto& content = std::get<any_op_msg_message>(_content);
    if (content.sections().size() != 1) {
        throw_protocol_error("Expected a single OP_MSG body section");
    }
    auto& section = content.sections().front();
    // XXX: When more section types are supported, the lambda expression below will
    // need to be changed to handle non-body sections (it should throw in those cases)
    return section.visit(
        [&](body_section<bson::document> const& sec) -> const bson::document& { return sec.body; });
}

vector<asio::const_buffer> any_message::buffers(allocator<> a) const {
    return visit_content(
        [&](const auto& c) -> const_buffer_sequence decltype(auto) { return c.buffers(a); });
}

vector<asio::const_buffer> any_section::buffers(allocator<> a) const {
    vector<asio::const_buffer> bufs{a};
    this->visit([&](auto&& sec) {
        auto k = asio::const_buffer(&sec.kind(), 1);
        bufs.push_back(k);
        // XXX: This is valid for now, as we only hold a section type that returns a single buffer.
        asio::const_buffer cb = sec.buffers(a);
        bufs.push_back(cb);
        // XXX: Future versions may need to do this with the returned buffer sequence:
        // bufs.insert(bufs.end(), asio::buffer_sequence_begin(bs), asio::buffer_sequence_end(bs));
    });
    return bufs;
}

std::int32_t any_message::opcode() const noexcept {
    return visit_content([&](const auto& c) { return c.opcode(); });
}
}  // namespace amongoc::wire
