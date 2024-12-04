#pragma once

#include "./message.hpp"

#include <bson/doc.h>

#include <string_view>
#include <system_error>

namespace amongoc::wire {

/**
 * @brief An exception type based on `std::system_error` that carries the server message body
 */
class server_error : public std::system_error {
public:
    explicit server_error(bson::document body);

    bson::view body() const noexcept { return _body; }

private:
    bson::document _body;
};

/**
 * @brief Throws a std::system_error with `std::errc::protocol_error` as an error code
 *
 * @param msg The message string to be attached to the exception
 */
[[noreturn]]
void throw_protocol_error(std::string_view msg);

void throw_if_section_error(body_section<bson::document> const& sec);
void throw_if_section_error(any_section const& sec);
void throw_if_message_error(any_op_msg_message const& op);
void throw_if_message_error(any_message const& msg);

}  // namespace amongoc::wire
