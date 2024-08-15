#include <amongoc/status.h>

#include <asio/error.hpp>
#include <netdb.h>

#include <cstring>
#include <system_error>

using namespace amongoc;

status status::from(const std::error_code& ec) noexcept {
    if (ec.category() == std::generic_category()) {
        return {&amongoc_generic_category, ec.value()};
    } else if (ec.category() == std::system_category()
               or ec.category() == asio::error::get_system_category()) {
        return {&amongoc_system_category, ec.value()};
    } else if (ec.category() == asio::error::get_netdb_category()) {
        return {&amongoc_netdb_category, ec.value()};
    } else if (ec.category() == asio::error::get_addrinfo_category()) {
        return {&amongoc_addrinfo_category, ec.value()};
    } else {
        return {&amongoc_unknown_category, ec.value()};
    }
}

std::string status::message() const noexcept {
    auto        s = amongoc_status_strdup_message(*this);
    std::string str(s);
    free(s);
    return str;
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

amongoc_status_category_vtable amongoc_generic_category = {
    .name            = [] { return "generic"; },
    .strdup_message  = [](int c) { return strdup(std::strerror(c)); },
    .is_cancellation = [](int c) { return c == ECANCELED; },
    .is_timeout      = [](int c) { return c == ETIMEDOUT || c == ETIME; },
};

// On POSIX, the system is the same as the generic category.
// TODO: On Windows, this will not be sufficient
amongoc_status_category_vtable amongoc_system_category = amongoc_generic_category;

amongoc_status_category_vtable amongoc_netdb_category = {
    .name = [] { return "amongoc.netdb"; },
    .strdup_message
    = [](int c) { return strdup(asio::error::get_netdb_category().message(c).data()); },
};

amongoc_status_category_vtable amongoc_addrinfo_category = {
    .name = [] { return "amongoc.addrinfo"; },
    .strdup_message
    = [](int c) { return strdup(asio::error::get_addrinfo_category().message(c).data()); },
};

amongoc_status_category_vtable amongoc_unknown_category = {
    .name           = [] { return "amongoc.unknown"; },
    .strdup_message = [](int c) { return strdup(("amongoc.unknown:" + std::to_string(c)).data()); },
};

std::error_code status::as_error_code() const noexcept {
    if (category == &amongoc_generic_category) {
        return std::error_code(code, std::generic_category());
    } else if (category == &amongoc_system_category) {
        return std::error_code(code, std::system_category());
    } else if (category == &amongoc_netdb_category) {
        return std::error_code(code, asio::error::get_netdb_category());
    } else if (category == &amongoc_addrinfo_category) {
        return std::error_code(code, asio::error::get_addrinfo_category());
    } else {
        return std::error_code(this->code, unknown_category_inst);
    }
}
