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

  Be sure to read the :ref:`co.alloc` section below, which specifies requirements
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
- A `std__error_code` -- this will be converted using `amongoc_status::from`,
  and the result value will be `amongoc_nil`.
- :cpp:`nullptr` or literal :cpp:`0` -- will construct an emitter result with
  `amongoc_okay` and `amongoc_nil`. This return kind is "mere success", akin to
  returning :cpp:`void`.


Exception Handling with `amongoc_emitter`
=========================================

If an `amongoc_emitter` coroutine throws an exception, the following will happen:

1. If the exception is derived from `std__system_error`, the error code will be
   given to `amongoc_status::from` and the resulting status object will be the
   result status of the emitter, with an `amongoc_nil` result value.
2. Otherwise, if the exception type is derived from an `amongoc::exception`,
   then the `exception::status` will be the result status of the emitter.
3. Otherwise, if the exception type is `std__bad_alloc`, the emitter
   will resolve with generic cateogry and ``ENOMEM``.
4. Otherwise, **the program will terminate**. *Don't let this happen!*


`amongoc::co_task` as a Coroutine
*********************************

.. class:: template <typename T> co_task

  This is a dedicated C++ coroutine return type. It is move-only. Using a
  `co_task` as a coroutine is more efficient than using `amongoc_emitter`, but
  is not part of the public API, so its usage is limited to internal interfaces
  only.

  .. type:: result_type = result<T, std__exception_ptr>

  .. function:: nanosender auto as_sender() &&

    Convert the coroutine to a nanosender. It will send a `result_type` object.


Exception Handling with `co_task`
=================================

If a `co_task` |A| is awaited within another `co_task` coroutine |B| and the
coroutine associated with |A| throws an exception |x|, then |x| will be
re-thrown within |B| when |A| is :cpp:`co_await`'d.

**When used as a** `nanosender` (i.e. with `co_task::as_sender` ), if the
underlying coroutine throws an unhandled exception, then the sent `result`
object will contain a `std__exception_ptr` for that exception.


Awaitable Types
###############

Within an `amongoc_emitter` coroutine or a `co_task` coroutine, any type that
meets `nanosender` is valid for :cpp:`co_await`-ing (this includes
`unique_emitter` itself, since it implements the `nanosender` interface).

When awaiting a `nanosender` |S|, a special receiver will be connected to |S|
that will resume the parent coroutine. This will schedule the coroutine to be
resumed by |S| when it invokes the attached receiver.

The result type from the :cpp:`co_await` on the `nanosender` will be the
`sends_t` of that nanosender. When awaiting a `unique_emitter`, this will be an
`emitter_result`.


Exception Throwing
******************

`nanosender`\ s, unlike P2300 senders, do not have a distinct error channel. For
that reason, :cpp:`co_await`-ing a nanosender will never throw an exception.
Instead, error information must be transmitted through the nanosender's result
type.


.. _co.alloc:

Memory Allocation
#################

C++ coroutines support customizing the allocation of the coroutine's state.
Coroutines based on `amongoc_emitter` and `co_task` will *refuse* to use the
default :cpp:`operator new`, and *require* that a `mlib::allocator` is
provided to the coroutine.

For this reason, a `co_task` or `amongoc_emitter` coroutine *must* accept as its
first parameter one of:

1. A pointer to `amongoc_loop` (which is assumed to never be :cpp:`nullptr`!) --
   The `mlib::allocator` will be obtained from the event loop.
2. A `mlib::allocator` directly.
3. An `mlib_allocator`, which will be converted to a `mlib::allocator`.
4. Any type which supports `get_allocator` with an allocator that is convertible
   to a `mlib::allocator`.

If this requirement is not met, then the coroutine will fail to compile when
attempting to resolve the :cpp:`operator new` for the coroutine.

.. rubric:: Example

