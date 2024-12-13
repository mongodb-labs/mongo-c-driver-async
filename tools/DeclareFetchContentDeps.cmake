#[[

    This file announces some dependencies to FetchContent in a way that a
    subsequent `find_package` will invoke `FetchContent_MakeAvailable` to
    automatically download and include the packages as part of the build.

    This is only used for packages that do not need to be transitively imported
    into dependees. Other dependencies should be installed by the system
    packager.

    Inclusion of this file has no immediate effect. This will only be triggered
    by appropriate calls to `find_package`

]]
include_guard()
include(FetchContent)

FetchContent_Declare(
    neo-fun
    GIT_REPOSITORY https://github.com/vector-of-bool/neo-fun.git
    GIT_TAG 0.14.1
    OVERRIDE_FIND_PACKAGE
)
file(WRITE "${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/neo-funExtra.cmake" [=[
    cmake_path(SET __neo_fun_src_dir NORMALIZE "${neo-fun_SOURCE_DIR}/src")
    file(GLOB_RECURSE __nf_sources ${__neo_fun_src_dir}/*.c*)
    file(GLOB_RECURSE __nf_headers ${__neo_fun_src_dir}/*.h*)
    file(GLOB_RECURSE __nf_tests ${__neo_fun_src_dir}/*.test.c*)
    list(REMOVE_ITEM __nf_sources ~~~ ${__nf_tests})

    add_library(neo-fun STATIC EXCLUDE_FROM_ALL)
    add_library(neo::fun ALIAS neo-fun)
    target_sources(neo-fun
        PRIVATE ${__nf_sources}
        PUBLIC FILE_SET HEADERS BASE_DIRS ${__neo_fun_src_dir} FILES ${__nf_headers}
        )
    target_compile_features(neo-fun PUBLIC cxx_std_20)
]=])

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-32-0
    OVERRIDE_FIND_PACKAGE
)
file(WRITE "${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/asioExtra.cmake" [[
    set(__asio_src_dir "${asio_SOURCE_DIR}/asio/include")
    cmake_path(NORMAL_PATH __asio_src_dir)
    add_library(asio INTERFACE EXCLUDE_FROM_ALL)
    add_library(asio::asio ALIAS asio)
    # Add the header root as SYSTEM, to inhibit warnings on GNU compilers
    target_include_directories(asio SYSTEM INTERFACE ${__asio_src_dir})
]])

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    OVERRIDE_FIND_PACKAGE
)
