########
Distinct
########

.. function:: amongoc_emitter [[type(bson_value_vec)]] amongoc_distinct(amongoc_collection* coll, mlib_str_view field_name, bson_view filter, const amongoc_distinct_params* [[nullable]] params)

  Obtain a list of distinct values for the fields of a document that match the
  query.

  .. seealso:: :dbcommand:`dbcmd.distinct`


.. struct:: [[zero_initializable]] amongoc_distinct_params

  .. member::
    bson_view collation
    bson_value_ref comment

  .. seealso:: :doc:`params`
