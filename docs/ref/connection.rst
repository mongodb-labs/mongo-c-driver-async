##########################
Type: `amongoc_connection`
##########################

.. |this-header| replace:: :header-file:`amongoc/client.h`
.. header-file:: amongoc/client.h

  Contains the following:

  - `amongoc_connection`


.. struct:: amongoc_connection

  A type that acts as an opaque handle to a MongoDB connection.

  :header: |this-header|


.. function::
  amongoc_emitter [[type(amongoc_connection)]] amongoc_conn_connect(amongoc_loop* loop, const char* name, const char* svc)

  Asynchronously connect to the remote server at the given location.

  :param loop: The event loop on which to connect.
  :param name: The hostname to connect to.
  :param svc: The service (port number) to connect to.
  :return: An `amongoc_emitter` that will resolve with an `amongoc_connection`
  :allocation: Memory allocation is performed by `loop`.
  :header: |this-header|

  The name resolution semantics for `name` and `svc` entirely depend on the
  semantics provided by `loop`.


.. function::
  void amongoc_conn_destroy(amongoc_connection [[transfer]])

  Destroy the connection object, releasing any resources it may have acquired.

  :header: |this-header|

  If the given connection is null (i.e. zero-initialized), then this function
  has no effect.


.. function::
  amongoc_emitter [[type(bson_mut)]] amongoc_conn_command(amongoc_connection conn, bson_view doc)
  amongoc_emitter [[type(bson_mut)]] amongoc_conn_command_nocopy(amongoc_connection conn, bson_view doc)

  Issue a command against the MongoDB server connected with `conn`

  :param conn: An `amongoc_connection` that was connected with `amongoc_conn_connect`.
  :param doc: View of a BSON document that defines the command. The ``_nocopy``
    version of this call will not copy the data into the emitter, requiring that
    the caller ensure the data viewd by `doc` remains valid until the operation
    completes.
  :return: An `amongoc_emitter` that resolves with a `bson_mut` containing the
    server's response message.
  :allocation: Memory allocation is performed by the event loop of the connection.
  :header: |this-header|
