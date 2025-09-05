VERSION 0.8

# Tweak the default container registry used for pulling system images.
ARG --global default_container_registry = "docker.io"

build-gcc:
    ARG --required gcc_version
    FROM $default_container_registry/gcc:$gcc_version
    ARG warnings_as_errors=true
    DO --pass-args +BOOTSTRAP_BUILD_INSTALL_EXPORT \
        --warnings_as_errors=$warnings_as_errors \
        --build_deps "ccache" \
        --vcpkg_bs_deps "build-essential perl git pkg-config linux-libc-dev curl zip unzip"

build-clang:
    ARG --required clang_version_major
    FROM $default_container_registry/ubuntu:24.04
    DO +INIT
    # Required for the LLVM installer:
    RUN __install lsb-release software-properties-common gnupg
    RUN curl -Ls https://apt.llvm.org/llvm.sh -o llvm.sh && \
        bash llvm.sh "$clang_version_major"
    ENV CC=clang-$clang_version_major
    ENV CXX=clang++-$clang_version_major
    ARG warnings_as_errors=true
    DO --pass-args +BOOTSTRAP_BUILD_INSTALL_EXPORT \
        --warnings_as_errors=$warnings_as_errors \
        --build_deps "ccache" \
        --vcpkg_bs_deps "build-essential perl git pkg-config linux-libc-dev curl zip unzip"

build-alpine:
    ARG alpine_version=3.20
    FROM $default_container_registry/alpine:$alpine_version
    ARG warnings_as_errors=true
    DO --pass-args +BOOTSTRAP_BUILD_INSTALL_EXPORT \
        --build_deps "build-base git cmake gcc g++ ninja make ccache python3" \
        --vcpkg_bs_deps "pkgconfig linux-headers perl bash tar zip unzip curl" \
        --third_deps "fmt-dev boost-dev openssl-dev"

build-debian:
    FROM $default_container_registry/debian:12
    DO --pass-args +BOOTSTRAP_BUILD_INSTALL_EXPORT \
        --build_deps "build-essential cmake git ninja-build python3 ccache" \
        --vcpkg_bs_deps "perl pkg-config linux-libc-dev curl zip unzip" \
        --third_deps "libfmt-dev libboost-url1.81-dev libboost-container1.81-dev libssl-dev"

build-rl:
    FROM $default_container_registry/rockylinux:8
    RUN dnf -y install epel-release unzip
    LET cmake_url = "https://github.com/Kitware/CMake/releases/download/v3.30.3/cmake-3.30.3-linux-x86_64.sh"
    RUN curl "$cmake_url" -Lo cmake.sh && \
        sh cmake.sh --exclude-subdir --prefix=/usr/local/ --skip-license
    LET ninja_url = "https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-linux.zip"
    RUN curl -L "$ninja_url" -o ninja.zip && \
        unzip ninja.zip -d /usr/local/bin/
    CACHE ~/.ccache  # Epel Ccache still uses the old cache location
    DO --pass-args +BOOTSTRAP_BUILD_INSTALL_EXPORT \
        --launcher "scl run gcc-toolset-12 --" \
        --build_deps "gcc-toolset-12 python3.12 ccache" \
        --vcpkg_bs_deps "zip unzip git perl"

build-fedora:
    FROM $default_container_registry/fedora:41
    DO --pass-args +BOOTSTRAP_BUILD_INSTALL_EXPORT \
        --build_deps "cmake ninja-build git gcc gcc-c++ python3.12 ccache" \
        --vcpkg_bs_deps "zip unzip perl" \
        --third_deps "boost-devel boost-url fmt-devel openssl-devel"

