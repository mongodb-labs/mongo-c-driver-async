##################################################
Header: |this-header| -- Asynchronous Utility APIs
##################################################

.. |this-header| replace:: :header-file:`amongoc/async.h`
.. header-file:: amongoc/async.h

  This header contains miscellaneous asynchronous utilities and building blocks.

.. |opstate-alloc| replace:: The allocator for the emitter and operation state.
.. |uses-loop-alloc| replace:: This operation uses the event loop's allocator.


Functions
#########

.. function::
  amongoc_emitter [[type(U)]] amongoc_then(\
      amongoc_emitter [[transfer, type(T)]] em, \
      amongoc_async_flags flags, \
      mlib_allocator alloc, \
      amongoc_box [[transfer, type(UserData)]] userdata, \
      amongoc_then_transformer [[type(User=UserData, In=T, Out=U)]] tr)

  Connect a continuation to an `amongoc_emitter`.

  :param em: |attr.transfer| The emitter to be composed.
  :param flags: Options for the behavior of the transformed emitter.
  :param alloc: |opstate-alloc|
  :param userdata: |attr.transfer| Arbitrary userdata that will be forwarded to `tr`.
  :param tr: The transformer callback. See `amongoc_then_transformer` for information.
  :return: A new `amongoc_emitter` |R|
  :header: |this-header|

  When the emitter `em` resolves, its result status and value will be given to
  `tr`. The return value from `tr` will become the new result value of the
  returned emitter |R|. The transform function `tr` may also modify the final
  status to change the result status of |R|.

  .. hint::

    If you need to start another asynchronous operation to define the new
    result, use `amongoc_let` instead. `amongoc_then` is only for synchronous
    continuations.


.. function::
  amongoc_emitter [[type(U)]] amongoc_let(\
      amongoc_emitter [[transfer, type(T)]] em, \
      amongoc_async_flags flags, \
      mlib_allocator alloc, \
      amongoc_box [[transfer, type(UserData)]] userdata, \
      amongoc_let_transformer [[type(User=UserData, In=T, Out=U)]] tr)

  Connect a continuation that defines a new asynchronous operation to be
  launched immediately upon completion of the input operation.

  :param em: |attr.transfer| The input emitter to be composed.
  :param flags: Options for the behavior of the transformed emitter.
  :param alloc: |opstate-alloc|
  :param userdata: |attr.transfer| Arbitrary userdata that is forwarded to `tr`.
  :param tr: The transformer callback. See `amongoc_let_transformer` for more information.
  :return: A new `amongoc_emitter` |R|.
  :header: |this-header|

  When the input emitter `em` resolves, the transformer function `tr` will be
  called to obtain a new emitter :math:`e'`. The new emitter :math:`e'` will be
  launched immediately, and its result will be used as the result of the
  composed emitter |R|.

  Use this function when the initiation of an asynchronous operation depends on
  the result of another asynchronous operation.


.. function:: amongoc_emitter [[type(T)]] amongoc_just(amongoc_status st, amongoc_box [[transfer, type(T)]] value, mlib_allocator alloc)

  Create an emitter that will resolve immediately with the given status and
  result value.

  :param st: The result status.
  :param value: |attr.transfer| The result value.
  :param alloc: |opstate-alloc|
  :return: A new `amongoc_emitter` |R| whose result status will be `st` and
    result value will be `value`
  :header: |this-header|

  .. note::

    The returned emitter here is not tied to any event loop, and it will call
    `amongoc_handler_complete` *immediately* within the call to `amongoc_start` invoked
    on its associated `amongoc_operation`.

  .. note::

    This operation does not support cancellation and will never encounter an
    error during its completion. The status `st` and result `value` will always
    be sent to the handler.


.. function::
  amongoc_emitter [[type(T)]] amognoc_then_just( \
      amongoc_emitter [[transfer]] em, \
      amongoc_async_flags flags, \
      amongoc_status st, \
      amongoc_box [[transfer, type(T)]] value, \
      mlib_allocator alloc)

  Create a continuation that replaces an emitter's result value with the given
  status `st` and result `value`.

  :param em: The input operation to be modified.
  :param flags: Behavior control flags.
  :param st: The new status of the operation.
  :param value: The new value of the operation.
  :param alloc: |opstate-alloc|
  :return: A new emitter |R| for the composed operation.
  :header: |this-header|

  Upon successful completion, the result value from `em` will be immediately
  destroyed and the emitter |R| will resolve with `st` and `value`. Upon
  failure (i.e. if `flags` specify a different behavior), then the `value`
  object will be destroyed and the error will be propagated.


.. function:: amongoc_emitter [[type(nil)]] amongoc_schedule(amongoc_loop* loop)

  Create an emitter that will resolve within the given event loop as soon as
  possible.

  :param loop: The event loop that will invoke `amongoc_handler_complete` on the handler.
  :return: An `amongoc_emitter` for the schedule operation. It will always emit
    `amongoc_nil` to its handler.
  :allocation: |uses-loop-alloc|
  :header: |this-header|

  When connected to a handler and the resulting operation is started, the
  handler for the operation will be enqueued with the event loop using
  `amongoc_loop_vtable::call_soon`.


.. function:: amongoc_emitter [[type(nil)]] amongoc_schedule_later(amongoc_loop* loop, std::timespec duration)

  Schedule a completion after `duration` has elapsed.

  :param loop: The event loop that controls the timer and will complete the operation.
  :param duration: The amount of time to delay the operation.
  :return: An `amongoc_emitter` that resolves with `amongoc_nil` upon success
    after `duration` has elapsed. **Note** that the operation may resolve earlier
    in case of error or cancellation.
  :allocation: |uses-loop-alloc|
  :header: |this-header|


