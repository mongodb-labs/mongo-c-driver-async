cmake_minimum_required(VERSION 3.12)
project(neo-fun VERSION "@__pkg_version@")

file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE tests CONFIGURE_DEPENDS src/*.test.cpp)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS src/*.hpp src/*.h)
list(REMOVE_ITEM sources ${tests})

add_library(neo-fun STATIC)
target_sources(neo-fun
    PRIVATE ${sources}
    PUBLIC FILE_SET HEADERS BASE_DIRS src/ FILES ${headers}
)
target_compile_features(neo-fun PUBLIC cxx_std_20)
set_target_properties(
    neo-fun PROPERTIES
    DEBUG_POSTFIX -dbg
    EXPORT_NAME neo::fun
)
install(
    TARGETS neo-fun
    EXPORT neo-fun-targets
    ARCHIVE DESTINATION lib
    FILE_SET HEADERS DESTINATION include
)
install(
    EXPORT neo-fun-targets
    DESTINATION lib/cmake
    FILE neo-funConfig.cmake
)
