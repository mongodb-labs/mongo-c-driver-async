######################
Type: `amongoc_client`
######################

.. |this-header| replace:: :header-file:`amongoc/client.h`
.. header-file:: amongoc/client.h

  Contains the following:

  - `amongoc_client`


.. struct:: amongoc_client

  A type that acts as an opaque handle to a MongoDB client.

  :header: |this-header|


.. function::
  amongoc_emitter [[type(amongoc_client)]] amongoc_client_connect(amongoc_loop* loop, const char* name, const char* svc)

  Asynchronously connect a new client to the remote server at the given location.

  :param loop: The event loop on which to connect.
  :param name: The hostname to connect to.
  :param svc: The service (port number) to connect to.
  :return: An `amongoc_emitter` that will resolve with an `amongoc_client`
  :allocation: Memory allocation is performed by `loop`.
  :header: |this-header|

  The name resolution semantics for `name` and `svc` entirely depend on the
  semantics provided by `loop`.


.. function::
  void amongoc_client_destroy(amongoc_client [[transfer]])

  Destroy the client object, releasing any resources it may have acquired.

  :header: |this-header|

  If the given client is null (i.e. zero-initialized), then this function has no
  effect.


.. function::
  amongoc_emitter [[type(bson_mut)]] amongoc_client_command(amongoc_client cl, bson_view doc)

  Issue a client command against the MongoDB server connected with `cl`

  :param cl: An `amongoc_client` that was connected with `amongoc_client_connect`.
  :param doc: View of a BSON document that defines the command. The document
    data is copied into the emitter so it does not need to be persisted beyond
    this call.
  :return: An `amongoc_emitter` that resolves with a `bson_mut` containing the
    server's response message.
  :allocation: Memory allocation is performed by the event loop of the client.
  :header: |this-header|
