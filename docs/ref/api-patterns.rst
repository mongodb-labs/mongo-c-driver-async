######################
|amongoc| API Patterns
######################

The following details concern patterns in the |amongoc| API


Naming
######

All C types, macros, functions declared by |amongoc| are prefixed with
``amongoc_``. All public C++ APIs are written within the ``amongoc`` namespace.
All public APIs (including macros and enumerators) are written in ``snake_case``
with lowercase letters.

All public C API structs, unless otherwise noted, are ``typedef``\ 'd to their
own name, meaning that the ``struct`` tag is unnecessary.


C Function Names
****************

Most C API functions follow the naming scheme:

``amongoc_<term>``
    For very-high-level building-block APIs that are expected to be used
    frequently. Examples:

    - `amongoc_just`
    - `amongoc_let`
    - `amongoc_start`
    - `amongoc_is_error`

``amongoc_<subject>_<verb>``
    For functions that act on a single value, where ``<subject>`` is a noun or
    noun phrase that designates the kind of its subject parameter. Examples:

    - `amongoc_emitter_discard`
    - `amongoc_handler_complete`
    - `amongoc_default_loop_run`

``amongoc_<subject>_<verb>_<object>``
    For functions that act on a primary value with a secondary object type,
    where ``<subject>`` and ``<object>`` are nouns or noun phrases corresponding
    to the kind of the associated parameter. Examples:

    - `amongoc_emitter_connect_handler`
    - `amongoc_loop_get_allocator`


Parameter Ordering
##################

C function parameters generally follow a standard ordering as follows:

1. An event loop (`amongoc_loop`)
2. The function subject. This is the primary target upon which a function will
   act. For callbacks this will be the userdata box/pointer.
3. Flags and ancillary parameters
4. Result status (`amongoc_status`)
5. Result value (`amongoc_box`)
6. An allocator (`mlib_allocator`)
7. A userdata pointer or `amongoc_box` to bind with a callback parameter
8. A callback function pointer (example: `amongoc_let`)
