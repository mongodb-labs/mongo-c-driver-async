#############################
Header: ``amongoc/handler.h``
#############################

.. |this-header| replace:: :header-file:`amongoc/handler.h`
.. header-file:: amongoc/handler.h


Handlers
########

.. struct:: amongoc_handler

  A generic asynchronous-operation-completion-handler type, with some additional
  controls for operation cancellation.

  :header: |this-header|

  .. type:: T

    (Exposition-only for |attr.type|) The result type expected by the handler
    for `amongoc_handler_vtable::complete`. A handler should also accept
    `amongoc_nil` in the case that the completion status is non-successful.

    When an `amongoc_handler` return value or parameter is annotated with
    |attr.type|, the attribute specifies this `T` type.

  .. type:: UserData

    (Exposition-only for |attr.type|) The type contained within `userdata`.

  .. member:: amongoc_box [[type(UserData)]] userdata

    The data associated with this handler object.

  .. member:: amongoc_handler_vtable const* vtable

    Pointers to methodd for the handler object.

  .. function:: as_unique() &&

    (C++) Move the handler into an `amongoc::unique_handler` to be managed automatically

  .. function:: void complete(amongoc_status st, amongoc::unique_box&& value)

    :C API: `amongoc_handler_complete`


.. struct:: amongoc_handler_vtable

  The set of function pointers that handle actions for an `amongoc_handler`. The
  "member functions" described here are actually function pointer member
  variables that should be filled-out by the user.

  :header: |this-header|

  .. function:: void complete(amongoc_handler* self, amongoc_status st, amongoc_box [[transfer, type(T)]] result)

    The function that handles the completion of an associated operation. This
    function is **required** to be defined for all handler objects.

    :param self: Pointer to the handler object being completed.
    :param st: The status of the operation.
    :param result: |attr.transfer| The result value of the operation.

    .. seealso:: `amongoc_handler_complete`

  .. function:: amongoc_box register_stop(amongoc_handler const* self, void* [[type(V)]] userdata, void(*callback)(void* [[type(V)]])) [[optional]]

    Register a stop callback with the handler.

    :param self: Pointer to the handler object.
    :param userdata: An arbitrary pointer that should be used when the `callback` is invoked
      during operation cancellation.
    :param callback: The callback function that should be invoked to cancel the associated
      operation.
    :return: A cookie value that when destroyed will disconnect the stop callback.

    This is used by asynchronous operations to implement cancellation. Before an
    operation begins, it *may* call `register_stop` to register a callback that
    should be invoked to cause the cancellation of the associated operation.

    If this callback is not set for a handler, then cancelling the operation
    will not be possible.

    .. note:: Instead of calling this API function directly, use `amongoc_register_stop` or
        `amongoc::unique_handler::register_stop`

  .. function:: mlib_allocator get_allocator(amongoc_handler const* self, mlib_allocator dflt) [[optional]]

    Obtain an allocator associated with this handler.

    :param self: Pointer to the handler object
    :param dflt: The default allocator that should be returned if the handler
      does not provide an allocator.
    :return: The function must return a valid allocator that is associated with the
      handler object.

    If this function is omitted, then the handler will not have an associated
    allocator.

    .. seealso:: :ref:`handler.allocator`.

    .. note:: Don't call this directly. Use: `amongoc_handler_get_allocator`


.. function:: void amongoc_handler_complete(amongoc_handler* [[type(T)]] hnd, amongoc_status st, amongoc_box [[transfer, type(T)]] res)

  Invoke the completion callback for the handler.

  :C++ API: `amongoc::unique_handler::complete`
  :param hnd: The handler to be completed.
  :param st: The status of the operation.
  :param res: |attr.transfer| The final result value for the operation. Even though
    the parameter is marked with |attr.type| that matches the handler `hnd`, it is
    likely that he handler must also accept `amongoc_nil` in the case that `st`
    represents failure. Exceptions to this rule will be documented.
  :header: |this-header|

  .. important:: A handler object should be completed *at most once*.


.. function::
  amongoc_box amongoc_register_stop(const amongoc_handler* h, void* [[type(V)]] userdata, void(*callback)(void* [[type(V)]]))

  Register a stop callback with the handler. This function has no effect if
  `amongoc_handler_vtable::register_stop` is not set.

  :C++ API: `amongoc::unique_handler::register_stop`
  :param h: The handler object with which to register the callback
  :param userdata: Arbitrary pointer that will be passed to `callback` at a later point.
  :param callback: The callback function that should cancel the associated operation.
  :return: An `amongoc_box` cookie object that when destroyed will unregister the
    callback from the handler. The type of value contained by this box is
    unspecified.
  :header: |this-header|

