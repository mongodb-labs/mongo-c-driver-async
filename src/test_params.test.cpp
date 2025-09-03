#include "./test_params.test.hpp"

#ifndef _WIN32
#include <unistd.h>  // getpid()
#else
#include <windows.h>
#endif

amongoc::testing::parameters_type::parameters_type() {
// Generate a unique name for the client application to isolate failpoints
#ifndef _WIN32
    app_name = "test-app-" + std::to_string(::getpid());
#else
    app_name = "test-app-" + std::to_string(::GetCurrentProcessId());
#endif
}
