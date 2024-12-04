###########
Nanosenders
###########

.. warning:: |devdocs-page|

|amongoc| contains a modified subset of the P2300__ Senders+Receivers library
known as *nanosenders*. This name reflects the reduced/simplified feature set
and API surface.

__ https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2300r10.html

.. namespace:: amongoc


Traits & Types
##############

.. struct:: template <typename S> nanosender_traits

  Allows customization of a type `S` to behave as a `nanosender`

  .. type:: sends_type = S::sends_type

    The type that will be *sent* by `S` when it completes.

  .. function::
      template <nanoreceiver_of<sends_type> R> \
        nanooperation auto connect(S&& s, R&& r) [[1]]
      template <nanoreceiver_of<sends_type> R> \
        nanooperation connect(const S& s, R&& r) [[2]]

    Connects a nanosender `s` to a nanoreceiver `r`.

    - Overload #1 calls :expr:`std__move(s).connect(std::forward<R>(r))` (Move-connects)
    - Overload #2 calls :expr:`s.connect(std::forward<R>(r))` (Copy-connects)

    Both overloads are constrained on whether their associated expression is
    valid. This call will return a `nanooperation`.

.. type:: template <typename S> sends_t

  Get the type that will be sent by a `nanosender` via `nanosender_traits`.
  Yields the type :cpp:`nanosender_traits<std::remove_cvref_t<S>>::sends_type`.


.. class:: template <typename T> archetype_nanoreceiver

  A type that implements `nanoreceiver_of<T>`


Concepts
########

.. concept:: template <typename S> nanosender

  .. var::
    S&& s
    archetype_nanoreceiver<sends_t<S>> recv

    .. rubric:: Requires:

    - :cpp:`typename sends_t<S>` must name a non-``void`` type
    -
      :cpp:`nanosender_traits<std::remove_cvref_t<S>>::connect(std::move(s), std::move(recv))`
      -- Must create a `nanooperation` by connecting the nanosender `s` to the nanoreceiver `recv`

.. concept:: template <typename S, typename T> nanosender_of

  Matches a `nanosender` `S` whose :expr:`sends_t<S>` is convertible to `T`.

.. concept:: template <typename R, typename T> nanoreceiver_of = std::invocable<R, T>

  The object must be invocable with the given value as its sole argument. The
  receiver and value will be perfect-forwarded for the invocation.

  A nanoreceiver can safely assume that it will only be invoked *once*.

.. concept:: template <typename R, typename S> nanoreceiver_for = \
      nanosender<S> and nanoreceiver_of<sends_t<S>>

  Check that the type `R` is a valid receiver for the sender `S`.

.. concept:: template <typename O> nanooperation

  A type that holds the *operation state* of a connected `nanosender` and associated
  `nanoreceiver <nanoreceiver_of>`.

  .. var:: O& op

    .. rubric:: Requires:

    - :expr:`op.start()` ``noexcept`` -- Launches the associated operation.


Functions
#########

.. function::
    template <nanosender S, nanoreceiver_for<S> R> \
    nanooperation auto connect(S&& s, R&& r)

  Connects a nanosender `s` to a nanoreceiver `r`. Perfect-forwards each
  argument. Returns a new operation state.

  .. note:: This is an invocable object, not a function template


.. function::
  template <nanosender S, std::invocable<sends_t<S>> H> \
    requires nanosender<std::invoke_result_t<H, sends_t<S>>> \
  nanosender auto let(S&& s, H&& handler) [[1]]
  auto let(auto&& handler) [[2]]

  Create a continuation sender |S_ret| for the nanosender `s`. The invocable
  `handler` must return a new `nanosender` when invoked with the value sent by
  `s`.

  The overload ``[[2]]`` of `let` that accepts only a `handler` returns a
  closure object that can be used as the right-hand size of an ``operator|``.
  The expression ``s | let(h)`` is equivalent to ``let(s, h)``.

  :param s: A nanosender to be continued.
  :param handler: A handler function that must accept a `sends_t<S>` argument
    and must return a `nanosender` object.
  :return: A new nanosender |S_ret|, which sends ``sends_t<invoke_result_t<H, sends_t<S>>>``

  When `s` completes, the `handler` will be invoked with the result from `s` to
  obtain a new `nanosender` |S'|.

  |S'| will be immediately `connect`\ ed to another receiver to form a new
  `nanooperation` |O'|, which will be started immediately to continue the
  composed operation. The result value sent by |S'| will be re-sent via |S_ret|.

  This is the C++ equivalent of `amongoc_let` (and `amongoc_let` is implemented
  in terms of `let`).