.. function::
  mlib_allocator amongoc_handler_get_allocator(amongoc_handler const* h, mlib_allocator dflt)

  Obtain the allocator associated with an handler object.

  :C++ API: `amongoc::unique_handler::get_allocator`
  :param h: Pointer to an `amongoc_handler`
  :param dflt: The fallback allocator to be returned if `h` does not have an
    associated allocator.
  :header: |this-header|

  .. seealso:: :ref:`handler.allocator`

.. function:: void amongoc_handler_delete(amongoc_handler [[transfer]] h)

  Destroy a handler object.

  :C++ API: Use `amongoc::unique_handler`
  :header: |this-header|

  .. note:: Don't call this function on a handler that has been transferred
    elsewhere. This function will usually only be needed when a handler
    is unused, otherwise it will be the responsibility of an `amongoc_operation`
    to destroy the handler.


C++ APIs
########

.. rubric:: Namespace ``amongoc``
.. namespace:: amongoc

.. class:: unique_handler

  Provides a move-only wrapper around `amongoc_handler`, preventing programmer
  error and ensuring desctruction of the associated object.

  :header: |this-header|

  .. function:: handler_stop_token get_stop_token() const

    Obtain a stop token associated with the handler object.

  .. function:: allocator<> get_allocator() const

    Obtain the allocator associated with the handler.

    :C API: `amongoc_handler_get_allocator`

    .. seealso:: :ref:`handler.allocator`

  .. function:: static unique_handler from(allocator<> a, auto&& fn)

    Create a :class:`unique_handler` from an invocable object. The object `fn`
    must be invocable with an `emitter_result` argument.

    :param a: An allocator used to allocate the handler's state, and may be
      associated with the new handler.
    :param fn: The invocable object that will be called as the completion
      callback for the handler.
    :allocation: Allocation of the handler's state data will be performed using
      `a`. **If** the invocable `fn` has an associated `mlib::allocator` |A'|,
      then the returned handler will use |A'| as its associated allocator,
      otherwise it will use `a`.

    .. important::

      Note that the `amongoc_handler_vtable::register_stop` function will not be
      defined, so the new handler will not have cancellation support.

  .. function:: void complete(amongoc_status st, unique_box&& value)

    :C API: `amongoc_handler_complete`

  .. function:: unique_box register_stop(void* [[type(V)]] userdata, void(*callback)(void* [[type(V)]]))

    :C API: `amongoc_register_stop`

    .. warning::

      The returned box must be destroyed before the associated handler is
      destroyed: The box may contain state that refers to the handler object.

  .. function:: amongoc_handler release() &&

    Relinquish ownership of the managed object and return it to the caller. This
    function is used to interface with C APIs that |attr.transfer| an
    `amongoc_handler`.

  .. function:: void operator()(emitter_result&& r)

    Invokes :cpp:`complete(r.status, std::move(r).value)`


.. class:: handler_stop_token

  Implements a *stopptable token* type for use with an `amongoc_handler`. This
  type is compatible with the standard library stoppable token interface.

  :header: |this-header|

  .. function:: handler_stop_token(const amongoc_handler&)

    Create a stop token that is bound to the given handler.

  .. function:: bool stop_possible() const

    Return ``true`` if the associated handler has stop registration methods.

  .. function:: bool stop_requested() const

    Always returns ``false`` (this stop token only supports callback-based stopping)

  .. class:: template <typename F> callback_type

    The stop-callback type to be used with this stop token.

    .. function:: callback_type(handler_stop_token, F&& fn)

      Construct the stop callback associated with this token, which will invoke
      `fn` when a stop is requested

    .. function:: ~callback_type()

      Disconnects the stop callback from the stop state.


.. namespace:: 0

.. _handler.allocator:

The Handler-Associated Allocator
################################

An `amongoc_handler` may have an associated allocator |A|. This can be obtained
using `amongoc_handler_get_allocator`, and is customizable by providing the
`amongoc_handler_vtable::get_allocator` function pointer on a handler.

The associated allocator |A| will be used to allocate transient operation state
for the operation to which it is bound. Note that an operation may use a
different allocator for different aspects of its state depending on how the
associated `amongoc_emitter` was constructed. To ensure that all aspects of an
operation use the same allocator |A|, use |A| when creating handlers and use the
same |A| when creating emitters.

High-level APIs will often deal with the creation of handlers, and will accept
allocators in their interface to be bound with any handlers that they create.