Note that the C++ coroutine machinery handles this transparently, so the
parameter need only be present, not necessarily used within the coroutine
itself::

  co_task<int> add_numbers(allocator<> /* unnamed */, int a, int b) {
    co_return a + b;
  }

In the above, event though the `mlib::allocator` parameter is unnamed and
unused within the coroutine body, it will still be used by the coroutine's
promise to allocate memory for the coroutine state.

Allocation Failure
******************

If allocation fails for a `co_task` coroutine, then the coroutine function will
immediately throw without returning a `co_task` object. If allocation fails for
an `amongoc_emitter` coroutine, then the returned emitter will be from
`amongoc_alloc_failure`.


Parameter Lifetimes
###################

An important thing to remember about coroutines is that the parameters are
captured by their declared type. This means that reference parameters are
captured by reference, including reference-like types (e.g. `std__string_view`
and `bson_view`).

Because of these capture semantics, care should be taken that reference-like
parameters outlive the coroutine body for the duration that they are used. This
can be done in one of three ways:

1. Capture only using value types. This means that `std__string_view` and
   :cpp:`const std::string&` should be passed as `std__string` instead.
2. Document the lifetime requirements of reference-like parameters. This places
   the onus on the user, and is often less than ideal.
3. Create a shim function that copies arguments by-value before calling the
   real coroutine::

    emitter resolve_addr(const char* address) {
      return _co_resolve(std::string(address));
    }

    static emitter _co_resolve(std::string s) {
      co_await do_stuff(s);
    }

Note that this is a non-issue for coroutines that are immediately
:cpp:`co_await`'d in their caller's scope, since the lifetime of the arguments
is guaranteed to be at least the lifetime of the coroutine itself::

  co_task<int> use_string(std::string const& s) {
    co_await do_stuff(s);
    co_return 0;
  }

  co_task<int> outer_co() {
    co_await use_string("I am a string");
  }

In the above, a temporary `std__string` is passed to ``use_string`` and the
reference parameter will be bound to that temporary. **This is safe here,** because
the coroutine for ``use_string`` is immediately awaited and is guaranteed to
complete before the temporary string is destroyed.


Coroutines Versus Nanosenders
#############################

It is reasonable to ask when to use coroutines versus using `nanosender`\ s
directly. It may be tempting to use coroutines *always*, since they are easier
to write and read than a pipeline of `then <amongoc::then>` and
`let <amongoc::let>` closures.

The following drawbacks of coroutines over pure nanosenders might be considered:

1. A coroutine often requires requires dynamic memory allocation, unless the
   compiler can perform allocation elision, which is still a very fragile
   optimization. A composed nanosender will often require no dynamic memory
   allocation at all!

   However, this allocation requirement is not usually a problem for
   `amongoc_emitter` coroutines, since they would need to dynamically allocate
   storage anyway if they would need to use nanosenders that wouldn't fit inside
   of an `amongoc_box`.
2. Reference parameter lifetime can be tricky to deal with. This is managable,
   but requires care.
3. A `amongoc_emitter` created from a coroutine will require slightly more
   memory than an `amongoc_emitter` created from an equivalent `nanosender`.


When to Use Coroutines
**********************

It should also be considered that coroutines will *astronomically* improve
maintainability in the face of non-trivial control flow, such as looping,
branching, recursion, and error propagation.

In general, prefer coroutines for high-level constructs that require non-trivial
control flow.


When to Use Nanosenders
***********************

The pure `nanosender` APIs should be used for very small building-blocks and
high-traffic APIs, since they are guaranteed to be non-allocating.


Coroutine Machinery in |amongoc|
################################

This section will be a crash-course on C++20 coroutine machinery and how it is
implemented in |amongoc|.

.. seealso::

  For a more detailed explanation of how coroutines operate, see
  `the cppreference page about C++ coroutines`__.

  __ https://en.cppreference.com/w/cpp/language/coroutines

C++ coroutines give a large amount of flexibility to the author in terms of how
they are scheduled and how they communicate with their surrounding context.


