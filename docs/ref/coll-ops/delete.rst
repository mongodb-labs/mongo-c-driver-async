######
Delete
######

.. function::
  amongoc_emitter [[type(int64_t)]] amongoc_delete_ex(amongoc_collection* coll, \
      bson_view filter, \
      bool delete_only_one, \
      amongoc_delete_params const* [[nullable]] params)
  amongoc_emitter [[type(int64_t)]] amongoc_delete_many(amongoc_collection* coll, bson_view filter, amongoc_delete_params const* [[nullable]] params)
  amongoc_emitter [[type(int64_t)]] amongoc_delete_one(amongoc_collection* coll, bson_view filter, amongoc_delete_params const* [[nullable]] params)

  Delete documents from a collection. Resolves with the number of documents that
  were deleted.

  .. seealso:: :dbcommand:`dbcmd.delete`


.. struct:: [[zero_initializable]] amongoc_delete_params

  .. member::
    bson_view collation
    bson_value_ref hint
    bson_view let
    bson_value_ref comment

  .. seealso:: :doc:`params`
