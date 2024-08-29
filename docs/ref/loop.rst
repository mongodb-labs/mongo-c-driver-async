####################
Type: `amongoc_loop`
####################

.. header-file:: amongoc/loop.h

  - `amongoc_loop`
  - `amongoc_loop_vtable`

.. header-file:: amongoc/default_loop.h

  - `amongoc_default_loop_init`
  - `amongoc_default_loop_destroy`
  - `amongoc_default_loop_run`

.. struct:: amongoc_loop

  An event loop for the amongoc library.

  :header: :header-file:`amongoc/loop.h`

  .. member:: amongoc_box userdata

    The arbitrary user data associated with the event loop.

  .. member:: amongoc_loop_vtable vtable

    The virtual method table for the event loop


.. function:: mlib_allocator amongoc_loop_get_allocator(const amongoc_loop* loop)

  Get the `mlib_allocator` associate with an event loop.

  :param loop: An initialized event loop
  :header: :header-file:`amongoc/loop.h`

  Calls `amongoc_loop_vtable::get_allocator` to look for the allocator. If the
  loop does not provide an allocator the this function will return
  `amongoc_default_allocator`.


.. struct:: amongoc_loop_vtable

  :header: :header-file:`amongoc/loop.h`

  This struct should be filled out with function pointers that implement the associated
  asynchronous operation functions. The "methods" documented below correspond to the
  function pointers that need to be provided.

  .. rubric:: Function Pointers

  .. function:: void call_soon(amongoc_loop* self, amongoc_status st, amongoc_box arg, amongoc_handler hnd)

    Register the given `amongoc_handler` to be completed by the event loop
    without any blocking or associated delay.

    :param st: The `amongoc_status` to be given to `amongoc_handler_complete`
    :param arg: The value that will be passed as the ``res`` argument for `amongoc_handler_complete`
    :param hnd: The `amongoc_handler` that will be run.

    The event loop **must** invoke the handler as-if by:
    :expr:`amongoc_handler_complete(hnd, st, arg)`.


  .. function:: void call_later(amongoc_loop* self, std::timespec duration, amongoc_box arg, amongoc_handler hnd)

    Register an `amongoc_handler` to be completed after a set delay.

    :param duration: The delay after which the operation should be completed.
    :param arg: The result value that should be passed to the handler when it is
      completed.
    :param hnd: The handler that should be completed.

    The event loop should perform
    :expr:`amongoc_handler_complete(hnd, amongoc_okay, arg)` no sooner than
    after `duration` amount of time has elapsed since the call to `call_later`.

    If the event loop needs to invoke the handler earlier due to errors or
    cancellation, then a non-zero `amongoc_status` should be given to
    `amongoc_handler_complete` to notify the handler that its duration may not
    have elapsed.

  .. function:: void getaddrinfo(amongoc_loop* self, const char* name, const char* svc, amongoc_handler on_resolve)

    Initiate a name-resolution operation.

    :param name: The name that should be resolve (e.g. a domain name or IP address)
    :param svc: Hint for the service to be resolved (e.g. a port number or protocol name)
    :param on_resolve: The handler to be invoked when resolution completes.

    Upon success, the result value given to `amongoc_handler_complete` will be treated
    as an opaque object containing the resolved results, to be used with
    `tcp_connect`.

  .. function:: void tcp_connect(amongoc_loop* self, amongoc_view addrinfo, amongoc_handler on_connect)

    Initiate a TCP connect operation.

    :param addrinfo: The result object that was given to the ``on_resolve`` handler
      from a successful completion of a `getaddrinfo` operation.
    :param on_connect: The handler to be invoked when the operation completes.

    Upon success, the result value to `amongoc_handler_complete` will be treated as an
    opaque object representing the live TCP connection. The connection object
    may be destroyed at any time via `amongoc_box_destroy`, which should release
    any associated resources and close the connection.

  .. function:: void tcp_write_some(amongoc_loop* self, amongoc_view conn, const char* data, std::size_t len, amongoc_handler on_write)

    Write some data to a TCP connection.

    :param conn: The connection object that resulted from `tcp_connect`.
    :param data: Pointer to the beginning of a data buffer to be written to the socket.
    :param len: The length of the buffer pointed-to by `data`.
    :param on_write: The handler for the operation.

    This function should write at-most `len` bytes from `data` into the TCP
    connection referenced by `conn`. The result value given to
    `amongoc_handler_complete` must be a `std::size_t` value equal to the number of
    bytes that were successfully written to the socket.

  .. function:: void tcp_read_some(amongoc_loop* self, amongoc_view conn, char* data, std::size_t maxlen, amongoc_handler on_read)

    Read some data from a TCP connection.

    :param conn: The connection object that came from `tcp_connect`.
    :param data: Pointer to the beginning of a mutable buffer where data can be written.
    :param maxlen: The maximum number of bytes that can be written to `data`
    :param on_read: A handler for the operation.

    This function should read at-most `maxlen` bytes from the TCP connection
    `conn` into the buffer `data`. The result given to `amongoc_handler_complete` must
    be a `std::size_t` value equal to the number of bytes that were read from
    the socket.

  .. function:: mlib_allocator get_allocator(const amongoc_loop* self) [[optional]]

    Obtain the `mlib_allocator` associated with the event loop. Various
    library components will call this function to perform dynamic memory
    management for objects associated with the event loop.

    .. note::

      Do not call this method directly. Use `amongoc_loop_get_allocator`.


The Default Event Loop
######################

amongoc provides a default event loop in ``<amongoc/default_loop.h>``. This is a
simple single-threaded event loop that provides all the base operations.

.. function:: void amongoc_default_loop_init(amongoc_loop* [[storage]] loop)

  Initialize a new default event loop.

  :param loop: Pointer to storage for a new `amongoc_loop`
  :header: :header-file:`amongoc/default_loop.h`

  Each call to this function must be followed by a later call to
  `amongoc_default_loop_destroy`.

.. function:: void amongoc_default_loop_destroy(amongoc_loop* loop)

  Destroy a default event loop.

  :param loop: Pointer to a loop that was previously initiatlized using
    `amongoc_default_loop_init`.
  :header: :header-file:`amongoc/default_loop.h`

.. function:: void amongoc_default_loop_run(amongoc_loop* loop)

  Execute the default event loop.

  :param loop: A loop constructed with `amongoc_default_loop_init`.
  :header: :header-file:`amongoc/default_loop.h`

  This function will run all pending asynchronous operations until there is no
  more work to be executed in the event loop.
