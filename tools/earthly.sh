#!/usr/bin/env bash

set -euo pipefail

: "${EARTHLY_VERSION:=0.8.16}"

this_file=${BASH_SOURCE[0]}
this_dir=$(readlink -f "$(dirname "$this_file")")
repo_dir=$(dirname "$this_dir")
scratch_dir="$repo_dir/_build"
mkdir -p "$scratch_dir"

# Calc the arch of the executable we want
case "$HOSTTYPE" in
    x64|x86_64)
        arch=amd64
        ;;
    arm64)
        arch=arm64
        ;;
    *)
        echo "Unsupported architecture for automatic Earthly download: $HOSTTYPE" 1>&1
        exit 99
        ;;
esac

# The location where the Earthly executable will live
cache_dir="$scratch_dir/_earthly/$EARTHLY_VERSION"
mkdir -p "$cache_dir"

exe_filename="earthly-$(uname | tr '[:upper:]' '[:lower:]')-$arch"
EARTHLY_EXE="$cache_dir/$exe_filename"

# Download if it isn't already present
if ! test -f "$EARTHLY_EXE"; then
    echo "Downloading $exe_filename $EARTHLY_VERSION"
    url="https://github.com/earthly/earthly/releases/download/v$EARTHLY_VERSION/$exe_filename"
    curl --retry 5 -LsS --max-time 120 --fail "$url" --output "$EARTHLY_EXE"
    chmod a+x "$EARTHLY_EXE"
fi

"$EARTHLY_EXE" "$@"
