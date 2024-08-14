#include <amongoc/status.h>

#include <asio/error.hpp>
#include <netdb.h>

#include <cstring>
#include <system_error>

using namespace amongoc;

const char* amongoc_status_message(status st) {
    auto cat = (amongoc_status_category)st.category;
    switch (cat) {
    case amongoc_status_category_generic:
        return std::strerror(st.code);
        // return std::generic_category().message(st.code);
    case amongoc_status_category_system:
        // XXX: On Windows, this is not sufficient for correct errors. Use FormatMessage()
        return std::strerror(st.code);
    case amongoc_status_category_netdb:
    case amongoc_status_category_addrinfo:
        return ::gai_strerror(st.code);
    default:
    case amongoc_status_category_unknown:
        return "(Unknown status category)";
    }
}

status status::from(const std::error_code& ec) noexcept {
    if (ec.category() == std::generic_category()) {
        return {amongoc_status_category_generic, ec.value()};
    } else if (ec.category() == std::system_category()
               or ec.category() == asio::error::get_system_category()) {
        return {amongoc_status_category_system, ec.value()};
    } else if (ec.category() == asio::error::get_netdb_category()) {
        return {amongoc_status_category_netdb, ec.value()};
    } else if (ec.category() == asio::error::get_addrinfo_category()) {
        return {amongoc_status_category_addrinfo, ec.value()};
    } else {
        return {amongoc_status_category_unknown, ec.value()};
    }
}

namespace {

class unknown_error_category : public std::error_category {
    const char* name() const noexcept override { return "amongoc.unknown"; }
    std::string message(int ec) const noexcept override {
        return "amongoc.unknown:" + std::to_string(ec);
    }
};

unknown_error_category unknown_category_inst;

}  // namespace

std::error_code status::as_error_code() const noexcept {
    auto cat = static_cast<amongoc_status_category>(this->category);
    switch (cat) {
    case amongoc_status_category_generic:
        return std::error_code(this->code, std::generic_category());
    case amongoc_status_category_system:
        return std::error_code(this->code, std::system_category());
    case amongoc_status_category_netdb:
        return std::error_code(this->code, asio::error::get_netdb_category());
    case amongoc_status_category_addrinfo:
        return std::error_code(this->code, asio::error::get_addrinfo_category());
    default:
    case amongoc_status_category_unknown:
        return std::error_code(this->code, unknown_category_inst);
    }
}
