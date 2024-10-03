#pragma once

#include "./message.hpp"

#include <bson/doc.h>

#include <string_view>
#include <system_error>

namespace amongoc::wire {

class server_error : public std::system_error {
public:
    explicit server_error(bson::document body);

    bson::view body() const noexcept { return _body; }

private:
    bson::document _body;
};

[[noreturn]]
void throw_protocol_error(std::string_view msg);

void throw_if_section_error(body_section<bson::document> const& sec);
void throw_if_section_error(any_section const& sec);
void throw_if_message_error(any_op_msg_message const& op);
void throw_if_message_error(any_message const& msg);

}  // namespace amongoc::wire
