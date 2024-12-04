#########
Aggregate
#########

.. function::
  amongoc_emitter [[type(amongoc_cursor)]] amongoc_aggregate(amongoc_database* db, bson_view const* pipeline, size_t pipeline_len, amongoc_aggregate_params const* [[nullable]] params)
  amongoc_emitter [[type(amongoc_cursor)]] amongoc_aggregate(amongoc_collection* coll, bson_view const* pipeline, size_t pipeline_len, amongoc_aggregate_params const* [[nullable]] params)

  Execute an aggregation pipeline on a database or collection. Resolves with an
  `amongoc_cursor`.

.. struct:: [[zero_initializable]] amongoc_aggregate_params

  .. member::
    bool allow_disk_use
    int32_t batch_size
    bool bypass_document_validation
    bson_view collation
    mlib_duration max_await_time
    bson_value_ref comment
    bson_value_ref hint
    bson_view let

  .. seealso:: :doc:`params`
