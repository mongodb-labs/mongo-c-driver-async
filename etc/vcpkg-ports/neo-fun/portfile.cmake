vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH source_dir
    REPO vector-of-bool/neo-fun
    REF "${VERSION}"
    SHA512 a203fcc3e532c6ce6a4a179392d22c6b454d3164ba73cec04ab3f21ed4c6c74c32ae157a586afa1afff89a8964c35d7ae5515c9b7d15f6350cabf0d993bc5974
    HEAD_REF develop
)

set(__pkg_version "${VERSION}")

configure_file("${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" "${source_dir}" @ONLY)

vcpkg_cmake_configure(
    SOURCE_PATH "${source_dir}"
)
vcpkg_cmake_build()
vcpkg_cmake_install()
vcpkg_install_copyright(FILE_LIST LICENSE.txt)
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
