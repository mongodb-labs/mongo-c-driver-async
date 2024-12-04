#include <amongoc/status.h>
#include <amongoc/wire/error.hpp>
#include <amongoc/wire/message.hpp>
#include <amongoc/wire/proto.hpp>

#include <bson/doc.h>

#include <system_error>

amongoc::wire::server_error::server_error(bson::document msg)
    : system_error(std::error_code(msg.find("code")->value().as_int32(), server_category()),
                   std::string(std::string_view(msg.find("errmsg")->value().utf8)))
    , _body(mlib_fwd(msg)) {}

void amongoc::wire::throw_protocol_error(std::string_view msg) {
    throw std::system_error(std::make_error_code(std::errc::protocol_error), std::string(msg));
}

void amongoc::wire::throw_if_section_error(amongoc::wire::body_section<bson::document> const& sec) {
    auto ok = sec.body.find("ok");
    if (ok == sec.body.end()) {
        // No "ok" field, no error to check
        return;
    }
    if (ok->value().as_bool()) {
        // The message is okay
        return;
    }
    throw server_error(sec.body);
}

void amongoc::wire::throw_if_section_error(const any_section& sec) {
    sec.visit([](const auto& s) { throw_if_section_error(s); });
}

void amongoc::wire::throw_if_message_error(const any_op_msg_message& op_msg) {
    for (const auto& sec : op_msg.sections()) {
        throw_if_section_error(sec);
    }
}

void amongoc::wire::throw_if_message_error(const any_message& msg) {
    msg.visit_content([](const auto& n) { throw_if_message_error(n); });
}
