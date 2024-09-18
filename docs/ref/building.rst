##############################
Configuring, Building, & Using
##############################

.. default-domain:: std
.. default-role:: any

Building |amongoc| is supported with the following build tools:

- CMake_ 3.25 or newer
- GCC ≥12.0 **or** Clang ≥17.0
- Earthly_ 0.8 or newer (for :ref:`building with Earthly <building.earthly>`)

.. _CMake: https://cmake.org/
.. _Earthly: https://earthly.dev/


Build Configuration
###################

The following CMake_ configuration options are supported:

.. cmake:variable:: AMONGOC_INSTALL_CMAKEDIR

  Set the installation path for CMake package files that are to be found using
  :external:cmake:command:`find_package <command:find_package>`

  :default: :sh:`${CMAKE_INSTALL_LIBDIR}/cmake`

  This is based on the similar variables that are used in
  :external:cmake:module:`GNUInstallDirs <module:GNUInstallDirs>`.

.. cmake:variable:: MONGO_SANITIZE

  A comma-separated list of sanitizers to enable for the build. Note that if
  this is enabled, then the exported library will attempt to link to the
  sanitizer libraries as well.

.. cmake:variable:: MONGO_USE_CCACHE

  If enabled, the |amongoc| build will use ``ccache`` for compilation. This
  defaults to ``ON`` if a suitable ``ccache`` executable is found.

.. cmake:variable:: MONGO_USE_LLD

  If enabled, thne |amongoc| build will link using the LLD linker instead of the
  default.

.. cmake:variable:: BUILD_TESTING

  This variables comes from the :cmake:module:`CTest <module:CTest>` CMake
  module and can toggle the generation/building of tests.

.. cmake:variable::
  CMAKE_INSTALL_LIBDIR
  CMAKE_INSTALL_BINDIR
  CMAKE_INSTALL_INCLUDEDIR

  These variables come from
  :external:cmake:module:`GNUInstallDirs <module:GNUInstallDirs>` and control
  the paths to installed files for separate package components. Refer to that
  module for details.

.. cmake:variable::
  AMONGOC_USE_PMM

  Toggle usage of PMM_ to automatically download and import dependencies at
  configure-time.

  .. _PMM: https://github.com/vector-of-bool/pmm
  .. _vcpkg: https://vcpkg.io/

  :default: ``ON``

  If this toggle is enabled, then vcpkg_ will be executed during CMake
  configuration to download and build the dependencies required by |amongoc|.

  If you want to manage dependencies yourself, disable this toggle. You will
  need to ensure that the
  :ref:`configure-time dependencies <building.cmake-deps>` are available to
  :external:cmake:command:`find_package <command:find_package>`.


.. _building.cmake-deps:

Third-Party Dependencies
########################

.. sidebar::

  .. note::

    At time of writing, neo-fun_ does not ship an installable CMake package and
    is installed using a custom vcpkg_ port in ``etc/vcpkg-ports/neo-fun``.

The following external libraries are required by |amongoc|:

- Catch2_ - Only required for testing and only imported if
  :cmake:variable:`BUILD_TESTING` is enabled.
- Asio_ - Used for asynchronous I/O (Note: Not ``Boost.Asio``!)
- Boost.URL_
- `{fmt}`_
- neo-fun_

.. _Catch2: https://github.com/catchorg/Catch2
.. _Asio: https://think-async.com/Asio/
.. _Boost.URL: https://www.boost.io/libraries/url/
.. _{fmt}: https://fmt.dev/
.. _neo-fun: https://github.com/vector-of-bool/neo-fun


.. _building.earthly:

Building with Earthly
#####################

Earthly_ is a container-based build automation tool. |amongoc| ships with an

.. file:: Earthfile

  The configuration file building and package with Earthly_.

  .. _build targets:

  .. earthly-target::
    +build-alpine
    +build-debian
    +build-rl

    Build targets that build for Alpine Linux (with libmusl), Debian, and
    RockyLinux (for RedHat-compatible binaries).

    The Alpine and Debian build uses the system's default toolchain. The
    RockyLinux build uses the RedHat devtoolset to obtain an up-to-date compiler
    for producing RedHat-compatible binaries.

    .. earthly-artifact::
      +build-xyz/pkg
      +build-xyz/install

      Built artifacts from the :ref:`build targets <build targets>`. The
      ``/pkg`` artifact contains binary packages create by CPack: A ``.tar.gz``
      archive, a ``.zip`` archive, and a self-extracting shell script ``.sh``.
      The ``/install`` artifact contains an install tree from the build.

  .. earthly-target:: +build-multi

    Builds all of `+build-alpine`, `+build-debian`, and `+build-rl` at once.

    .. earthly-artifact:: +build-multi/

      The root artifact directory contains all artifacts from all other build
      targets.


Importing in CMake
##################

.. highlight:: cmake

To use |amongoc| in a CMake project, import the ``amongoc`` package:

.. sidebar::

  Note that the CMake package has no ``COMPONENTS``. Specifying any components
  will result in an error at import-time.

::

  find_package(amongoc 0.1.0 REQUIRED)

Using the Imported Target
*************************

The CMake package defines a primary imported target: ``amongoc::amongoc``, which
can be linked into an application::

  add_executbale(my-program main.c)
  target_link_libraries(my-program PRIVATE amongoc::amongoc)

Dependency Imports
******************

By default, the |amongoc| CMake package will attempt to import dependencies
using :cmake:command:`find_dependency <command:find_dependency>`. This import
can be disabled by changing :cmake:variable:`AMONGOC_FIND_DEPENDENCIES`.

.. cmake:variable:: AMONGOC_FIND_DEPENDENCIES

  :default: ``ON``

  .. note::

    This is an **import-time** CMake setting that is defined for projects that
    call ``find_package`` to import ``amongoc``. It has no effect on building
    |amongoc| itself.

  If enabled (the default), then |amongoc| will try to find its dependencies
  during import. If disabled, then |amongoc| will assume that the necessary
  imported targets will be defined elsewhere by the importing package.

  The following imported targets are used by the imported ``amongoc::amongoc``
  target:

  - ``neo::fun``
  - ``asio::asio``
  - ``fmt::fmt``
  - ``Boost::url``

