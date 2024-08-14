cmake_minimum_required(VERSION 3.28)
project(amongoc VERSION 0.1.0 DESCRIPTION "An Asynchronous MongoDB Library for C")

# Enable testing with CTest
include(CTest)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/tools")

# Support modules
include(CMakePackageConfigHelpers)
# Developer aides
include(Sanitizers)
include(Ccache)
include(LLDLinker)

# Enable C++20 for our C++ code
set(CMAKE_CXX_STANDARD 20)

# Pull our deps with PMM
include(pmm)
pmm(BPT DEP_FILES bpt.deps.yaml)

# Add Catch integrations
include(Catch)

# If we generate any DLLs, use a .dll.lib suffix for the importlibs, to distinguish
# them from actual static libraries
set(CMAKE_IMPORT_LIBRARY_SUFFIX .dll.lib)

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
target_compile_features(amongoc PUBLIC cxx_std_23)

# Link deps and platform libs.
# TODO: Export these deps
target_link_libraries(amongoc PUBLIC neo-fun::neo-fun asio::asio amongoc::detail::c_platform)

# Add tests
add_executable(amongoc-test ${test_sources})
target_link_libraries(amongoc-test PRIVATE amongoc catch2::main)
target_include_directories(amongoc-test PRIVATE src)
catch_discover_tests(amongoc-test)

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

# TODO: These install paths aren't ideal. Get them sorted before a proper release.
# TODO: The neo-fun and Asio dependencies need to be propagated
if(WIN32)
    set(install_infix ${PROJECT_NAME}-${PROJECT_VERSION})
else()
    set(install_infix lib/${PROJECT_NAME}-${PROJECT_VERSION})
endif()
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/amongocConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/amongocConfig-version.cmake
    DESTINATION ${install_infix}/cmake
)
install(
    TARGETS amongoc
    EXPORT amongoc-targets
    RUNTIME DESTINATION ${install_infix}/bin/
    LIBRARY DESTINATION ${install_infix}/lib/
    ARCHIVE DESTINATION ${install_infix}/lib/
    FILE_SET HEADERS DESTINATION ${install_infix}/include/
    INCLUDES DESTINATION ${install_infix}/include/
)
install(
    EXPORT amongoc-targets
    DESTINATION ${install_infix}/cmake
    NAMESPACE amongoc::
)

include(CPack)

