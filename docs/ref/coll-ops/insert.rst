######
Insert
######

.. function::
  amongoc_emitter [[type(amongoc_write_result)]] amongoc_insert_ex(amongoc_collection* coll, const bson_view* documents, size_t n_docs, amongoc_insert_params const* [[nullable]] params)
  amongoc_emitter [[type(amongoc_write_result)]] amongoc_insert_one(amongoc_collection* coll, __bson_viewable doc)
  amongoc_emitter [[type(amongoc_write_result)]] amongoc_insert_one(amongoc_collection* coll, __bson_viewable doc, amongoc_insert_params const* [[nullable]] params)

  Insert data into the collection. Resolves with an `amongoc_write_result`.

  :param coll: Pointer to a collation to be updated.
  :param documents: Pointer to a document or array of documents to be inserted.
  :param n_docs: Number of documents pointed-to by `documents`
  :param params: Named parameters for the insert operation.

  .. seealso:: :dbcommand:`dbcmd.insert`


.. struct:: [[zero_initializable]] amongoc_insert_params

  .. member::
    bool bypass_document_validation
    bool ordered
    bson_value_ref comment

  .. seealso:: :doc:`params`
