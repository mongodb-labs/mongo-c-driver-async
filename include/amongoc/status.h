#pragma once

#include "./config.h"

#ifdef __cplusplus
#include <system_error>
#endif

typedef struct amongoc_status amongoc_status;

AMONGOC_EXTERN_C_BEGIN

/**
 * @brief Obtain a human-readable message describing the given status
 *
 * @return const char* A static null-terminated C string describing the status
 */
extern const char* amongoc_status_message(amongoc_status);

AMONGOC_EXTERN_C_END

/**
 * @brief Status categories for amongoc_status
 *
 */
enum amongoc_status_category {
    // Generic POSIX/<errno.h> values
    amongoc_status_category_generic = 0,
    // Platform-specific errors
    amongoc_status_category_system = 1,
    // Errors from DNS
    amongoc_status_category_netdb = 2,
    // Errors for network address information
    amongoc_status_category_addrinfo = 3,
    // Unknown error categories
    amongoc_status_category_unknown = 99999,
};

struct amongoc_status {
    // The category of the error. One of `amongoc_status_category`
    int category;
    // The error code integer value
    int code;

#ifdef __cplusplus
    amongoc_status()
        : category(0)
        , code(0) {}

    amongoc_status(amongoc_status_category cat, int code)
        : category(cat)
        , code(code) {}

    // Allow conversion from literal zero
    amongoc_status(decltype(nullptr))
        : amongoc_status() {}

    // Construct an amongoc_status from a std::error_code
    static amongoc_status from(std::error_code const&) noexcept;
    // Convert from an amongoc_status to a std::error_code
    std::error_code as_error_code() const noexcept;

    // Obtain the message string associated with this status
    const char* message() const noexcept { return ::amongoc_status_message(*this); }
#endif
};

#define amongoc_okay (AMONGOC_INIT(amongoc_status){amongoc_status_category_generic, 0})

#ifdef __cplusplus
namespace amongoc {

using status = ::amongoc_status;

}  // namespace amongoc
#endif
