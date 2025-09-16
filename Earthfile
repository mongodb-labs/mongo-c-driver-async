VERSION 0.8

# Tweak the default container registry used for pulling system images.
ARG --global default_container_registry = "docker.io"

init:
    ARG --required env
    # Toggle the building of test programs
    ARG test = true
    # Toggle whether we use vcpkg to obtain dependencies
    ARG use_vcpkg = true
    FROM --pass-args $env
    DO --pass-args +INSTALL_DEPS

build:
    ARG warnings_as_errors = true
    ARG configs   = Debug;RelWithDebInfo
    ARG test      = true
    ARG use_vcpkg = true
    FROM --pass-args +init
    DO --pass-args +BUILD_INSTALL_EXPORT

test:
    FROM --pass-args +build
    RUN uv run --group=build \
        make ctest-run TEST_CONFIG=Debug JUNIT_OUTPUT=/results.xml
    SAVE ARTIFACT /results.xml

# Target used to install LLVM for a build. Not used outside this file
env.llvm:
    ARG --required llvm_major_version
    # LLVM doesn't provide a container, so we just use Ubuntu and the automated
    # LLVM installer script to get the appropriate major version
    FROM $default_container_registry/ubuntu:24.04
    DO +BASE
    # Required for the LLVM installer:
    RUN __install lsb-release software-properties-common gnupg
    # Install the major version using the automated LLVM installer:
    RUN curl -Ls https://apt.llvm.org/llvm.sh -o llvm.sh && \
        bash llvm.sh "$llvm_major_version"
    # Declare our preferred compiler version using CC and CXX env vars
    ENV CC=clang-$llvm_major_version
    ENV CXX=clang++-$llvm_major_version

run:
    LOCALLY
    ARG --required target
    BUILD --pass-args $target

# Miscellaneous system init
BASE:
    FUNCTION
    COPY --chmod=755 tools/__tool /usr/local/bin/__tool
    RUN __tool __init
    # Basic requirements:
    IF __can_install epel-release # test -f /etc/redhat-release && ! test -f /etc/fedora-release
        RUN __install epel-release
    END
    RUN (curl --version || __install curl)

    # Obtain uv
    ARG uv_version = "0.8.15"
    ARG uv_install_sh_url = "https://astral.sh/uv/$uv_version/install.sh"
    IF ! test -f /usr/local/bin/uv
        RUN curl -LsSf "$uv_install_sh_url" \
                | env UV_UNMANAGED_INSTALL=/opt/uv sh - \
            && ln -s /opt/uv/uv /usr/local/bin/uv \
            && uv --version
    END

BUILD_INSTALL_EXPORT:
    FUNCTION
    # Build and install
    DO --pass-args +BUILD --install_prefix=/opt/amongoc --cpack_out=/tmp/pkg
    # Export
    SAVE ARTIFACT /tmp/pkg/* /pkg/
    SAVE ARTIFACT /opt/amongoc/* /install/

# Install dependencies, possibly warming up the user-local vcpkg cache if vcpkg is used
INSTALL_DEPS:
    FUNCTION
    DO +BASE
    # Do we want to use vcpkg?
    ARG use_vcpkg=true
    # Are we installing test-only dependencies?
    ARG test=true

    IF __bool $test
        # We use Git to obtain certain test artifacts.
        RUN __install git
    END

    IF test -f /etc/alpine-release
        # Basic Alpine requirements:
        RUN __install build-base ccache
        IF __bool $use_vcpkg
            # Requirements for vcpkg to install our dependencies:
            RUN __install pkgconfig linux-headers perl bash tar zip unzip git
        ELSE
            # Our dependencies, obtained from the system package manager:
            RUN __install fmt-dev boost-dev openssl-dev
        END
    ELSE IF test -f /etc/debian_version
        RUN __install build-essential ccache
        IF __bool $use_vcpkg
            RUN __install zip unzip pkg-config git
        ELSE
            RUN __install libfmt-dev libssl-dev
            IF __can_install libboost-url-dev
                # Install the default version, if available
                RUN __install libboost-url-dev libboost-container-dev
            ELSE
                # Older debian requires qualified versions
                RUN __install libboost-url1.81-dev libboost-container1.81-dev
            END
        END
    ELSE IF test -f /etc/redhat-release
        RUN __install python3.12 ccache gcc gcc-c++
        # Specify a version of the GCC toolset to be installed, available in
        # RHEL-based systems ≤9.x
        ARG gts_version
        IF test "$gts_version" != ''
            RUN __install scl-utils gcc-toolset-$gts_version
            ENV LAUNCHER = "scl run gcc-toolset-$gts_version -- "
        ELSE
            RUN __install gcc gcc-c++
        END
        IF __bool $use_vcpkg
            RUN __install zip unzip perl git
        ELSE
            RUN __install boost-devel fmt-devel openssl-devel boost-url
        END
    END

    # Set the directory where Ccache writes its data, and cache that across runs
    ENV CCACHE_DIR = /run/ccache
    CACHE /run/ccache

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
            pmm(VCPKG REVISION 2025.08.27)
            " > $src_tmp/CMakeLists.txt
        # Running CMake now will prepare our dependencies without configuring the rest of the project
        CACHE ~/.cache/vcpkg
        RUN $LAUNCHER uv run --with=cmake~=3.20 --with=ninja cmake -G Ninja -S $src_tmp -B $src_tmp/_build/vcpkg-bootstrapping
    END

COPY_SRC:
    FUNCTION
    COPY --dir CMakeLists.txt vcpkg*.json etc/ src/ tools/ include/ etc/ \
            tests/ docs/ Makefile pyproject.toml uv.lock \
        .

BUILD:
    FUNCTION
    ARG install_prefix
    ARG cpack_out
    DO +COPY_SRC
    ARG --required test
    ARG --required warnings_as_errors
    ARG --required use_vcpkg
    ARG --required configs
    # Configure
    RUN $LAUNCHER uv run --group=build \
            make build \
                CONFIGS="$configs" \
                INSTALL_PREFIX=$install_prefix \
                USE_PMM=$(__boolstr $use_vcpkg) \
                WARNINGS_AS_ERRORS=$(__boolstr $warnings_as_errors) \
                BUILD_TESTING=$(__boolstr $test)
    IF test "$install_prefix" != ""
        FOR conf IN Debug # Release RelWithDebInfo
            RUN $LAUNCHER uv run --group=build \
                    make install-fast INSTALL_PREFIX=$install_prefix INSTALL_CONFIG=$conf
        END
    END
    IF test "$cpack_out" != ""
        RUN $LAUNCHER uv run --group=build \
                make package-fast \
                    CPACK_OUT="$cpack_out" \
                    PACKAGE_CONFIGS="$configs"
    END
