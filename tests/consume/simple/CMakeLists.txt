cmake_minimum_required(VERSION 3.25)
project(TestUseAmongoc)

include(CTest)

if(TEST_IMPORT_MODE STREQUAL "add_subdirectory")
    add_subdirectory("${HOST_PROJECT_amongoc_SOURCE_DIR}" amongoc-build ${TEST_ADD_SUBDIR_EXCLUDE_FROM_ALL})
elseif(TEST_IMPORT_MODE STREQUAL "FetchContent")
    include(FetchContent)
    set(local_copy "${CMAKE_CURRENT_BINARY_DIR}/amongoc-src")
    # Create a pristine copy of the source tree. We don't want to copy a build directory,
    # as that might cause a recursive copy.
    file(
        # Trailing dot prevents creation of an intermediate directory:
        COPY "${HOST_PROJECT_amongoc_SOURCE_DIR}/."
        DESTINATION "${local_copy}"
        # Exclude all directories starting with an underscore, plus some other common
        # dirs we don't need
        REGEX "/_|/.git/|/build/|/cmake-build/" EXCLUDE
    )
    FetchContent_Declare(
        amongoc
        URL "${local_copy}"
    )
    message(STATUS "Importing amongoc using FetchContent …")
    FetchContent_MakeAvailable(amongoc)
elseif(TEST_IMPORT_MODE STREQUAL "FetchContent-Git")
    include(FetchContent)
    FetchContent_Declare(
        amongoc
        GIT_REPOSITORY "${HOST_PROJECT_amongoc_SOURCE_DIR}"
        GIT_TAG HEAD
    )
    message(STATUS "Importing amongoc using FetchContent with Git …")
    FetchContent_MakeAvailable(amongoc)
elseif(TEST_IMPORT_MODE STREQUAL "find_package")
    find_package(amongoc CONFIG REQUIRED)
endif()

add_executable(test-app test.c)
target_link_libraries(test-app amongoc::amongoc)

add_test(test-app test-app)
