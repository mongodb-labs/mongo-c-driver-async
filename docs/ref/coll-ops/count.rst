#####
Count
#####

.. function::
  amongoc_emitter [[type(int64_t)]] amongoc_count_documents(amongoc_collection* coll, bson_view filter, const amongoc_count_params* [[nullable]] params)
  amongoc_emitter [[type(int64_t)]] amongoc_estimated_document_count(amongoc_collection* coll, const amongoc_count_params* [[nullable]] params)

  Count the documents in a collection.

  .. seealso::

    `amongoc_estimated_document_count` is implemented in terms of
    :dbcommand:`dbcmd.count`, while `amongoc_count_documents` returns an exact
    answer using an :dbcommand:`dbcmd.aggregate` command.

.. struct:: [[zero_initializable]] amongoc_count_params

  .. member::
    bson_view collation
    bson_value_ref hint
    int64_t limit
    mlib_duration max_time
    int64_t skip
    bson_value_ref comment

  .. seealso:: :doc:`params`
