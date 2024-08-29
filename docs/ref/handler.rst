#############################
Header: ``amongoc/handler.h``
#############################

.. header-file:: amongoc/handler.h

.. struct:: amongoc_handler

  A generic asynchronous-operation-completion-handler type, with some additional
  controls for operation cancellation.

  :header: :header-file:`amongoc/handler.h`

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

    Move the handler into an `amongoc::unique_handler` to be managed automatically

  .. function:: void complete(amongoc_status st, amongoc::unique_box&& value)

    :C API: `amongoc_handler_complete`


.. struct:: amongoc_handler_vtable

  The set of function pointers that handle actions for an `amongoc_handler`. The
  "member functions" described here are actually function pointer member
  variables that should be filled-out by the user.

  :header: :header-file:`amongoc/handler.h`

  .. function:: void complete(amongoc_view [[type(UserData)]] userdata, amongoc_status st, amongoc_box [[transfer, type(T)]] result)

    The function that handles the completion of an associated operation. This
    function is **required** to be defined for all handler objects.

    :param userdata: View of the `amongoc_handler::userdata` value for the associated handler.
    :param st: The status of the operation.
    :param result: |attr.transfer| The result value of the operation.

    .. seealso:: `amongoc_handler_complete`

  .. function:: amongoc_box register_stop(amongoc_view [[type(UserData)]] hnd_userdata, void* [[type(V)]] userdata, void(*callback)(void* [[type(V)]])) [[optional]]

    Register a stop callback with the handler.

    :param hnd_userdata: An `amongoc_view` derived from the `amongoc_handler::userdata` value.
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


.. function:: void amongoc_handler_complete(amongoc_handler* [[type(T)]] hnd, amongoc_status st, amongoc_box [[transfer, type(T)]] res)

  Invoke the completion callback for the handler.

  :C++ API: `amongoc::unique_handler::complete`
  :param hnd: The handler to be completed.
  :param st: The status of the operation.
  :param res: |attr.transfer| The final result value for the operation. Even though
    the parameter is marked with |attr.type| that matches the handler `hnd`, it is
    likely that he handler must also accept `amongoc_nil` in the case that `st`
    represents failure. Exceptions to this rule will be documented.
  :header: :header-file:`amongoc/handler.h`

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
  :header: :header-file:`amongoc/handler.h`

  .. warning:: Destroy the returned cookie object *before* calling `amongoc_handler_complete`
    on the handler!

    `amongoc_handler_complete` will destroy the data associated with the handler object,
    which may include data structures that are referred-to by the cookie
    returned by this function.


.. function:: void amongoc_handler_discard(amongoc_handler [[transfer]] h)

  Discard a handler object that will not be used.

  :C++ API: Use `amongoc::unique_handler`
  :header: :header-file:`amongoc/handler.h`

.. rubric:: Namespace ``amongoc``
.. namespace:: amongoc

.. class:: handler_stop_token

  Implements a *stopptable token* type for use with an `amongoc_handler`. This
  type is compatible with the standard library stoppable token interface.

  :header: :header-file:`amongoc/handler.h`

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


.. class:: unique_handler

  Provides a move-only wrapper around `amongoc_handler`, preventing programmer
  error and ensuring desctruction of the associated object.

  :header: :header-file:`amongoc/handler.h`

  .. function:: static unique_handler from(auto&& fn)

    Create a :class:`unique_handler` from an invocable object. The object `fn`
    must be invocable with `amongoc_status` and `unique_box` arguments.

    This function will automatically fill in the `amongoc_handler::userdata` and
    `amongoc_handler::vtable` members on a new handler object.

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

.. namespace:: 0