.. function::
  amongoc_emitter amongoc_timeout(amongoc_loop* loop, amongoc_emitter [[transfer]] em, std::timespec duration)

  Attach a timeout to the asynchronous operation `em`.

  :param loop: The event loop that will handle the timeout.
  :param em: |attr.transfer| An `amongoc_emitter` for an operation that will be
    cancelled if it exceeds the duration of the timeout.
  :param duration: The timeout duration.
  :return: A new emitter |R| representing the operation with the timeout.
  :allocation: |uses-loop-alloc|
  :header: |this-header|

  **If the timeout is hit** before the `em` resolves, then `em` will be
  cancelled immediately. After cancellation completes, |R| will resolve with a
  status of ``ETIMEDOUT`` and value `amongoc_nil`.

  If the timeout does not hit before `em` resolves, then the result status and
  value from `em` will be emitted by |R|.

  .. important::

    If the operation `em` does not properly support cancellation, then the
    timeout cannot work, as the composed operation must wait for the `em`
    operation to resolve after the cancellation has been requested. (All default
    operations provided by |amongoc| support cancellation, unless otherwise
    specified.)


.. function::
  amongoc_emitter amongoc_alloc_failure()

  Obtain an emitter that immediately resolves with a generic ``ENOMEM`` for its
  completion status. This may be returned by any API returning an
  `amongoc_emitter` that requires memory allocation.

  :allocation: This function and the returned emitter do not allocate memory.
  :header: |this-header|


.. function:: amongoc_operation amongoc_tie(amongoc_emitter [[transfer, type(T)]] em, amongoc_status* [[storage]] st, amongoc_box* [[storage, type(T)]] value)

  Create an `amongoc_operation` object that captures the emitter's results in
  the given locations.

  :param em: |attr.transfer| The operation to be executed.
  :param st: |attr.storage| Pointer to an `amongoc_status` object to receive the
    emitter's final status. If ``NULL``, the status will be discarded.
  :param value: |attr.storage| Pointer to an `amongoc_box` object that will hold
    the emitter's result. If ``NULL``, the emitter's result value will be
    destoyed instead of stored.
  :allocation: Memory allocation is controlled by the emitter.
  :header: |this-header|

  .. important::

    It is essential that the two pointed-to locations be alive and valid until
    the returned `amongoc_operation` completes or is destroyed.


.. function:: amongoc_operation amongoc_detach(amongoc_emitter [[transfer]] em)

  Create a "detached" operation for an emitter.

  :param em: The emitter to be detached.
  :allocation: Memory allocation is controlled by the emitter.
  :header: |this-header|

  The returned operation object can be launched with `amongoc_start`. The final
  result value from the emitter `em` will be immediatly destroyed when it
  resolves.

  .. hint::

    This function is equivalent to :expr:`amongoc_tie(em, nullptr, nullptr)`


Types
#####

.. type::
  amongoc_then_transformer = \
      amongoc_box [[type(Out)]] (*)\
          (amongoc_box [[transfer, type(User)]] userdata, \
           amongoc_status* st, \
           amongoc_box [[transfer, type(In)]] value)
  amongoc_let_transformer = \
      amongoc_emitter [[type(Out)]] (*)\
          (amongoc_box [[transfer, type(User)]] userdata, \
           amongoc_status st, \
           amongoc_box [[transfer, type(In)]] value)

  The function pointer types used to transform an emitter result for
  `amongoc_then` and `amongoc_let`, respectively. The following parameters are
  used:

  :header: |this-header|

  `amongoc_box` |attr.transfer| :doc-attr:`[[type(User)]] <[[type(T)]]>` ``userdata``
    The ``userdata`` value that was given to `amongoc_then`/`amongoc_let`.

    .. note::

      If the transformer function is not called but the associated emitter is
      destroyed or resolves in another way, then the ``userdata`` will be
      destroyed automatically using `amongoc_box_destroy`. For this reason: Be
      sure to attach a destructor to your userdata, since it may need to be
      cleaned up by code that is outside of your control.

  `amongoc_status` ``st`` / `amongoc_status` ``*st``
    The resolve status of the input emitter.

    For `amongoc_then`, this is a non-null pointer to a status object that may
    be modified by the transformer. The modified status will then be used as the
    result status of the composed emitter.

  `amongoc_box` |attr.transfer| :doc-attr:`[[type(In)]] <[[type(T)]]>` ``value``
    The result value that was emitted by the input emitter.

  The ``then`` transformer is expected to return an `amongoc_box`, while the
  ``let`` transformer must return an `amongoc_emitter`. For an explanation of
  this behavior, refer to `amongoc_then` and `amongoc_let`, respectively.


Constants
#########

.. enum:: amongoc_async_flags

  Flags to control the behavior of `amongoc_then` and `amongoc_let`

  :header: |this-header|

  .. enumerator:: amongoc_async_default

    No special behavior.

  .. enumerator:: amongoc_async_forward_errors

    ..
      XXX: For some reason, Sphinx will occassionally fail to resolve references
      to this enumerator. I'm not sure what causes this, but rebuilding the docs will
      usually succeed

    If this flag is specified and the input emitter resolves with an error
    status (checked using `amongoc_is_error`), then the transformation function
    will be skipped and the error from the emitter will be immediately forwarded
    to the next handler.
