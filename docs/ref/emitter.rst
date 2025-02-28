###########
Emitter API
###########

.. header-file::
  amongoc/emitter.h
  amongoc/emitter.hpp

  Contains types and functions for working with :term:`emitters <emitter>`

.. header-file:: amongoc/emitter_result.hpp

  Contains the definition of `amongoc::emitter_result`

Types
#####

C Types
*******

.. struct:: [[zero_initializable]] amongoc_emitter

  `amongoc_emitter` is the core of the |amongoc| asynchronous design.

  :C++ API: `amongoc::unique_emitter`
  :header: :header-file:`amongoc/emitter.h`

  .. type:: T

    (Exposition-only for |attr.type|) The result type of the emitter.

  .. type:: UserData

    (Exposition-only for |attr.type|) The type contained within `userdata`.

  .. seealso:: :doc:`/explain/async-model`

  When an `amongoc_emitter` return value or parameter is annotated with
  |attr.type|, the attribute specifies the type that will be stored in the
  `amongoc_box` result value if the emitter resolves successfully.

  .. member:: amongoc_box [[type(UserData)]] userdata

    Arbitrary data associated with the emitter

  .. member:: amongoc_emitter_vtable const* vtable

    Virtual method table for the emitter.

  .. function:: amongoc::unique_emitter as_unique() &&

    (C++) Move the emitter into an `amongoc::unique_emitter` to be managed
    automatically.

.. struct:: amongoc_emitter_vtable

  Virtual method table to be used with an `amongoc_emitter`. The functions
  described below are function pointers that should be provided by the user.

  :header: :header-file:`amongoc/emitter.h`

  .. function:: amongoc_operation connect(amongoc_box [[transfer, type(UserData)]] userdata, amongoc_handler [[transfer, type(T)]] handler)

    Backing implementation for `amongoc_emitter_connect_handler`. Connects an
    `amongoc_emitter` to the handler `handler`.

    :param userdata: The `amongoc_emitter::userdata` value.
    :param handler: |attr.type| The `amongoc_handler` for the operation.

    :return: A new `amongoc_operation` object.

    .. note::

      It is important that the implementation of `connect` call not initiate any
      asynchronous operations: that must be deferred until the invocation of
      `amongoc_start` on the returned operation object.

      If the returned operation object is destroyed with
      `amongoc_operation_delete` without ever being started with
      `amongoc_start`, then there must be no observable effect.


C++ Types
*********

.. class:: amongoc::unique_emitter

  Provides move-only ownership semantics around an `amongoc_emitter`, preventing
  programmer error and ensuring destruction if the emitter is discarded.

  :header: :header-file:`amongoc/emitter.hpp`

  .. function:: unique_emitter(amongoc_emitter&&)

    Take ownership of the given C emitter object.

  .. function::
    unique_emitter(unique_emitter&&)
    unique_emitter& operator=(unique_emitter&&)

    The :class:`unique_emitter` is a move-only type.

  .. function:: amongoc_emitter release() &&

    Relinquish ownership of the wrapped C emitter and return it to the caller.
    This function is used to interface with C APIs that want to |attr.transfer|
    an `amongoc_emitter`.

  .. function:: template <typename F> static unique_emitter from_connector(mlib::allocator<> alloc, F&& fn)

    Create an emitter from a connector function object.

    :param alloc: The allocator to use for the state.
    :param fn: The object must support a call signature of
      :expr:`unique_operation(unique_handler)`. That is: It must be callable
      with a `unique_handler` argument and return a new `unique_operation`
      object representing the composed operation.
    :return: A new emitter for the connected operation.

  .. function::
    unique_operation connect(unique_handler&& [[type(T)]] hnd) &&

    :C API: `amongoc_emitter_connect_handler`

  .. function::
    template <typename F> \
    unique_operation bind_allocator_connect(allocator<> a, F&& fn) &&

    Creates an invocable object with `unique_handler::from` and calls `connect`
    with that new handler.

    :param a: The allocator to be bound with the new handler. See: :ref:`handler.allocator`
    :param fn: The function that implements the handler callback. Must accept
      an `emitter_result` argument.


.. class:: amongoc::emitter_result

  Encapsulates the pair of status+value when an emitter completes.

  :header: :header-file:`amongoc/emitter_result.hpp`

  .. function::
    emitter_result()  [[1]]
    explicit emitter_result(amongoc_status s)  [[2]]
    explicit emitter_result(amongoc_status s, unique_box&& v)  [[3]]

    Initialize the attrributes of the `emitter_result`

    .. rubric:: Overloads

    1. Initializes `status` with `amongoc_okay` and `value` with `amongoc_nil`.
    2. Initializes `status` with `s` and `value` with `amongoc_nil`.
    3. Initializes `status` with `s` and `value` with `v`

  .. member::
    amongoc_status status
    unique_box value

    The result status and result value for an emitter.


Functions & Macros
##################

.. function:: amongoc_operation amongoc_emitter_connect_handler(amongoc_emitter [[transfer, type(T)]] em, amongoc_handler [[transfer, type(T)]] hnd)

  Connect an emitter with a handler. Calls `amongoc_emitter_vtable::connect`.

  :C++ API: `amongoc::unique_emitter::connect`
  :header: :header-file:`amongoc/emitter.h`

  .. hint::

    This is a very low-level API. In general, users should be composing emitters
    using high-level APIs such as those in the :header-file:`amongoc/async.h`
    header.

.. function:: void amongoc_emitter_delete(amongoc_emitter [[transfer]] em)

  Discard an unused emitter object without connecting it to anything. The
  associated asynchronous operation will never be launched, but associated
  prepared data will be freed.

  :C++ API: Use `amongoc::unique_emitter`
  :header: :header-file:`amongoc/emitter.h`
