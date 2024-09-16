include(FetchContent)

FetchContent_Declare(
    mongodb-specifications
    GIT_REPOSITORY "https://github.com/mongodb/specifications"
    GIT_TAG 93dc9645c9d78838e12f103cc3ad2fef51fd5da9
)

FetchContent_MakeAvailable(mongodb-specifications)
set(Specifications_SOURCE_DIR "${mongodb-specifications_SOURCE_DIR}")
message(DEBUG "Specifications cloned: [${Specifications_SOURCE_DIR}]")
