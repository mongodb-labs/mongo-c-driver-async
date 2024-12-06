######
Update
######

.. function::
  [[1]] amongoc_emitter [[type(amongoc_write_result)]] \
    amongoc_update_many(amongoc_collection* coll, bson_view filter, bson_view updates, const amongoc_update_params* [[nullable]] params)
  [[2]] amongoc_emitter [[type(amongoc_write_result)]] \
    amongoc_update_one(amongoc_collection* coll, bson_view filter, bson_view updates, const amongoc_update_params* [[nullable]] params)
  [[3]] amongoc_emitter [[type(amongoc_write_result)]] \
    amongoc_update_many_with_pipeline(amongoc_collection* coll, bson_view filter, bson_view const* pipeline, size_t pipeline_len, const amongoc_update_params* [[nullable]] params)
  [[4]] amongoc_emitter [[type(amongoc_write_result)]] \
    amongoc_update_one_with_pipeline(amongoc_collection* coll, bson_view filter, bson_view const* pipeline, size_t pipeline_len, const amongoc_update_params* [[nullable]] params)
  [[5]] amongoc_emitter [[type(amongoc_write_result)]] \
    amongoc_update_ex(amongoc_collection* coll, bson_view filter, bson_view const* updates, size_t pipeline_len, bool is_multi, amongoc_update_params const* [[nullable]] params)

  Perform an *update* operation.

  :param coll: The collection which contains the data to be modified.
  :param filter: A filter to select the content to be modified.
  :param updates: For ``[[1]]`` and ``[[2]]``, this must point to a single
    document that contains one or more :external:ref:`update-operators`
  :param pipeline: For ``[[3]]`` and ``[[4]]`` this must be a pointer to one or
    an array of documents that provide an :external:ref:`aggregation-pipeline`.
    See: The *Update with an Aggregation Pipeline* section of the
    :dbcommand:`dbcmd.update` page.
  :param pipeline_len:

    - For ``[[3]]`` and ``[[4]]`` specifies the number of documents pointed-to
      by `pipeline`.

    - **Advabced**: ``[[5]]``, setting this to ``0`` will treat `updates` as a
      pointer to a single document to be treated as either a replacement (for
      `amongoc_replace_one`) or as an update specification (for
      `amongoc_update_one` and `amongoc_update_many`). Setting this to a
      positive number will treat `updates` as a pointer-to-array which specifies
      an aggregation pipeline (for `amongoc_update_one_with_pipeline` and
      `amongoc_update_many_with_pipeline`).
  :param is_multi: For ``[[5]]``: If |true|, updates all documents that match
    the filter. Otherwise, updates at most one document.
  :return: Resolves with an `amongoc_write_result`

.. struct:: [[zero_initializable]] amongoc_update_params

  .. member::
    bson_view const* array_filters
    size_t array_filters_len
    bool bypass_document_validation
    bson_view collation
    bson_value_ref hint
    bson_view let
    bson_value_ref comment

    .. seealso:: :doc:`params`

  .. member:: bool upsert

    If |true|, the operation acts as an *upsert*. Refer to:
    :external:ref:`upsert-behavior`.
