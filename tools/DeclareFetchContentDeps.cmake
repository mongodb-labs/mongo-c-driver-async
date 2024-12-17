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
    URL https://github.com/vector-of-bool/neo-fun/archive/refs/tags/0.14.1.tar.gz
    URL_HASH SHA256=bff0fbc5244f8e0148f41e9322206e1af217a819b9dae487c691db04e38aac32
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
    URL https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-32-0.tar.gz
    URL_HASH SHA256=f1b94b80eeb00bb63a3c8cef5047d4e409df4d8a3fe502305976965827d95672
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
    URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.7.1.tar.gz
    URL_HASH SHA256=c991b247a1a0d7bb9c39aa35faf0fe9e19764213f28ffba3109388e62ee0269c
    OVERRIDE_FIND_PACKAGE
)
