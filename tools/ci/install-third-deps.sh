#!/usr/bin/env sh

# This script is used in CI processes to install the build-time-dependencies
# for the system package manager, not using vcpkg. This relies on the __install
# script being present.

set -eu

if test -f /etc/debian_version; then
    __install libfmt-dev libboost-url-dev libboost-container-dev libssl-dev \
    || __install libfmt-dev libboost-url1.81-dev libboost-container1.81-dev libssl-dev
fi

if test -f /etc/alpine-release; then
    __install fmt-dev boost-dev openssl-dev
fi

if test -f /etc/redhat-release; then
    __install boost-devel boost-url fmt-devel openssl-devel
fi
