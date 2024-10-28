#####################
Header: |this-header|
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

.. struct:: amongoc_collection

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