Triggering Coroutine Magic
**************************

The C++ coroutine machinery is not triggered by the signature of the function,
but by the presence of a coroutine control keyword within the function body
(i.e. :cpp:`co_await`, :cpp:`co_return`, or :cpp:`co_yield`). **A function is
not a coroutine unless it uses a coroutine keyword, even if the return type of
the function is a coroutine type**. For example::

  co_task<int> this_is_not_a_coroutine() {
    return this_is_a_coroutine();
  }

  co_task<int> this_is_a_coroutine() {
    co_return 42;
  }

In the above, ``this_is_not_a_coroutine`` is a regular function that happens to
return a `co_task` object.

When the compiler sees a coroutine control keyword, it transforms the function
definition into a coroutine function. It uses `std__coroutine_traits` to look
up the :term:`promise <coroutine promise>` of the coroutine.


Support Concepts
****************

.. concept:: template <typename A> awaiter

  .. note:: This is not a real library concept, and is only for illustrative purposes

  An awaiter is used at a :cpp:`co_await` expression to control the behavior of
  the that :cpp:`co_await` expression and the (possible) suspension of the
  enclosing coroutine.

  .. function::
    template <typename P> \
    auto await_suspend(std::coroutine_handle<P> suspender)

    :tparam P: The :term:`promise <coroutine promise>` type of the coroutine
      that is being suspended.

    When the parent coroutine suspends, it will call `await_suspend` with a
    :term:`handle to the coroutine <coroutine handle>` that is being suspended.
    This give the awaiter the opportunity to reschedule the coroutine at some
    point in the future. If `await_suspend` returns a new coroutine handle |R|,
    then |R| will be resumed after the enclosing coroutine suspends (this is
    known as *symmetric transfer*). If `await_suspend` returns :cpp:`void`,
    then control returns to the *resumer* of the coroutine.

  .. function::
    auto await_resume()

    When the coroutine is resumed, the `await_resume` function will determine
    the result of the :cpp:`co_await` expression that uses the awaiter.

  .. function::
    bool await_ready()

    If the ``await_ready`` function returns :cpp:`true`, then the coroutine will
    skip the call of `await_suspend` and immediately calls `await_resume` to
    obtain the :cpp:`co_await` result without ever suspending the coroutine.


.. concept:: template <typename A> awaitable

  An object |A| is *awaitable* if:

  1. It has a member :cpp:`operator co_await()` that returns an `awaiter`
  2. There is a non-member :cpp:`operator co_await(A)` that returns an `awaiter`
  3. The object |A| is itself a valid `awaiter`

  When a :cpp:`co_await` expression appears in a coroutine body, the compiler
  will attempt to obtain an `awaiter` according to the above rules.


.. glossary::

  coroutine handle

    The handle of a coroutine is a pointer-like type accessed using
    `std::coroutine_handle\<P> <std__coroutine_handle>`, where the template
    parameter ``P`` is the :term:`promise <coroutine promise>` type of the
    coroutine, or :cpp:`void` to represent a handle to a coroutine with an
    unknown promise type.

    The coroutine handle allows one to resume or destroy a coroutine. If the
    template argument is non-:cpp:`void`, then the promise object can also be
    obtained via the coroutine handle.


Promise Types
*************

.. glossary::

  coroutine promise

    The coroutine *promise* implements the primary control surface for a C++
    coroutine.

    When a coroutine function is called, a promise object will be created
    automatically in an unspecified storage location (usually dynamically
    allocated beside the coroutine state, but may be elided). The promise will
    live as long as the coroutine is alive, and will be destroyed when the
    coroutine is destroyed. A promise object is never moved nor copied (its
    address is always stable).

    |amongoc| uses two main promise types: `emitter_promise` and
    `co_task\<T>::promise_type`.

A coroutine promise should implement the following interface:

