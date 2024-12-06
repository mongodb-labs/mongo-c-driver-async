###################
Database Handle API
###################

.. header-file:: amongoc/database.h

  Components for database-level functionality.


Types
#####

.. type:: amongoc_database

  An incomplete type that acts as a handle to a database open on an
  `amongoc_client`. Create with `amongoc_database_new`, destroy with
  `amongoc_database_delete`.

  A database handle has the following properties:

  - A client handle, obtainable using `amongoc_database_get_client`
  - A name, obtainable using `amongoc_database_get_name`


Functions & Macros
##################

.. function:: amongoc_database* amongoc_database_new(amongoc_client* cl, __string_convertible name)

  Create a new database handle associated with client `cl` for the database
  named `name`.

  :param cl:
    A valid client handle instance. The client handle must outlive the returned
    database handle.
  :return:
    Upon success, returns a non-null handle to a database. If there is an
    allocation failure, returns a null handle.

  .. note::

    This operation does not attempt to perform any modifications on the actual
    remote database. It only creates a client-side database handle.


.. function:: void amongoc_database_delete(amongoc_database* [[transfer, nullable]] db)

  Delete the client-side database handle. If the handle is null, does nothing.

  .. note::

    This function does not perform any server-side operation. It only deletes
    the resources associated with the client-side handle.


.. function::
  amongoc_client amongoc_database_get_client(amongoc_database const* db)
  const char* amongoc_database_get_name(amongoc_database const* db)

  Obtain the `amongoc_client` client handle or database name string associated
  with the database handle.
