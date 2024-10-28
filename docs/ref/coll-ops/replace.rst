#######
Replace
#######

.. function:: amongoc_emitter amongoc_replace_one(amongoc_collection* coll, bson_view filter, bson_view replacement, amongoc_replace_params const* [[nullable]] params)

  .. seealso::

    Uses a specific :dbcommand:`dbcmd.update` command to replace a single
    document in a collection


.. struct:: [[zero_initializable]] amongoc_replace_params

  .. member::
    bool bypass_document_validation
    bson_view collation
    bson_value_ref hint
    bool upsert
    bson_view let
    bson_value_ref comment

  .. seealso:: :doc:`params`