.. function::
    template <nanosender S, std::invocable<sends_t<S>> H> \
    nanosender auto then(S&& s, H&& handler) [[1]]
    auto then(auto&& handler) [[2]]

  Create a continuation sender |S_ret| for the nanosender `s`. The return value
  from `handler` will be the new value that is sent by |S_ret|.

  The overload ``[[2]]`` of `then` that accepts only a `handler` returns a
  closure object that can be used as the right-hand size of an ``operator|``.
  The expression ``s | then(h)`` is equivalent to ``then(s, h)``.

  :param s: A `nanosender` to be composed.
  :param handler: A handler function that must be invocable with `sends_t\<S>`,
    which returns a |T|.
  :return: A new `nanosender` |S_ret| that sends a |T|.


Classes
#######

.. class::
  template <typename Predicate, nanosender... S> first_where
  template <nanosender... S> first_completed

  Provides a `nanosender` |S| that completes with a
  :expr:`std::variant<sends_t<S...>>` |V|, where the active alternative in |V|
  corresponds to the nanosender |S| which first completed.

  The `Predicate` type is a predicate that determines when to accept a value
  from the input senders. A `first_completed` sender is equivalent to a
  `first_where` that accepts every value value it sees.

  When the first value is accepted, all other pending nanosenders will be
  cancelled immediately. |S| will only resolve once all input senders resolve,
  so it is essential that the input senders respect cancellation otherwise the
  operation for |S| will stall waiting for the senders to complete normally.

  .. type:: sends_type = std::variant<sends_t<S>...>

  .. rubric:: CTAD

  `first_completed` supports CTAD, and is recommended for most cases.

.. class:: template <typename T> just

  Provides a `nanosender` |S| that immediately completes with a `T`. The
  connected receiver will be invoked within the ``start()`` call on the
  resulting operation.

  .. type:: sends_type = T

  .. note::

    The stored value will be perfect-forwarded and supports reference types for
    `T`:

    - If given an :term:`lvalue` |x|, then `just` will store an lvalue reference
      to |x|. When it completes, the receiver will be passed an lvalue reference
      to that |x|.
    - If given an r-value of type `T`, then `just` will hold a copy of that value.
    - If `just` is copy-connected, then the held `T` will be copied into the
      operation state as a `T`. (Copy-connecting a `just` requires that `T` be
      copy-constructible.)
    - If `just` is move-connected, then the held `T` will be moved into the
      operation state as a `T`.

  .. hint::

    Beware that passing an :term:`lvalue` via CTAD to `just()` will cause the
    `just` to hold a reference to that lvalue::

      auto foo() {
        std::string h = "Hello!";
        return just(h); // UB!! The returned just() holds a reference to `h`!
      }

    If you have an lvalue that you want to give ownership to a `just`, use
    `std::move` to give the object to the `just`::

      std::string some_string = xyz();
      auto J = just(std::move(some_string));  // J now owns the `some_string`

    If you want to give `just` an independent copy without moving-from the
    object, use ``auto()`` to force a copy::

      std::string some_string = xyz();
      auto J = just(auto(some_string));  // J owns a copy of `some_string`


C API Compatibilty
##################

The `nanosender` APIs are not part of the public API, but are used to implement
it.


`unique_emitter` is a `nanosender`
**********************************

The `unique_emitter` type acts as a `nanosender` which sends an
`emitter_result` value.

When a :expr:`nanoreceiver_of<emitter_result>` is connected a `unique_emitter`,
the C++ receiver type will be converted to a `unique_handler` using
`as_handler`.


`unique_handler` is a :expr:`nanoreceiver_of<emitter_result>`
*************************************************************

A `unique_handler` object can be used as a receiver of `emitter_result` via
its `unique_handler::operator()`.


Adaptors
********

.. function:: unique_handler as_handler(mlib::allocator<> a, auto&& recv)

  Creates a `unique_handler` |H| from a C++ nanoreceiver.

  :param a: An allocator for the handler's state. Only used if `recv` cannot be
    inlined within a box.
  :param recv: A nanoreceiver. Must be a receiver for either an `emitter_result`
    or a `result\<unique_box>`.
  :return: A new `unique_handler` |H|

  When the handler |H| is `completed <amongoc_handler_complete>`, the status and
  value are bound in either an `emitter_result` or a `result\<unique_box>`
  (whichever is expected by `recv`) and then passed to `recv`.


.. function::
  unique_emitter as_emitter(mlib::allocator<> a, nanosender auto&& snd)

  Create a `unique_emitter` |E| from a C++ nanosender.

  :param a: An allocator for the emitter's state. Only used if `snd` cannot be
    inlined within a box.
  :param snd: A `nanosender`. Must send an `emitter_result`.
  :return: A new `unique_emitter` |E|.

  When the sender `snd` completes with an `emitter_result` |R|, the
  `status <emitter_result::status>` and `value <emitter_result::value>` from |R|
  will be passed to `amongoc_handler_complete`.
