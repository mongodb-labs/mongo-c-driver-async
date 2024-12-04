#################
Common Parameters
#################

Several of the :doc:`collation operations <index>` accept an overlapping set of
named parameters, passed via fields of an aggregate type associated with the
operation. This page will document the semantics of those parameters.


.. struct:: __common_params

  An exposition-only struct of common operation parameters.


  .. member:: bool bypass_document_validation

    A boolean option that is valid for commands which insert or modify content
    of a collection. If |true|, then documents can be created in a collection
    which do not meet its
    :external:ref:`validation requirements <schema-validation-overview>`.


  .. member:: bson_view collation

    This field represents an optional *collation* to be used with a search
    operation. If given a `null view <bson_view_null>`, then no collation will
    be attached to the query.

    .. seealso:: :ref:`collation`


  .. member:: bson_value_ref comment

    Specify an arbitrary value to be attached to the command. This has no
    behavioral effect, but does appear in
    :ref:`server-side log messages <log-messages-ref>` and :ref:`database
    profiler <profiler>` output. If this is a :ref:`null reference
    <bson-null-ref>`, no comment will be attached to database commands.


  .. member:: bson_value_ref hint

    Specifiy an *index hint* for an operation. If |zero-initialized|, no hint
    will be used. This value **must** be a
    :ref:`null reference <bson-null-ref>`, a string, or a document view.


  .. member:: bson_view let

    Specify a list of variables that may be substituted in the associated
    operation query.

    .. seealso::

      - :external:ref:`The "let" syntax for the "find" command <find-let-syntax>`
      - :external:ref:`find-let-example`


  .. member:: int64_t limit

    Specifiy a non-negative limit. This will set the maximum number of results
    to return from a query. A limit of zero is equivalent to specifying no
    limit.


  .. member::
    bson_view max
    bson_view min

    Specify the *exclusive upper bound* and *inclusive lower bound* for the
    index specified with the `hint` field.

    .. note:: If either of these are set, then `hint` must also be provided.


  .. member:: bson_view projection

    Set the projection document for a query.

    .. seealso:: :external+mongodb:term:`projection` (MongoDB Glossary)


  .. member:: int64_t skip

    A non-negative integer that specifies the number of documents to skip in the
    results.


  .. member:: bson_view sort

    An optional sort specification for a query.