build-multi:
    FROM $default_container_registry/alpine
    # COPY +build-rl/ out/rl/  ## XXX: Redhat build is broken: Investigate GCC linker issues
    COPY (+build-debian/ --use_vcpkg=false) out/debian/
    COPY (+build-alpine/ --use_vcpkg=false) out/alpine/
    COPY (+build-fedora/ --use_vcpkg=false) out/fedora/
    SAVE ARTIFACT out/* /

matrix:
    BUILD +run \
        --target +build-debian --target +build-alpine --target +build-fedora \
        --use_vcpkg=true --use_vcpkg=false \
        --test=false --test=true

run:
    LOCALLY
    ARG --required target
    BUILD --pass-args $target

# Miscellaneous system init
INIT:
    FUNCTION
    COPY --chmod=755 tools/__install tools/__bool tools/__boolstr /usr/local/bin/
    RUN __install curl
    ARG uv_version = "0.8.15"
    ARG uv_install_sh_url = "https://astral.sh/uv/$uv_version/install.sh"
    IF ! uv --version
        RUN (curl -LsSf "$uv_install_sh_url" || wget -qO- "$uv_install_sh_url") \
            | env UV_UNMANAGED_INSTALL=/opt/uv sh - \
            && ln -s /opt/uv/uv /usr/local/bin/uv \
            && uv --version
    END

BOOTSTRAP_BUILD_INSTALL_EXPORT:
    FUNCTION
    # Bootstrap
    DO --pass-args +BOOTSTRAP_DEPS
    # Build and install
    DO --pass-args +BUILD --install_prefix=/opt/amongoc --cpack_out=/tmp/pkg
    # Export
    SAVE ARTIFACT /tmp/pkg/* /pkg/
    SAVE ARTIFACT /opt/amongoc/* /install/

# Install dependencies, possibly warming up the user-local vcpkg cache if vcpkg is used
BOOTSTRAP_DEPS:
    FUNCTION
    DO +INIT
    # Dependencies that are required for the build. Always installed
    ARG build_deps
    RUN __install $build_deps
    # Switch behavior based on whether we use vcpkg
    ARG use_vcpkg=true
    IF ! __bool $use_vcpkg
        # No vcpkg. Install system dependencies
        ARG third_deps
        RUN __install $third_deps
        # Install system deps for testing, if needed
        ARG test_deps
        ARG test=true
        IF $test
            RUN __install $test_deps
        END
    ELSE
        # vcpkg may have dependencies that need to be installed to bootstrap
        ARG vcpkg_bs_deps
        RUN __install $vcpkg_bs_deps
        # Required when bootstrapping vcpkg on Alpine:
        ENV VCPKG_FORCE_SYSTEM_BINARIES=1
        # Bootstrap dependencies
        LET src_tmp=/s-tmp
        WORKDIR $src_tmp
        COPY --dir vcpkg*.json $src_tmp
        COPY tools/pmm.cmake $src_tmp/tools/
        RUN printf %s "cmake_minimum_required(VERSION 3.20)
            project(tmp)
            include(tools/pmm.cmake)
            pmm(VCPKG REVISION 2024.08.23)
            " > $src_tmp/CMakeLists.txt
        # Running CMake now will prepare our dependencies without configuring the rest of the project
        CACHE ~/.cache/vcpkg
        ARG launcher
        RUN $launcher uv run --with=cmake~=3.20 --with=ninja cmake -G Ninja -S $src_tmp -B $src_tmp/_build/vcpkg-bootstrapping
    END

COPY_SRC:
    FUNCTION
    COPY --dir CMakeLists.txt vcpkg*.json etc/ src/ tools/ include/ etc/ \
            tests/ Makefile pyproject.toml uv.lock \
        .

BUILD:
    FUNCTION
    ARG install_prefix
    ARG cpack_out
    ARG launcher
    DO +COPY_SRC
    CACHE ~/.cache/ccache
    # Toggle testing
    ARG test=true
    # Enable -Werror
    ARG warnings_as_errors=false
    # Toggle PMM in the build
    ARG use_vcpkg=true
    # The configurations to build (semicolon-separated list)
    ARG configs=Debug
    # Configure
    RUN $launcher make test \
            LAUNCHER='uv run --group=build' \
            CONFIGS="$configs" \
            TEST_CONFIG="Debug" \
            INSTALL_PREFIX=$install_prefix \
            USE_PMM=$(__boolstr $use_vcpkg) \
            WARNINGS_AS_ERRORS=$(__boolstr $warnings_as_errors) \
            BUILD_TESTING=$(__boolstr $test)
    IF test "$install_prefix" != ""
        FOR conf IN Debug # Release RelWithDebInfo
            RUN $launcher make install-fast LAUNCHER='uv run --group=build' INSTALL_PREFIX=$install_prefix INSTALL_CONFIG=$conf
        END
    END
    IF test "$cpack_out" != ""
        RUN $launcher make package-fast LAUNCHER='uv run --group=build' \
            CPACK_OUT="$cpack_out" \
            PACKAGE_CONFIGS=Debug
    END
