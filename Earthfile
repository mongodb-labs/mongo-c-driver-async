VERSION 0.8

build-alpine:
    FROM alpine:3.20
    # Install build deps. Some deps are for vcpkg bootstrapping
    RUN apk add build-base git cmake gcc g++ ninja make curl zip unzip tar pkgconfig linux-headers ccache
    DO +BOOTSTRAP_BUILD_INSTALL_EXPORT

build-debian:
    FROM debian:bookworm
    RUN apt-get update && \
        apt-get -y install build-essential cmake git curl zip unzip tar ninja-build pkg-config
    DO +BOOTSTRAP_BUILD_INSTALL_EXPORT

build-rl:
    FROM rockylinux:9
    RUN dnf -y install zip unzip git gcc-toolset-12
    LET cmake_url = "https://github.com/Kitware/CMake/releases/download/v3.30.3/cmake-3.30.3-linux-x86_64.sh"
    RUN curl "$cmake_url" -Lo cmake.sh && \
        sh cmake.sh --exclude-subdir --prefix=/usr/local/ --skip-license
    LET ninja_url = "https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-linux.zip"
    RUN curl -L "$ninja_url" -o ninja.zip && \
        unzip ninja.zip -d /usr/local/bin/
    DO +BOOTSTRAP_BUILD_INSTALL_EXPORT --launcher "scl run gcc-toolset-12 --"

build-multi:
    FROM alpine
    COPY +build-rl/ out/rl/
    COPY +build-debian/ out/debian/
    COPY +build-alpine/ out/alpine/
    SAVE ARTIFACT out/* /

BOOTSTRAP_BUILD_INSTALL_EXPORT:
    FUNCTION
    # Bootstrap
    DO --pass-args +BOOTSTRAP_DEPS
    # Build and install
    DO --pass-args +BUILD --install_prefix=/opt/amongoc --cpack_out=/tmp/pkg
    # Export
    SAVE ARTIFACT /tmp/pkg/* /pkg/
    SAVE ARTIFACT /opt/amongoc/* /install/

# Warms-up the user-local vcpkg cache of packages based on the packages we install for our build
BOOTSTRAP_DEPS:
    FUNCTION
    ARG launcher
    # Required when bootstrapping vcpkg on Alpine:
    ENV VCPKG_FORCE_SYSTEM_BINARIES=1
    # Bootstrap dependencies
    LET src_tmp=/s-tmp
    WORKDIR $src_tmp
    COPY --dir vcpkg*.json $src_tmp
    COPY tools/pmm.cmake $src_tmp/tools/
    COPY --dir etc/vcpkg-ports $src_tmp/etc/
    RUN printf %s "cmake_minimum_required(VERSION 3.20)
        project(tmp)
        include(tools/pmm.cmake)
        pmm(VCPKG REVISION 2024.08.23)
        " > $src_tmp/CMakeLists.txt
    # Running CMake now will prepare our dependencies without configuring the rest of the project
    RUN $launcher cmake -S $src_tmp -B $src_tmp/_build/vcpkg-bootstrapping

COPY_SRC:
    FUNCTION
    COPY --dir CMakeLists.txt vcpkg*.json etc/ src/ tools/ include/ etc/ .

BUILD:
    FUNCTION
    ARG install_prefix
    ARG cpack_out
    ARG launcher
    DO +COPY_SRC
    CACHE ~/.cache
    CACHE ~/.ccache
    RUN $launcher cmake -S . -B _build -G "Ninja Multi-Config" \
        -D CMAKE_CROSS_CONFIGS="Debug;Release" \
        -D CMAKE_INSTALL_PREFIX=$prefix \
        -D CMAKE_DEFAULT_CONFIGS=all
    RUN $launcher cmake --build _build
    IF test "$install_prefix" != ""
        RUN cmake --install _build --prefix="$install_prefix" --config Debug
        RUN cmake --install _build --prefix="$install_prefix" --config Release
    END
    IF test "$cpack_out" != ""
        RUN cmake -E chdir _build \
            cpack -B "$cpack_out" -C "Debug;Release" -G "STGZ;TGZ;ZIP" && \
            rm "$cpack_out/_CPack_Packages" -rf
    END
