cmake_minimum_required(VERSION 3.25)
project(amongoc VERSION 0.1.0 DESCRIPTION "An Asynchronous MongoDB Library for C")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/tools")

# Pull our deps with PMM+vcpkg
option(AMONGOC_USE_PMM "Enable PMM execution to bootstrap dependency packages automatically" ${PROJECT_IS_TOP_LEVEL})
if(AMONGOC_USE_PMM)
    include(pmm)
    pmm(VCPKG REVISION 2024.08.23)
endif()

# Set up `find_package()` to trigger FetchContent for certain deps
include(DeclareFetchContentDeps)

find_package(asio 1.25.0 REQUIRED)
find_package(Boost CONFIG REQUIRED URL Container)
find_package(fmt CONFIG REQUIRED)
find_package(neo-fun REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Enable testing with CTest
# Note: We respect the BUILD_TESTING option from CTest.cmake
include(CTest)

if(BUILD_TESTING)
    find_package(Catch2 CONFIG REQUIRED)
endif()

# Support modules
include(CMakePackageConfigHelpers)

# Platform bits
include(MongoSettings)
include(MongoPlatform)
# Developer aides
include(Sanitizers)
include(Ccache)
include(LLDLinker)
include(MongoWarnings)

if(PROJECT_IS_TOP_LEVEL)
    # Set compile warnings to be errors
    if(MONGODB_DEVELOPER)
        set(CMAKE_COMPILE_WARNING_AS_ERROR TRUE)
    endif()
    mongo_add_warning_options(
        gnu-like:-Wall
        # False-positive in GCC with coroutines and a custom allocator:
        gnu-like:lang-cxx:-Wno-mismatched-new-delete
        gnu-like:-Wextra
        # gnu-like:-Wconversion
        gnu-like:-Wsign-conversion
        gnu-like:-Wsign-compare
    )
endif()

# Tweak some options if we are added as a subdirectory project
if(NOT PROJECT_IS_TOP_LEVEL)
    # We aren't the top-level project. Don't define test cases
    set(BUILD_TESTING FALSE)
endif()

# When compiling C++ with GCC, set a deeper diagnostics depth
add_compile_options($<$<AND:$<CXX_COMPILER_ID:GNU>,$<COMPILE_LANGUAGE:CXX>>:-fconcepts-diagnostics-depth=4>)

# If we generate any DLLs, use a .dll.lib suffix for the importlibs, to distinguish
# them from actual static libraries
set(CMAKE_IMPORT_LIBRARY_SUFFIX .dll.lib)

# Debug binaries will have a `-dbg` suffix to allow multi-conf installation
set(CMAKE_DEBUG_POSTFIX -dbg)
set(CMAKE_RELWITHDEBINFO_POSTFIX -reldbg)
set(CMAKE_MINSIZEREL_POSTFIX -minsize)

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
    src/*.test.c
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

# Prevent any code from emitting a `neo_assert` that would require downstream linking
target_compile_definitions(amongoc PRIVATE NEO_ENABLE_CHECKS=0)

# Enable C++23 features on the target
# We currently only use C++23 for `auto(). Future revisions
# can remove these requirements for greater portability.
target_compile_features(amongoc PRIVATE cxx_std_23)

# Link deps and platform libs.
target_link_libraries(amongoc
    PUBLIC
        amongoc::detail::c_platform
        Threads::Threads
    PRIVATE
        fmt::fmt
        Boost::container
        Boost::url
        $<BUILD_INTERFACE:asio::asio>
        $<BUILD_INTERFACE:neo::fun>
        OpenSSL::SSL
    )
if(BUILD_TESTING)
    # Add Catch integrations
    include(Catch)

    # Add tests
    add_executable(amongoc-test ${test_sources})
    target_link_libraries(amongoc-test PRIVATE
        amongoc::amongoc
        Catch2::Catch2
        $<BUILD_INTERFACE:asio::asio>
        $<BUILD_INTERFACE:neo::fun>
        Boost::container
        )
    target_include_directories(amongoc-test PRIVATE src)
    catch_discover_tests(amongoc-test DISCOVERY_MODE PRE_TEST
        PROPERTIES SKIP_REGULAR_EXPRESSION ":[0-9]+: SKIPPED:"
        )
    target_compile_features(amongoc-test PRIVATE cxx_std_23)

    # A test case that simply invokes each BSON API
    add_executable(bson-use-test tests/use-bson.test.c)
    target_link_libraries(bson-use-test PRIVATE amongoc::amongoc)

    # Add a test that compiles as C++17 and includes all of our headers
    add_executable(cxx17-test tests/cxx17.test.cpp)
    target_link_libraries(cxx17-test PRIVATE amongoc::amongoc)
    set_property(TARGET cxx17-test PROPERTY CXX_STANDARD 17)

    # Generate URI tests
    include(GenURITests)
    target_sources(amongoc-test PRIVATE ${URITest_SOURCES})

    # Add compile-time tests for generic function signatures
    target_sources(amongoc-test PRIVATE
        tests/sigcheck.test.cpp
        tests/sigcheck.test.c
        tests/sigcheck.test.h
    )

    # Don't add the testproject tests if we are already building as a test project
    if(NOT DEFINED HOST_PROJECT_CMAKE_SOURCE_DIR)
        include(TestProject)
        # Pass through some variables that point to our dependency packages. The
        # tests will use these same packages when importing amongoc.
        # TODO: Add test cases that don't share dependencies with the parent build
        set(passthru_args
            PASSTHRU_VARS fmt_DIR
            # Boost has a lot of variables that need to be forwarded:
            PASSTHRU_VARS_REGEX "[bB]oost.*(_DIR|_PATH)"
        )
        add_test_cmake_project(
            "CMake/Consume/Import with find_package" ./tests/consume/simple
            INSTALL_PARENT
            SETTINGS
                TEST_IMPORT_MODE=find_package
            # Debug issues with find_package():
            CONFIGURE_ARGS --debug-find
            ${passthru_args}
            )
        add_test_cmake_project(
            "CMake/Consume/Import with FetchContent" ./tests/consume/simple
            SETTINGS
                TEST_IMPORT_MODE=FetchContent
            ${passthru_args}
            )
        add_test_cmake_project(
            "CMake/Consume/Import with FetchContent (Git)" ./tests/consume/simple
            SETTINGS
                TEST_IMPORT_MODE=FetchContent-Git
            ${passthru_args}
            )
        add_test_cmake_project(
            "CMake/Consume/Import with add_subdirectory" ./tests/consume/simple
            SETTINGS
                TEST_IMPORT_MODE=add_subdirectory
            ${passthru_args}
            )
        add_test_cmake_project(
            "CMake/Consume/Import with add_subdirectory - EXCLUDE_FROM_ALL" ./tests/consume/simple
            SETTINGS
                TEST_IMPORT_MODE=add_subdirectory
                TEST_ADD_SUBDIR_EXCLUDE_FROM_ALL=EXCLUDE_FROM_ALL
            ${passthru_args}
            )
    endif()
endif()

# Compile and link all documentation examples
file(GLOB_RECURSE examples CONFIGURE_DEPENDS docs/*.example.c docs/*.example.cpp)

foreach(filepath IN LISTS examples)
    cmake_path(GET filepath STEM name)
    message(STATUS "Example: ${name}")
    set(exe "${name}.example")
    add_executable("${exe}" ${filepath})
    target_compile_features(${exe} PRIVATE cxx_std_23)
    target_link_libraries(${exe} amongoc::amongoc)
endforeach()

if(PROJECT_IS_TOP_LEVEL OR AMONGOC_GENERATE_INSTALL_RULES)
    # We base our install paths on the GNU style
    include(GNUInstallDirs)

    set(AMONGOC_INSTALL_CMAKEDIR
        ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}-${PROJECT_VERSION}
        CACHE STRING "Installation directory where CMake package files will be written"
    )

    # Generate the package-config files. Use camelCaseFileNames to enforce case sensitive find_package()
    configure_package_config_file(
        etc/amongocConfig.cmake.in
        amongocConfig.cmake
        INSTALL_DESTINATION "${AMONGOC_INSTALL_CMAKEDIR}"
    )
    write_basic_package_version_file(
        amongocConfig-version.cmake
        COMPATIBILITY SameMajorVersion
    )

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
endif()