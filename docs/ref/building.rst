##############################
Configuring, Building, & Using
##############################

.. default-domain:: std
.. default-role:: any

Building |amongoc| requires a C++20 compiler. The following tools are known to
work:

- CMake_ 3.25 or newer
- GCC ≥12.0\ [#fn-redhat-issue]_ **or** Clang ≥17.0
- Earthly_ 0.8 or newer (for :ref:`building with Earthly <building.earthly>`)

Building on Windows or with MSVC is not currently supported.

.. _CMake: https://cmake.org/
.. _Earthly: https://earthly.dev/

.. _building.deps:

Third-Party Dependencies
########################

When configuring and building |amongoc| with CMake, the configuration script
will attempt to automatically obtain build-time dependencies using vcpkg_. This
behavior can be disabled by setting :cmake:variable:`AMONGOC_USE_PMM` to
``OFF``. Relying on vcpkg_ will attempt to build the third-party dependencies
during project configuration, which may require additional configure-time
dependencies.

The following external libraries are required when **building** |amongoc|. If
not relying vcpkg_, then they will need to be installed and available before
configuring the project:

- OpenSSL
- Boost.URL and Boost.Container
- `{fmt}`_
- **If using vcpkg to install dependencies** (the default), then you will also
  need to have Tar, cURL, Perl, Zip/Unzip, pkg-config, Make, and Bash. On Linux,
  you will also need the linux development headers.

The following packages are downloaded using CMake's FetchContent, and need not
be manually installed:

- Catch2_ - Only required for testing and only imported if
  :cmake:variable:`BUILD_TESTING` is enabled.
- Asio_ - Used for asynchronous I/O (Build-time only) (Note: Not ``Boost.Asio``!)
- neo-fun_ (Build-time only)

.. _Catch2: https://github.com/catchorg/Catch2
.. _Asio: https://think-async.com/Asio/
.. _{fmt}: https://fmt.dev/
.. _neo-fun: https://github.com/vector-of-bool/neo-fun

The following table details the dependencies of the imported
``amongoc::amongoc`` target, along with the common Linux packages that contain
them:

.. list-table::
  :header-rows: 1
  :widths: 1, 2, 2, 2

  - - Targets
    - RedHat
    - Debian
    - Alpine

  - - ``fmt::fmt``
    - ``fmt-devel`` (may require EPEL)
    - ``libfmt-dev``
    - ``fmt-dev``
  - - ``Boost::url`` + ``Boost::container``
    - ``boost-devel``
    - ``libboost-{url,container}-dev`` (may require a version infix)
    - ``boost-dev``
  - - ``OpenSSL::SSL``
    - ``openssl-devel``
    - ``libssl-dev``
    - ``openssl-dev``
  - - ``Threads::Threads``
    - CMake built-in
    - CMake built-in
    - CMake built-in



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
  configure-time using vcpkg_.

  .. _PMM: https://github.com/vector-of-bool/pmm
  .. _vcpkg: https://vcpkg.io/

  :default: ``ON`` if configuring |amongoc| as the top-level project, ``OFF``
    otherwise (e.g. when added as a sub-project)

  If this toggle is enabled, then vcpkg_ will be executed during CMake
  configuration to download and build the dependencies required by |amongoc|.

  If you want to manage dependencies yourself, disable this toggle. You will
  need to ensure that the :ref:`configure-time dependencies <building.deps>` are
  available to :external:cmake:command:`find_package <command:find_package>`.


.. _building.earthly:

Building with Earthly
#####################

Earthly_ is a container-based build automation tool. |amongoc| ships with an
Earthfile that eases building by using containerization.

.. file:: Earthfile

  The configuration file building and package with Earthly_.

  .. _build targets:

  .. earthly-target::
    +build-alpine
    +build-debian
    +build-fedora
    +build-rl

    Build targets that build for Alpine Linux (with libmusl), Debian, Fedora,
    and RockyLinux (for RedHat-compatible binaries).

    The Alpine, Fedora, and Debian build uses the system's default toolchain.
    The RockyLinux build uses the RedHat devtoolset\ [#fn-redhat-issue]_ to
    obtain an up-to-date compiler for producing RedHat-compatible binaries.

    .. earthly-artifact::
      +build-xyz/pkg
      +build-xyz/install

      Built artifacts from the :ref:`build targets <build targets>`. The
      ``/pkg`` artifact contains binary packages create by CPack: A ``.tar.gz``
      archive, a ``.zip`` archive, and a self-extracting shell script ``.sh``.
      The ``/install`` artifact contains an install tree from the build.


    .. rubric:: Example

    To build and obtain a package for Debian-compatible systems, the following
    command can be used to obtain the packages for the `+build-debian` target:

    .. code-block:: console

      $ earthly -a +build-debian/pkg deb-pkg
      ## [Earthly output] ##
      $ ls deb-pkg
      amongoc-0.1.0-linux-x86_64.sh*
      amongoc-0.1.0-linux-x86_64.tar.gz
      amongoc-0.1.0-linux-x86_64.zip

    The resulting ``.sh`` script can be used to install the built library and
    headers.

    The same command can work for the `+build-alpine`, `+build-fedora`, and
    `+build-rl` targets.


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

By default, the |amongoc| CMake package will attempt to import dependencies
using :cmake:command:`find_dependency <command:find_dependency>`. This import
can be disabled by changing :cmake:variable:`AMONGOC_FIND_DEPENDENCIES`.

**If you build** |amongoc| using vcpkg_ (the default) it is highly recommended
to use vcpkg in your own project to install |amongoc|'s dependencies, as it is
not guaranteed that the packages provided elsewhere will be compatible with the
packages that were used in the |amongoc| build.

.. cmake:variable:: AMONGOC_FIND_DEPENDENCIES

  :default: ``ON``

  .. note::

    This is an **import-time** CMake setting that is defined for projects that
    call ``find_package`` to import ``amongoc``. It has no effect on building
    |amongoc| itself.

  If enabled (the default), then |amongoc| will try to find its dependencies
  during import. If disabled, then |amongoc| will assume that the necessary
  imported targets will be defined elsewhere by the importing package.


.. rubric:: Footnotes

.. [#fn-redhat-issue]

  There is a known issue with the RedHat dev toolset that results in certain
  internal symbols being incorrectly discarded and producing link-time errors.
