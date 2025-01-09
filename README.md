# `amongoc` - An Asynchronous MongoDB C Client Library

> [!IMPORTANT]
> This is NOT a supported MongoDB product!
>
> The content of this repository should be considered experimental and subject
> to breaking changes at any time.

<!-- Any information written here must be kept in-sync with the relevant
documentation pages, noted in comments. -->

This is `amongoc`, a C11 MongoDB asynchronous client library. It implements a
subset of the MongoDB driver APIs.

<!-- Details in ref/building.rst -->
`amongoc` is written in C++20 and requires an up-to-date compiler. The following
are recommended:

- CMake 3.25 or newer
- GCC ≥12.0 or Clang ≥17.0
- Earthly 0.8 or newer (for building with Earthly)

Building on Windows or with MSVC is not currently supported.

For more information on building, refer to
[the reference documentation][docs-building].


## Links

- [GitHub Repository][github]
- [Documentation Home][docs]
  - [Tutorials][docs-learn]
  - [Reference: Configuring, Building, and Using][docs-building]

[github]: https://github.com/mongodb-labs/mongo-c-driver-async
[docs]: https://mongodb-labs.github.io/mongo-c-driver-async/
[docs-learn]: https://mongodb-labs.github.io/mongo-c-driver-async/learn/
[docs-building]: https://mongodb-labs.github.io/mongo-c-driver-async/ref/building/
