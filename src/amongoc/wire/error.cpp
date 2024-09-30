#include <amongoc/bson/build.h>
#include <amongoc/status.h>
#include <amongoc/wire/error.hpp>
#include <amongoc/wire/message.hpp>
#include <amongoc/wire/proto.hpp>

#include <system_error>

void amongoc::wire::throw_protocol_error(std::string_view msg) {
    throw std::system_error(std::make_error_code(std::errc::protocol_error), std::string(msg));
}

void amongoc::wire::throw_if_section_error(amongoc::wire::body_section<bson::document> const& sec) {
    auto ok = sec.body.find("ok");
    if (ok == sec.body.end()) {
        // No "ok" field, no error to check
        return;
    }
    if (ok->as_bool()) {
        // The messaeg is okay
        return;
    }
    auto code = sec.body.find("code");
    auto ec   = code->as_int32();
    auto emsg = sec.body.find("errmsg")->utf8();
    throw std::system_error(std::error_code(ec, amongoc::server_category()), std::string(emsg));
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
