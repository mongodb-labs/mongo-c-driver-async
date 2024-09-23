#include "./proto.hpp"

#include <amongoc/bson/build.h>

#include <variant>

using namespace amongoc;

static_assert(wire::message_type<wire::any_op_msg_message>);
static_assert(wire::message_type<wire::any_message>);

template co_task<wire::any_message> wire::recv_message(allocator<>, tcp_connection_rw_stream&);

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
