
#include <amongoc/wire/error.hpp>

#include <system_error>

void amongoc::wire::throw_protocol_error(const char* msg) {
    throw std::system_error(std::make_error_code(std::errc::protocol_error), msg);
}
