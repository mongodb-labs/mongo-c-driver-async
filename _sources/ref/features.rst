##################
|amongoc| Features
##################

|amongoc| is the first official asynchronous MongoDB C driver library, and
targets compatibility with the driver library APIs that are common across other
language implementations.

Because the library needs to be built from the ground-up to support asynchronous
control flow, very little of the existing C driver implementation can be used
directly, meaning that much of the feature set needs to be reimplemented. This
page documents the status of the common driver library APIs within |amongoc|.
Since the library is a proof-of-concept, many features have no target release
date, but may be added in some future production-ready version.

.. |yes| replace:: ✅
.. |no| replace:: ❌
.. |never| replace:: 🚫
.. |wip| replace:: ✏

.. rubric:: Legend

- |yes| - Feature is implemented and supported
- |wip| - Feature is partially implemented
- |no| - Feature is not implemented in this proof-of-concept
- |never| - Feature is not planned

.. rubric:: Feature Status

- |wip| **Connection URIs**

  - |yes| **Basic URI parsing** --- Most URI options are recognized, but several are unimplemented
  - |no| **Multi-host URIs** --- Requires a non-standard URI parser
  - |no| **Programmatic connection parameters** --- Only connection URI strings
    are supported.

- |wip| **Connectivity**

  - |yes| **Hostname resolution**
  - |yes| **Transport encryption (TLS)** --- See also: the **TLS** point below
  - |wip| **Connection pooling**

    - |yes| **Basic connection pooling**
    - |no| **Pool options**: ``minPoolSize``, ``maxPoolSize``, ``maxIdleTimeMS``, ``maxConnecting``

  - |no| **SRV discovery**
  - |no| **Compression**
  - |no| **Authentication**
  - |no| **Automatic server selection**
  - |no| **Server discovery and monitoring**
  - |no| **Load balancing**
  - |never| **Legacy protocol support** --- e.g. ``OP_QUERY``

- |wip| **TLS**

  - |yes| **TLS Connectivity**
  - |yes| **Client certificates**
  - |yes| **Custom CA certificates**
  - |yes| **Respects system CAs**
  - |no| **Disable CRLs**
  - |no| **OCSP**

- |wip| **Data management**

  - |yes| **Basic CRUD APIs**
  - |no| **Collection management**
  - |no| **Index management**
  - |no| **Bulk write APIs**
  - |no| **Data encryption**

- |no| **Retryable operations**

  - |no| **Retryable reads**
  - |no| **Retryable writes**
  - |no| **Client sessions**
  - |no| **Transaction support**

- |no| **GridFS**
- |no| **Extended JSON parsing**
- |no| **Client-side monitoring**

  - |no| **Command event monitoring**
  - |no| **SDAM events** (SDAM is not yet supported)