.. concept:: template <typename P> promise_interface

  .. note::

    This is not a real library concept, and is only to describe the coroutine promise
    interface.

  .. function:: get_return_object()

    This function will be called to get the object that is returned when a
    coroutine function is initially called. In the following::

      my_coroutine_obj do_stuff() {
        co_await do_other_stuff();
      }

    ::

      my_coroutine_obj o = do_stuff();

    The promise's `get_return_object` is responsible for constructing the
    ``my_coroutine_obj`` object returned when ``do_stuff()`` is first called.

  .. function:: static get_return_object_on_allocation_failure()

      This is required if the promise implements custom memory allocation in a
      way that doesn't throw.

      This will be called instead of `get_return_object` if the custom
      `operator new` returns a null pointer.

  .. function:: void unhandled_exception()

    This function is invoked as-if within a :cpp:`catch` block that encloses the
    entire coroutine body. It allows the coroutine to handle exceptions that
    escape without being handled.

  .. function:: awaiter auto initial_suspend()

    Must return an `awaiter` that acts as the initial suspend point for
    the coroutine. This happens before any code within the coroutine body
    executes. This is usually `std__suspend_always` or `std__suspend_never`.

  .. function:: awaiter auto final_suspend()

    Must return an `awaiter` that acts as the final suspend point for the
    coroutine. This happens after all code within the coroutine body executes.
    It runs after any :cpp:`co_return` statement or after an unhandled exception
    escapes.

    If the coroutine does not suspend at its final suspend point, then the
    coroutine will be immediately destroyed by the runtime and all outstanding
    :term:`coroutine handles <coroutine handle>` are invalidated (for this
    reason, it is most common to always suspend at the final suspend point).

  .. function:: void return_value(auto&& x)

    This function is invoked when a :cpp:`co_return` statement is executed in
    the coroutine body. The parameter `x` is the operand to the :cpp:`co_return`
    statement.

  .. function::
    promise_interface(auto&&... args)
    void* operator new(std__size_t n, auto&&... args)
    void operator delete(void* p, std__size_t n)

    Implements dynamic memory allocation and construction for the coroutine
    state and promise. `n` specifies the minimum number of bytes required for
    the coroutine state. The arguments `args` are the arguments that are given
    when the coroutine function was invoked. This allows `operator new` and the
    promise constructor to have access to any arguments passed to the coroutine,
    allowing for allocator injection and behavior customization.

    .. important::

      At time of writing, GCC has a bug if the coroutine function is a
      non-static member function and the promise has customized `operator new`.
      C++ requires that the object of the member function is passed as the first
      argument in `args`, but GCC 14 itself will crash if it encounters this
      situation.

  .. function::
      auto get_stop_token()
      auto get_allocator()

      .. note:: These functions are not part of the standard C++ coroutine
        interface, but are used by |amongoc| to transmit stop tokens and
        allocators between coroutines and `nanosender`\ s.

      Returns the stop token and the allocator associated with the coroutine.

.. class:: coroutine_promise_allocator_mixin

  A mixin base class for promise types that implements dynamic memory allocation
  according to :ref:`co.alloc`.

.. struct:: emitter_promise : coroutine_promise_allocator_mixin

  Implements `promise_interface` for coroutines with an `amongoc_emitter`
  declared return type. Only the notable members are documented below.

  .. member:: unique_handler fin_handler

    This is the handler that is attached to the emitter during
    `amongoc_emitter_connect_handler`. This starts out as a null handler until
    the emitter is connected.

  .. function:: handler_stop_token get_stop_token() const

    Returns the stop token associated with `fin_handler`.

  .. function:: static amongoc_emitter get_return_object_on_allocation_failure() noexcept

    Returns an emitter from `amongoc_alloc_failure`.

  .. function:: void unhandled_exception() noexcept

    Implements a conversion from an unhandled exception type to an
    `amongoc_status` value that will be sent to `fin_handler`. Only a subset of
    exception types are supported, and other exceptions will cause the program
    to terminate.

  .. function:: awaiter auto final_suspend() noexcept

    During final suspension, the handler `fin_handler` will be completed with
    the final result value for the coroutine.

