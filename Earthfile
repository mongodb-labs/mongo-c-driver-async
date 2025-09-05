VERSION 0.8

# Tweak the default container registry used for pulling system images.
ARG --global default_container_registry = "docker.io"

build-gcc:
    ARG --required gcc_version
    # GCC provides a GCC container, based on Debian
    FROM $default_container_registry/gcc:$gcc_version
    DO --pass-args +BOOTSTRAP_BUILD_INSTALL_EXPORT

build-clang:
    ARG --required clang_version_major
    # LLVM doesn't provide a container, so we just use Ubuntu and the automated
    # LLVM installser script to get the appropriate major version
    FROM $default_container_registry/ubuntu:24.04
    DO +INIT
    # Required for the LLVM installer:
    RUN __install lsb-release software-properties-common gnupg
    # Install the major version using the automated LLVM installer:
    RUN curl -Ls https://apt.llvm.org/llvm.sh -o llvm.sh && \
        bash llvm.sh "$clang_version_major"
    # Declare our preferred compiler version using CC and CXX env vars
    ENV CC=clang-$clang_version_major
    ENV CXX=clang++-$clang_version_major
    DO --pass-args +BOOTSTRAP_BUILD_INSTALL_EXPORT

build-alpine:
    ARG alpine_version=3.20
    FROM $default_container_registry/alpine:$alpine_version
    DO --pass-args +BOOTSTRAP_BUILD_INSTALL_EXPORT

build-debian:
    ARG debian_version=12.11
    FROM $default_container_registry/debian:$debian_version
    DO --pass-args +BOOTSTRAP_BUILD_INSTALL_EXPORT

build-ubuntu:
    ARG ubuntu_version=24.04
    FROM $default_container_registry/ubuntu:$ubuntu_version
    DO --pass-args +BOOTSTRAP_BUILD_INSTALL_EXPORT

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
    COPY --chmod=755 tools/__tool /usr/local/bin/__tool
    RUN __tool __init
    # Basic requirements to even function:
    RUN __install lsb-release curl

    # Obtain uv
    ARG uv_version = "0.8.15"
    ARG uv_install_sh_url = "https://astral.sh/uv/$uv_version/install.sh"
    IF ! test -f /usr/local/bin/uv
        RUN curl -LsSf "$uv_install_sh_url" \
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
    # Do we want to use vcpkg?
    ARG use_vcpkg=true
    # Are we installing test-only dependencies?
    ARG test=true

    IF __bool $test
        # We use Git to obtain certain test artifacts.
        RUN __install git
    END

    IF __distro_is "Alpine-*"
        # Basic Alpine requirements:
        RUN __install build-base
        IF __bool $use_vcpkg
            # Requirements for vcpkg to install our dependencies:
            RUN __install pkgconfig linux-headers perl bash tar zip unzip git
        ELSE
            # Our dependencies, obtained from the system package manager:
            RUN __install fmt-dev boost-dev openssl-dev
        END
    ELSE IF __distro_is "Debian-*" "Ubuntu-*"
        RUN __install build-essential
        IF __bool $use_vcpkg
            RUN __install zip unzip pkg-config
        ELSE
            RUN __install libfmt-dev libssl-dev
            IF apt-cache show libboost-url-dev 2>&1 > /dev/null
                # Install the default version, if available
                RUN __install libboost-url-dev libboost-container-dev
            ELSE
                # Older debian requires qualified versions
                RUN __install libboost-url1.81-dev libboost-container1.81-dev
            END
        END
    END

    # Do some additional setup for vcpkg
    IF __bool $use_vcpkg
        # Required when bootstrapping vcpkg on Alpine:
        ENV VCPKG_FORCE_SYSTEM_BINARIES=1
        # Bootstrap dependencies, warming the user-local binary cache
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
