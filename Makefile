# *** Build Parameters ***
# Whether to use PMM for the build process
USE_PMM := true
# Whether to build tests
BUILD_TESTING := true
# Sanitizers to request (Comma-separated list)
SANITIZE :=
# The configurations to build (Semicolon-separated, or "all")
CONFIGS := Debug
# If running tests, the configuration to test
TEST_CONFIG := Debug
# Set the CMAKE_INSTALL_PREFIX and the `--prefix` arg for installs
INSTALL_PREFIX :=
# Treat compiler warnings as errors (Sets COMPILE_WARNING_AS_ERROR on amongoc)
WARNINGS_AS_ERRORS := false

# *** Execution Parameters ***
# Set the LAUNCHER parameter to prefix all executed commands
LAUNCHER :=

# Update the shell to be executed by the launching command. Make will use
# this to execute all commands in the Makefile:
SHELL := $(LAUNCHER) $(SHELL)

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

PYTHON_RUN :=
# Run CMake within the uv environment
CMAKE_RUN := cmake

SPHINX_JOBS ?= auto
SPHINX_ARGS := --jobs="$(SPHINX_JOBS)" --write-all --show-traceback --builder=dirhtml --fail-on-warning

DOCS_SRC := $(THIS_DIR)/docs
DOCS_OUT := $(BUILD_DIR)/docs/dev/html
docs-html:
	sphinx-build $(SPHINX_ARGS) $(DOCS_SRC) $(DOCS_OUT)

docs-serve:
	sphinx-autobuild $(SPHINX_ARGS) $(DOCS_SRC) $(DOCS_OUT)

configure:
	$(CMAKE_RUN) \
		-S "$(THIS_DIR)" \
		-B "$(BUILD_DIR)" \
		-D CMAKE_CROSS_CONFIGS="$(CONFIGS)" \
		-D CMAKE_DEFAULT_CONFIGS=all \
		-D AMONGOC_COMPILE_WARNING_AS_ERROR=$(WARNINGS_AS_ERRORS) \
		-D AMONGOC_USE_PMM=$(USE_PMM) \
		-D BUILD_TESTING=$(BUILD_TESTING) \
		-D MONGO_SANITIZE="$(SANITIZE)" \
		-D CMAKE_INSTALL_PREFIX=$(INSTALL_PREFIX) \
		-G "Ninja Multi-Config"

build: configure
	$(MAKE) build-fast

build-fast:
	$(CMAKE_RUN) --build "$(BUILD_DIR)"

test: build
	$(MAKE) test-fast

CTEST_ARGS := -C $(TEST_CONFIG) -j4 --output-on-failure --progress \
		-LE cmake\|uri/spec -E "URI/spec"
test-fast:
	$(CMAKE_RUN) -E chdir "$(BUILD_DIR)" ctest $(CTEST_ARGS)

JUNIT_OUTPUT := $(BUILD_DIR)/TestResults.xml
ctest-run:
	$(CMAKE_RUN) -E chdir "$(BUILD_DIR)" ctest $(CTEST_ARGS) \
		-T Start -T Test \
		--output-junit "$(JUNIT_OUTPUT)" \
	 	|| :
	uv tool run --isolated junit2html "$(JUNIT_OUTPUT)" "$(JUNIT_OUTPUT).html"

install: build
	$(MAKE) install-fast

INSTALL_CONFIG := Release
install-fast:
	$(CMAKE_RUN) --install "$(BUILD_DIR)" --config "$(INSTALL_CONFIG)" --prefix="$(INSTALL_PREFIX)"

package: build
	$(MAKE) package-fast

CPACK_OUT 		:= _cpack
PACKAGE_CONFIGS := Debug;Release;RelWithDebInfo
PACKAGE_FORMATS := STGZ;TGZ;ZIP
package-fast:
	$(CMAKE_RUN) -E chdir "$(BUILD_DIR)" \
		cpack -B "$(CPACK_OUT)" -C "$(PACKAGE_CONFIGS)" -G "$(PACKAGE_FORMATS)"
	rm -r -- "$(CPACK_OUT)/_CPack_Packages"

format-check:
	uv run --group=format --isolated $(PYTHON_RUN) tools/format.py --mode=check

format:
	uv run --group=format --isolated $(PYTHON_RUN) tools/format.py

packages:
	bash $(THIS_DIR)/tools/earthly.sh -a +build-multi/ _build/pkgs
