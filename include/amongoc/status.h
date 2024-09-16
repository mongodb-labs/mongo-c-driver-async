#pragma once

#include <mlib/config.h>

#if mlib_is_cxx()
#include <string>
#include <system_error>
#endif

typedef struct amongoc_status amongoc_status;

mlib_extern_c_begin();

/**
 * @brief Virtual table for customizing the behavior of `amongoc_status`
 *
 */
struct amongoc_status_category_vtable {
    // Get the name of the category
    const char* (*name)(void);
    // Dynamically allocate a new string for the message contained in the status
    char* (*strdup_message)(int code);
    // Test whether a particular integer value is an error
    bool (*is_error)(int code);
    // Test whether a particular integer value represents cancellation
    bool (*is_cancellation)(int code);
    // Test whether a particular integer value represents timeout
    bool (*is_timeout)(int code);
};

mlib_extern_c_end();

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

extern const struct amongoc_status_category_vtable amongoc_generic_category;
extern const struct amongoc_status_category_vtable amongoc_system_category;
extern const struct amongoc_status_category_vtable amongoc_netdb_category;
extern const struct amongoc_status_category_vtable amongoc_addrinfo_category;
extern const struct amongoc_status_category_vtable amongoc_unknown_category;

struct amongoc_status {
    // The category of the error. One of `amongoc_status_category`
    struct amongoc_status_category_vtable const* category;
    // The error code integer value
    int code;

#if mlib_is_cxx()
    amongoc_status() noexcept
        : category(&amongoc_generic_category)
        , code(0) {}

    amongoc_status(struct amongoc_status_category_vtable const* cat, int code) noexcept
        : category(cat)
        , code(code) {}

    // Allow conversion from literal zero
    amongoc_status(decltype(nullptr)) noexcept
        : amongoc_status() {}

    // Construct an amongoc_status from a std::error_code
    static amongoc_status from(std::error_code const&) noexcept;
    static amongoc_status from(std::errc ec) noexcept { return from(std::make_error_code(ec)); }

    /**
     * @brief Convert a status to a `std::error_code`
     *
     * @warning THIS IS LOSSY! The status may have a custom error category that
     * cannot be mapped automatically to a C++ error category. This method should
     * only be used on status values that have a category beloning to amongoc
     */
    std::error_code as_error_code() const noexcept;

    // Obtain the message string associated with this status
    std::string message() const noexcept;

    // Return `true` if the status represents an error
    inline bool is_error() const noexcept;

    bool operator==(amongoc_status const&) const = default;
#endif
};

/**
 * @brief Test whether the given status code represents an error condition.
 */
static inline bool amongoc_is_error(amongoc_status st) mlib_noexcept {
    // If the category defines a way to check for errors, ask the category
    if (st.category->is_error) {
        return st.category->is_error(st.code);
    }
    // The category says nothing about what is an error, so consider any non-zero value to be an
    // error
    return st.code != 0;
}

/**
 * @brief Return `true` if the given status represents a cancellation
 */
static inline bool amongoc_is_cancellation(amongoc_status st) mlib_noexcept {
    return st.category->is_cancellation && st.category->is_cancellation(st.code);
}

/**
 * @brief Return `true` if the given status represents an operational time-out
 */
static inline bool amongoc_is_timeout(amongoc_status st) mlib_noexcept {
    return st.category->is_timeout && st.category->is_timeout(st.code);
}

/**
 * @brief Obtain a human-readable message describing the given status
 *
 * @return char* A dynamically allocated null-terminated C string describing the status.
 * @note The returned string must be freed with free()!
 */
static inline char* amongoc_status_strdup_message(amongoc_status s) {
    return s.category->strdup_message(s.code);
}

#define amongoc_okay (mlib_init(amongoc_status){&amongoc_generic_category, 0})

#if mlib_is_cxx()
namespace amongoc {

using status = ::amongoc_status;

/**
 * @brief A basic exception type that carries an `amongoc_status` value
 */
class exception : std::runtime_error {
public:
    explicit exception(amongoc_status s) noexcept
        : runtime_error(s.message())
        , _status(s) {}

    // Get the status associated with this exception
    amongoc_status status() const noexcept { return _status; }

private:
    amongoc_status _status;
};

}  // namespace amongoc

bool amongoc_status::is_error() const noexcept { return amongoc_is_error(*this); }

#endif
