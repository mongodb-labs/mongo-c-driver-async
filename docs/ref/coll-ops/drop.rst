Drop
####

.. function::
  amongoc_emitter [[type(nil)]] amongoc_collection_drop(amongoc_collection* coll, amongoc_collection_drop_params const* [[nullable]] params)

  Delete all data from the referred-to collection

  .. seealso:: :dbcommand:`dbcmd.drop`


.. struct::
  [[zero_initializable]] amongoc_collection_drop_params

  .. member:: bson_value_ref [[nullable]] comment

  .. seealso:: :doc:`params`
