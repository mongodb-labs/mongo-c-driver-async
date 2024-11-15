#############
Operation API
#############

.. header-file::
  amongoc/operation.h
  amongoc/operation.hpp

  :term:`Operation state` utilities.


Types
#####

C Types
*******

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


C++ Types
*********

.. class:: amongoc::unique_operation

  Provides move-only ownership semantics around an `amongoc_operation`,
  preventing programmer error and ensuring destruction when the operation leaves
  scope.

  :header: :header-file:`amongoc/operation.hpp`

  .. function:: unique_operation(amongoc_operation&&)

    Take ownership of the given `amongoc_operation` object.

  .. function:: ~unique_operation()

    Calls `amongoc_operation_delete` on the operation.

  .. function:: template <typename F> unique_operation from_starter(unique_handler&& h, F&& fn)

    Create an operation object based on a starter function `fn` with an
    associated handler `h`.

    :param h: The handler object to be attached to the operation.
    :param fn: The starter invocable that will be invoked whent he operation is
      is started. An lvalue reference to the stored handler will be passed as
      the sole argument to the starter function.
    :allocation: The operation state will be allocated using
      :ref:`the allocator associated with the handler <handler.allocator>` `h`.

  .. function:: void start()

    :C API: `amongoc_start`

  .. function:: amongoc_operation release() &&

    Relinquish ownership of the wrapped operation and return it to the caller.
    This function is used to interface with C APIs that want to |attr.transfer|
    an `amongoc_operation`.


Functions & Macros
##################

.. function:: void amongoc_start(amongoc_operation* op)

  Launch the operation defined by the given operation object.

  :header: :header-file:`amongoc/operation.h`

.. function:: void amongoc_operation_delete(amongoc_operation [[transfer]] op)

  Destroy an operation object.

  :header: :header-file:`amongoc/operation.h`

  .. note::

    It is very important that the associated operation is *NOT* in-progress!

