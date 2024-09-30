#pragma once

#include "./message.hpp"

#include <bson/build.h>

#include <string_view>

namespace amongoc::wire {

[[noreturn]]
void throw_protocol_error(std::string_view msg);

void throw_if_section_error(body_section<bson::document> const& sec);
void throw_if_section_error(any_section const& sec);
void throw_if_message_error(any_op_msg_message const& op);
void throw_if_message_error(any_message const& msg);

}  // namespace amongoc::wire
