.SILENT:

.PHONY: poetry-install docs-html docs-serve default build test

default: docs-html

THIS_FILE := $(realpath $(lastword $(MAKEFILE_LIST)))
THIS_DIR := $(shell dirname $(THIS_FILE))
POETRY := poetry -C $(THIS_DIR)

BUILD_DIR := $(THIS_DIR)/_build/auto
_poetry_stamp := $(BUILD_DIR)/.poetry-install.stamp
poetry-install: $(_poetry_stamp)
$(_poetry_stamp): $(THIS_DIR)/poetry.lock $(THIS_DIR)/pyproject.toml
	$(POETRY) install --with=dev
	mkdir -p $(BUILD_DIR)
	touch $@

SPHINX_JOBS ?= auto
SPHINX_ARGS := -W -n -j "$(SPHINX_JOBS)" -a -b dirhtml

DOCS_SRC := $(THIS_DIR)/docs
DOCS_OUT := $(BUILD_DIR)/docs/dev/html
docs-html: poetry-install
	$(POETRY) run sphinx-build $(SPHINX_ARGS) $(DOCS_SRC) $(DOCS_OUT)

docs-serve: poetry-install
	$(POETRY) run sphinx-autobuild $(SPHINX_ARGS) $(DOCS_SRC) $(DOCS_OUT)

CONFIG ?= RelWithDebInfo
build:
	cmake -S . -B "$(BUILD_DIR)" -G "Ninja" -D CMAKE_BUILD_TYPE=$(CONFIG)
	cmake --build "$(BUILD_DIR)" --config $(CONFIG)

test: build
	cmake -E chdir "$(BUILD_DIR)" ctest -C "$(CONFIG)" --output-on-failure -j8

