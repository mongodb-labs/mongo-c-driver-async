####
Find
####

.. function::
  amongoc_emitter [[type(amongoc_cursor)]] amongoc_find(amongoc_collection* coll, bson_view filter, amongoc_find_params const* [[nullable]] opts)

  Execute a query on a collection. Resolves with an `amongoc_cursor`.

  A cursor must always be given to `amongoc_cursor_delete` or `amongoc_cursor_next`.

  .. seealso:: :dbcommand:`dbcmd.find`

.. struct:: [[zero_initializable]] amongoc_find_params

  .. member::
    bool allow_disk_use
    bool allow_partial_results
    int batch_size
    bson_view  collation
    bson_value_ref hint
    amongoc_cursor_type cursor_type
    bson_view  let
    int limit
    bson_view  max
    int64_t max_await_time_ms
    int64_t max_scan
    mlib_duration max_time
    bson_view  min
    bool no_cursor_timeout
    bool oplog_replay
    bson_view  projection
    bool return_key
    bool show_record_id
    int64_t skip
    bool snapshot
    bson_view  sort

  .. seealso:: :doc:`params`


.. struct:: amongoc_cursor

  A type that contains information about a partial result of a query. Should be
  deleted with `amongoc_cursor_delete`

  .. member:: int64_t cursor_id

    The integer identifier for the cursor within the collection.

  .. member:: amongoc_collection* collection

    Pointer to the collection handle that owns the cursor.

  .. member:: bson_doc records

    A BSON array document that contains the records of the result.


.. function::
  amongoc_cursor_delete(amongoc_cursor [[transfer]] cur)

  Releases resources associated with the given cursor |attr.transfer|.


.. function::
  amongoc_emitter [[type(amongoc_cursor)]] amongoc_cursor_next(amongoc_cursor [[transfer]] cur)

  Obtain the next set of results from a cursor.
