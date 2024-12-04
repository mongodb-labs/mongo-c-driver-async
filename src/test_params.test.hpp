#pragma once

#include <catch2/catch_test_macros.hpp>

#include <unistd.h>

#include <cstdlib>
#include <optional>
#include <random>
#include <string>

namespace amongoc::testing {

inline std::optional<std::string> default_from_env(const char* envvar) {
    auto e = std::getenv(envvar);
    if (not e) {
        return {};
    }
    return std::string(e);
}

struct parameters_type {
    std::optional<std::string> mongodb_uri = default_from_env("AMONGOC_TEST_MONGODB_URI");
    std::string                app_name;

    parameters_type() {
        // Generate a unique name for the client application to isolate failpoints
        app_name = "test-app-" + std::to_string(::getpid());
    }

    /**
     * @brief Check that a MongoDB URI has been specified for testing, or SKIP the current test
     */
    std::string require_uri() const {
        if (not mongodb_uri.has_value()) {
            SKIP("No MongoDB URI was set (pass --mongodb-uri or set $AMONGOC_TEST_MONGODB_URI)");
        }
        auto s = *mongodb_uri;
        if (s.contains("?")) {
            s.append("&appName=" + this->app_name);
        } else {
            s.append("?appName=" + this->app_name);
        }
        return s;
    }
};

inline parameters_type parameters;

}  // namespace amongoc::testing
