################
Platform Support
################

This page details information on the compiler/platform support of |amongoc|.

.. NOTE
  This page should be kept in sync with our CI tasks to ensure that it remains
  correct. The main CI tasks defined in `ci.yml` define our platform support.


Operating Systems
#################

Linux
*****

Full Support with Default Toolchain
===================================

|amongoc| is tested and supported on the following Linux distributions using the
toolchains and libraries that are available in their default package
repositories:

1. **Debian 12** and newer
2. **Ubuntu 24.04** and newer
3. **Fedora 40** and newer
4. **Alpine 3.18** and newer


Partially Supported
===================

|amongoc| is tested and supported on the following Linux platorms with some
caveats:

1. **Alpine 3.17** and **RHEL 10** (and derived) can build and run |amongoc|
   using the default platform toolchain, but it requires library dependencies
   that are not available in the default respositories. Building using ``vcpkg``
   is fully supported.
2. **RHEL 8** (and derived) can build and run |amongoc|, but it requires
   dependencies that are not available in the default repositories, and requires
   using a newer ``gcc-toolset`` (≥12.0) package to build.


Windows
*******

|amongoc| is supported on Windows using the **Visual Studio 2022** toolchain and
``vcpkg`` to obtain dependencies.

.. note::

  Building using other toolchains on Windows is not tested.


Compilers and Toolchains
########################

Fully Supported
***************

The following toolchains are fully supported:

- **GCC 12** and newer. See also: :ref:`gcc-linker-issue`
- **LLVM/Clang 17** and newer
- **Microsoft Visual Studio 2022** and newer

.. note::

  Apple/Clang is not tested.


.. _gcc-linker-issue:

GCC Linker Issues
*****************

While **GCC 13** and **GCC 12** can build |amongoc|, a bug in GCC causes a
linker error when all optimizations are enabled. The |amongoc| CMake build
detects these GCC versions and disables the specific optimizations that cause
the linker issue. If you see a linker issue that only appears when optimizations
are enabled, please file a bug report.
