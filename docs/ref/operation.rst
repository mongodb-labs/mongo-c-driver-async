###############################
Header: ``amongoc/operation.h``
###############################

.. header-file:: amongoc/operation.h

  Operation state utilities.

.. struct:: amongoc_operation

  Represents a composed operation ready to be launched. This is created by connecting
  an `amongoc_emitter` to an `amongoc_handler`.

  :header: :header-file:`amongoc/operation.h`

  .. member:: amongoc_box userdata

    Arbitrary data associated with the operation.

  .. member:: amongoc_handler handler

    Storage for the handler associated with the operation.

  .. member:: amongoc_start_callback start_callback

    The start callback for the operation. Do not call this directly. Prefer to
    use `amongoc_start` when launching an operation.

  .. function:: as_unique() &&

    Move the operation into a new `amongoc::unique_operation` object.

.. type:: amongoc_start_callback = void(*)(amongoc_operation* self)

  The operation-launching callback for the operation.

.. function:: void amongoc_start(amongoc_operation* op)

  Launch the asynchronous operation defined by the given operation object.

.. function:: void amongoc_operation_destroy(amongoc_operation [[transfer]] op)

  Destroy an asynchronous operation object.

  .. note::

    It is very important that the associated operation is *NOT* in-progress!

.. namespace:: amongoc

.. class:: unique_operation

  Provides move-only ownership semantics around an `amongoc_operation`,
  preventing programmer error and ensuring destruction when the operation leaves
  scope.

  .. function:: unique_operation(amongoc_operation&&)

    Take ownership of the given `amongoc_operation` object.

  .. function:: ~unique_operation()

    Calls `amongoc_operation_destroy` on the operation.

  .. function:: template <typename F> unique_operation from_starter(cxx_allocator<> a, unique_handler&& h, F fn)

    Create an operation object based on a starter function `fn` with an
    associated handler `hnd`. The function object must be invocable with a
    single :expr:`amongoc_operation&` argument, and will be called when the
    `start`/`amongoc_start` function is called on the operation. The given
    handler `hnd` will be attached to the returned operation in the
    `amongoc_operation::handler` field.

  .. function:: void start()

    :C API: `amongoc_start`

  .. function:: amongoc_operation [[transfer]] release()

    Relinquish ownership of the wrapped operation and return it to the caller.
    This function is used to interface with C APIs that want to |attr.transfer|
    an `amongoc_operation`.

.. namespace:: 0
