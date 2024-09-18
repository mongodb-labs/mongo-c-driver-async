cmake_minimum_required(VERSION 3.25)
project(amongoc VERSION 0.1.0 DESCRIPTION "An Asynchronous MongoDB Library for C")

# Enable testing with CTest
include(CTest)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/tools")
# Pull our deps with PMM+vcpkg
option(AMONGOC_USE_PMM "Enable PMM execution to bootstrap dependency packages automatically" ON)
if(AMONGOC_USE_PMM)
    include(pmm)
    pmm(VCPKG REVISION 2024.08.23)
endif()

find_package(asio CONFIG REQUIRED)
find_package(neo-fun CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED URL)
find_package(fmt CONFIG REQUIRED)

# Support modules
include(CMakePackageConfigHelpers)
# Developer aides
include(Sanitizers)
include(Ccache)
include(LLDLinker)

# When compiling C++ with GCC, set a deeper diagnostics depth
add_compile_options($<$<AND:$<CXX_COMPILER_ID:GNU>,$<COMPILE_LANGUAGE:CXX>>:-fconcepts-diagnostics-depth=4>)

# If we generate any DLLs, use a .dll.lib suffix for the importlibs, to distinguish
# them from actual static libraries
set(CMAKE_IMPORT_LIBRARY_SUFFIX .dll.lib)

# Debug binaries will have a `-dbg` suffix to allow multi-conf installation
set(CMAKE_DEBUG_POSTFIX -dbg)

# Collect compiled source files
file(
    GLOB_RECURSE sources
    CONFIGURE_DEPENDS
    src/*.cpp
    src/*.c
)
# Public headers files live in include/
file(
    GLOB_RECURSE pub_headers
    CONFIGURE_DEPENDS
    include/*.h
)
# Private header files live in src/
file(
    GLOB_RECURSE src_headers
    CONFIGURE_DEPENDS
    src/*.hpp
    src/*.h
)
# Test sources files have `.test` in their stem
file(
    GLOB_RECURSE test_sources
    CONFIGURE_DEPENDS
    src/*.test.cpp
)
# Remove the test sources from the list of library sources:
list(REMOVE_ITEM sources ${test_sources})
# Create the library from our gathered files
add_library(amongoc STATIC ${sources} ${src_headers} ${pub_headers})
add_library(amongoc::amongoc ALIAS amongoc)
# Attach headers. This also adds the include directories to the target
target_sources(amongoc
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include/
        FILES ${pub_headers}
    PRIVATE
        FILE_SET priv_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src/
        FILES ${src_headers}
)

# Enable C++23 features on the target
# We currently only use C++23 for `auto(). Future revisions
# can remove these requirements for greater portability.
target_compile_features(amongoc PUBLIC cxx_std_23)

# Link deps and platform libs.
target_link_libraries(amongoc PUBLIC
    neo::fun
    asio::asio
    fmt::fmt
    amongoc::detail::c_platform
    Boost::url
    )

if(BUILD_TESTING)
    # Add Catch integrations
    find_package(Catch2 CONFIG REQUIRED)
    include(Catch)

    # Add tests
    add_executable(amongoc-test ${test_sources})
    target_link_libraries(amongoc-test PRIVATE amongoc Catch2::Catch2WithMain)
    target_include_directories(amongoc-test PRIVATE src)
    catch_discover_tests(amongoc-test)

    # Generate URI tests
    include(GenURITests)
    target_sources(amongoc-test PRIVATE ${URITest_SOURCES})
endif()

# Compile and link all documentation examples
file(GLOB_RECURSE examples CONFIGURE_DEPENDS docs/*.example.c docs/*.example.cpp)

foreach(filepath IN LISTS examples)
    cmake_path(GET filepath STEM name)
    message(STATUS "Example: ${name}")
    set(exe "${name}.example")
    add_executable("${exe}" ${filepath})
    target_link_libraries(${exe} amongoc::amongoc)
endforeach()

# Generate the package-config files. Use camelCaseFileNames to enforce case sensitive find_package()
configure_package_config_file(
    etc/amongocConfig.cmake.in
    amongocConfig.cmake
    INSTALL_DESTINATION lib/cmake
)
write_basic_package_version_file(
    amongocConfig-version.cmake
    COMPATIBILITY SameMajorVersion
)

# We base our install paths on the GNU style
include(GNUInstallDirs)

set(AMONGOC_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake CACHE STRING "Installation directory where CMake package files will be written")
install(
    TARGETS amongoc
    EXPORT amongoc-targets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    FILE_SET HEADERS DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/amongocConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/amongocConfig-version.cmake
    DESTINATION "${AMONGOC_INSTALL_CMAKEDIR}"
)
install(
    EXPORT amongoc-targets
    DESTINATION "${AMONGOC_INSTALL_CMAKEDIR}"
    NAMESPACE amongoc::
)

string(TOLOWER "${CMAKE_SYSTEM_NAME}" CPACK_SYSTEM_NAME)
string(APPEND CPACK_SYSTEM_NAME "-${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)
