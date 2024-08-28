###########
Couroutines
###########

.. warning:: |devdocs-page|
.. namespace:: amongoc

The C++ implementation of |amongoc| can use C++ coroutines to implement
asynchronous control flow.

For simple operations, prefer to compose :doc:`nanosenders <nanosenders>`
directly. For more complex control flow, using coroutines enhances
maintainability and readability.

.. important::

  Be sure to read the :ref:`alloc` section below, which specifies requirements
  on the parameters of coroutines.


Coroutine Types
###############

`amongoc_emitter` as a Coroutine
********************************

The C API type `amongoc_emitter` is valid as a coroutine function return type.
The coroutine state will be allocated and stored within the
`amongoc_emitter::userdata`, and the coroutine will be destroyed when the
generated emitter is destroyed.

Coroutines created from `amongoc_emitter` are lazily started only when the
emitter is attached to a handler and the operation is launched with
`amongoc_start`.

From within an `amongoc_emitter` coroutine, one of the following can be returned
with :cpp:`co_return`:

- An `emitter_result` -- allows returning both an `amongoc_status` and a boxed
  result value.
- A `unique_box` -- resolves with status `amongoc_okay` and the given value as
  the result value.
- An `amongoc_status` -- the result value will be `amongoc_nil`.
- A `std::error_code` -- this will be converted using `amongoc_status::from`,
  and the result value will be `amongoc_nil`.
- :cpp:`nullptr` or literal :cpp:`0` -- will construct an emitter result with
  `amongoc_okay` and `amongoc_nil`. This return kind is "mere success", akin to
  returning :cpp:`void`.


Exception Handling with `amongoc_emitter`
=========================================

If an `amongoc_emitter` coroutine throws an exception, the following will happen:

1. If the exception is derived from `std::system_error`, the error code will be
   given to `amongoc_status::from` and the resulting status object will be the
   result status of the emitter, with an `amongoc_nil` result value.
2. Otherwise, if the exception type is derived from an `amongoc::exception`,
   then the `exception::status` will be the result status of the emitter.
3. Otherwise, **the program will terminate**. *Don't let this happen!*


`amongoc::co_task` as a Coroutine
*********************************

.. class:: template <typename T> co_task

  This is a dedicated C++ coroutine return type. It implements a `nanosender`
  which sends the type `T`. It is move-only. Using a `co_task` as a coroutine is
  more efficient than using `amongoc_emitter`, but is not part of the public
  API, so its usage is limited to internal interfaces only.

  The `co_task` coroutine type also supports transmitting arbitrary exception
  types when nested awaiting on other `co_task`\ s.

  .. type:: sends_type = T

    Announce the coroutine's result type to `nanosender_traits`

  .. function:: nanooperation auto connect(nanoreceiver_of<T> recv) &&

    Connect the task to a receiver. The returned `nanooperation`'s ``start()``
    function will launch the coroutine.


Exception Handling with `co_task`
=================================

If a `co_task` |A| is awaited within another `co_task` coroutine |B| and the
coroutine associated with |A| throws an exception |x|, then |x| will be
re-thrown within |B| when |A| is :cpp:`co_await`'d.

**When used as a** `nanosender` (i.e. with `co_task::connect` or other
nanosender APIs), if a `co_task` throws an exception, then an attempt will be
made to convert the thrown exception to the result type of the coroutine by
calling a static ``T::from_exception(eptr)`` function on the result type ``T``,
where ``eptr`` is the `std::exception_ptr` associated with the thrown exception.
**If** the result type ``T`` does not have a ``T::from_exception``, then **the
program will terminate**. Don't let this happen!


Awaitable Types
###############

Within an `amongoc_emitter` coroutine or a `co_task` coroutine, any type that
meets `nanosender` is valid for :cpp:`co_await`-ing (this includes
`amongoc_emitter` itself, since it implements the `nanosender` interface).

When awaiting a `nanosender` |S|, a special receiver will be connected to |S|
that will resume the parent coroutine. This will schedule the coroutine to be
resumed by |S| when it invokes the attached receiver.

The result type from the :cpp:`co_await` on the `nanosender` will be the
`sends_t` of that nanosender.


Exception Throwing
******************

`nanosender`\ s, unlike P2300 senders, do not have a distinct error channel. For
that reason, :cpp:`co_await`-ing a nanosender will never throw an exception.
Instead, error information must be transmitted through the nanosender's result
type.

The only exception to this no-exception-rule is `co_task`, which has special
machinery to support the propagation of exceptions between `co_task`\ s.

.. _alloc:

Memory Allocation
#################

C++ coroutines support customizing the allocation of the coroutine's state.
Coroutines based on `amongoc_emitter` and `co_task` will *refuse* to use the
default :cpp:`operator new`, and *require* that a `cxx_allocator` is provided to
the coroutine.

For this reason, a `co_task` or `amongoc_emitter` coroutine *must* accept as its
first parameter one of:

1. A pointer to `amongoc_loop` (which is assumed to never be :cpp:`nullptr`!) --
   The `cxx_allocator` will be obtained from the event loop.
2. A `cxx_allocator` directly.
3. An `amongoc_allocator`, which will be converted to a `cxx_allocator`.
4. Any type which supports `get_allocator` with an allocator that is convertible
   to a `cxx_allocator`.

If this requirement is not met, then the coroutine will fail to compile when
attempting to resolve the :cpp:`operator new` for the coroutine.

.. rubric:: Example

Note that the C++ coroutine machinery handles this transparently, so the
parameter need only be present, not necessarily used within the coroutine
itself::

  co_task<int> add_numbers(cxx_allocator<> /* unnamed */, int a, int b) {
    co_return a + b;
  }

In the above, event though the `cxx_allocator` parameter is unnamed and unused
within the coroutine body, it will still be used by the coroutine's promise to
allocate memory for the coroutine state.
