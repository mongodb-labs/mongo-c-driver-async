#####################
Collection & Data API
#####################

.. header-file:: amongoc/collection.h

  Defines a collection handle type and CRUD APIs.

.. |this-header| replace:: :header-file:`amongoc/collection.h`

.. toctree::
  :caption: Collection Operations
  :maxdepth: 2

  coll-ops/index


Types
#####

.. type:: amongoc_collection

  An incomplete type that acts as a handle on a collection within a database and
  associated client. Create using `amongoc_collection_new` and destroy with
  `amongoc_collection_delete`


.. var:: const amongoc_status_category_vtable amongoc_crud_category

  A status category related to CRUD operations.


.. enum:: amongoc_crud_errc

  .. enumerator:: amongoc_crud_okay

    Operation was succsesful

  .. enumerator:: amongoc_crud_write_errors

    The operation generated one or more write errors.


.. struct:: [[zero_initializable]] amongoc_write_result

  A result type produced by certain :doc:`collection operations <coll-ops/index>`.

  Delete with `amongoc_write_result_delete`.

  .. member::
    int64_t inserted_count
    int64_t matched_count
    int64_t modified_count
    int64_t deleted_count
    int64_t upserted_count
    amongoc_write_error_vec [[nullable]] write_errors


.. struct:: amongoc_write_error

  Delete with `amongoc_write_error_delete`.

  .. member::
    int32_t index
    amongoc_server_errc code
    mlib_str errmsg


.. struct:: amongoc_write_error_vec

  A :doc:`vector type </ref/vec>` of `amongoc_write_error`


Functions & Macros
##################

.. seealso::

  Refer to the :doc:`coll-ops/index` page for methods that perform CRUD
  operations on a collection.

.. function:: amongoc_collection* amongoc_collection_new(amongoc_client* client, __string_convertible db_name, __string_convertible coll_name)

  Create a new handle to a database collection. Does not perform any I/O, just
  creates a client-side handle.

  :param client: A valid client for the new collection handle. The client object
    must outlive the returned collection handle.
  :param db_name: Name of the database in the MongoDB server that will own the
    collection.
  :param coll_name: The name of the collection in the database.

  The returned collection handle must eventually be given to `amongoc_collection_delete`


.. function:: void amongoc_collection_delete(amongoc_collection* coll)

  Release any client-side resources associated with the collection handle.
