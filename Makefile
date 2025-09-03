.SILENT:
.PHONY: docs-html docs-serve default build test format format-check packages

# If given no other target, runs the build
default: build

# The absolute path that refers to this Makefile
THIS_FILE := $(realpath $(lastword $(MAKEFILE_LIST)))
# The directory that contains this Makefile (the repository root directory)
THIS_DIR := $(shell dirname $(THIS_FILE))

# Directory where we will scribble build files
BUILD_DIR ?= $(THIS_DIR)/_build/auto

# uv commands used in this file
UV_RUN     := uv run
DOCS_RUN   := $(UV_RUN) --isolated --group=docs
FORMAT_RUN := $(UV_RUN) --isolated --group=format
# Build is not isolated, because CMake caches paths to certain files
BUILD_RUN  := $(UV_RUN) --group=build
# Run CMake within the uv environment
CMAKE_RUN := $(BUILD_RUN) cmake

SPHINX_JOBS ?= auto
SPHINX_ARGS := -W -j "$(SPHINX_JOBS)" -aT -b dirhtml

DOCS_SRC := $(THIS_DIR)/docs
DOCS_OUT := $(BUILD_DIR)/docs/dev/html
docs-html:
	$(DOCS_RUN) sphinx-build $(SPHINX_ARGS) $(DOCS_SRC) $(DOCS_OUT)

docs-serve:
	$(DOCS_RUN) sphinx-autobuild $(SPHINX_ARGS) $(DOCS_SRC) $(DOCS_OUT)

build:
	$(CMAKE_RUN) \
		-S "$(THIS_DIR)" \
		-B "$(BUILD_DIR)" \
		--fresh \
		-D CMAKE_CROSS_CONFIGS="Debug" \
		-D CMAKE_DEFAULT_CONFIGS=all \
		-G "Ninja Multi-Config"
	$(CMAKE_RUN) --build "$(BUILD_DIR)"

test: build
	$(CMAKE_RUN) -E chdir "$(BUILD_DIR)" \
		ctest -C Debug -j4 --output-on-failure

format-check:
	$(UV_RUN) --group format tools/format.py --mode=check

format:
	$(UV_RUN) --group format tools/format.py

packages:
	bash $(THIS_DIR)/tools/earthly.sh -a +build-multi/ _build/pkgs