.. struct:: template <typename T> co_task<T>::finisher_base

  Abstract base class that implements the behavior when a `co_task` completes.
  This allows for a `co_task` to act as a `nanosender` or as an `awaitable`.

  .. function:: virtual std::coroutine_handle<> on_final_suspend() = 0

    Called after final suspension of the coroutine. The returned coroutine will
    be resumed after the `co_task` completes.

  .. function:: virtual in_place_stop_token stop_token() = 0

    Obtian the stop token for use with the coroutine.

.. struct:: template <typename T> co_task<T>::promise_type : coroutine_promise_allocator_mixin

  Implements the `promise_interface` for `co_task` coroutines.

  .. member:: finisher_base* _finisher

    A pointer to a concrete `finisher_base` object that tells the coroutine how
    to execute.

  .. function:: in_place_stop_token get_stop_token()

    Obtains a stop token via `_finisher` `finisher_base::stop_token`

  .. function:: awaiter auto final_suspend()

    Implements the final suspend point. Final suspension simply returns the
    coroutine handle of `finisher_base::on_final_suspend` from `_finisher`. The
    behavior of `_finisher` depends on whether the task is used as a
    `nanosender` or as an `awaitable`:

    1. As a `nanosender`, the finisher will invoke the receiver attached to the
       nanosender, passing it the `result_type` object for the coroutine.
    2. As an `awaitable`, the finisher will resume the coroutine that is
       awaiting the `co_task`, at which point the :cpp:`co_await` will either
       throw an exception or return the successful return value from the
       `co_task`.


Awaiting a Nanosender
*********************

`nanosender` await is performed by a non-member :cpp:`operator co_await()` that
is constrained on the `nanosender` concept. It returns a `nanosender_awaiter`.

.. class:: template <nanosender S> nanosender_awaiter

  Implements an `awaiter` object for a `nanosender` |S|.

  .. function:: bool await_ready() const

    Returns ``true`` if-and-only-if the underlying nanosender is known to
    complete synchronously.

  .. function:: sends_t<S> await_resume() noexcept

    Returns the result value that was sent by the enclosed nanosender `S`. This
    causes :cpp:`co_await` on a `nanosender` to return `sends_t` of the
    nanosender type.

  .. function:: template <typename Promise> void await_suspend(std::coroutine_handle<Promise> suspender)

    Handles suspension. An internal `nanoreceiver_of\<sends_t\<S>>` |R| is
    created, which, when invoked, will call ``resume()`` on the `suspender`
    coroutine. The wrapped nanosender |S| is `connected <amongoc::connect>` to
    the receiver |R| and the resulting `nanooperation` is launched immediately.
    The operation state is stored within the awaiter.

    :doc:`Queries <queries>` on the receiver |R| are forwarded to the promise of
    `suspender`. This exposes the stop token and allocator of the enclosing
    coroutine through the receiver |R|, and is required for cancellation to
    work.


Awaiting a `co_task`
********************

Awaiting on a `co_task` calls a member function :cpp:`operator co_await` on the
`co_task`. This returns a ``co_task<T>::awaiter`` object.

.. class:: template <typename T> co_task<T>::awaiter

  Implements an `awaiter` object for a `co_task`

  .. function:: bool await_ready() const

    Always returns :cpp:`false` (`co_task` coroutines are lazy and never
    complete immediately).

  .. function:: T await_resume()

    Obtain the result of the awaited coroutine. If the awaited coroutine threw
    an exception, this function will re-throw that same exception.

  .. function::
    template <typename P> std::coroutine_handle<> await_suspend(std::coroutine_handle<P> suspender)

    Suspends the parent coroutine and immediately launches the awaited coroutine.

    As with `nanosender_awaiter`, the stop token from `P` will be passed through
    to the enclosing `co_task`
