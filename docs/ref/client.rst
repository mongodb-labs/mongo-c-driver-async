##########
Client API
##########

.. |this-header| replace:: :header-file:`amongoc/client.h`
.. header-file:: amongoc/client.h

  Contains the following:

  - `amongoc_client`


Types
*****

.. struct:: amongoc_client

  A type that can be used to issue requests to a MongoDB server.

  :header: |this-header|


Functions
*********

.. function::
  amongoc_emitter [[type(amongoc_client*)]] amongoc_client_new(amongoc_loop* loop, __string_convertible uri)

  Asynchronously connect to the remote server at the given location.

  :param loop: The event loop on which to connect.
  :param uri: A connection URI string specifying the connection parameters.
  :return: An `amongoc_emitter` that will resolve with an `amongoc_client` pointer
  :allocation: Memory allocation is performed by `loop`.
  :header: |this-header|


.. function::
  void amongoc_client_delete(amongoc_client* [[transfer, nullable]] cl)

  Destroy the client object, releasing any resources it may have acquired.

  :header: |this-header|


.. function::
  amongoc_emitter [[type(bson_doc)]] amongoc_client_command(amongoc_client* cl, bson_view doc)
  amongoc_emitter [[type(bson_doc)]] amongoc_client_command_nocopy(amongoc_client* cl, bson_view doc)

  Issue a command against the MongoDB server connected with `cl`

  :param cl: An `amongoc_client` that was created with `amongoc_client_new`.
  :param doc: View of a BSON document that defines the command. The ``_nocopy``
    version of this call will not copy the data into the emitter, requiring that
    the caller ensure the data viewd by `doc` remains valid until the operation
    completes.
  :return: An `amongoc_emitter` that resolves with a `bson_doc` containing the
    server's response message.
  :allocation: Memory allocation is performed by the event loop of the connection.
  :header: |this-header|

  .. note:: The command requires a ``$db`` field.


.. function::
  amongoc_loop* amongoc_client_get_event_loop(amongoc_client const* cl)
  mlib_allocator amongoc_client_get_allocator(amongoc_client const* cl)

  Obtain the event loop or allocator associated with the client.
