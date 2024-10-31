######################
Creation & Destruction
######################

BSON data structures are managed using the `bson_doc` (C) or `bson::document`
(C++) types. These types own an associated region of memory that contains the
BSON data.


Creating (C)
############

Creation of a new `bson_doc` is performed using `bson_new`. When you are done
with a document object, it should be destroyed with `bson_delete`:

.. literalinclude:: bson.example.cpp
  :start-after: ex: [create-c]
  :end-before: end.

The `bson_doc` object is small and safe to pass and return by value from
functions and APIs.


Creating (C++)
##############

C++ code can create a document object using `bson::document`:

.. literalinclude:: bson.example.cpp
  :start-after: ex: [create-cxx]
  :end-before: end.

It will automatically destroy the object when it leaves scope. Copying a
`bson::document` will create a distinct copy of the data.

A C `bson_doc` can be converted to a C++ `bson::document` by passing it as an
r-value to the `bson::document` constructor.

A C++ `bson::document` can be converted to a C `bson_doc` in two ways:

- `bson::document::get` will return an lvalue reference to the wrapped
  `bson_doc`. This returned object should never be used with a |attr.transfer|
  parameter.
- `bson::document::release` will relinquish ownership of the held `bson_doc` and
  return it to the caller. The `bson::document` object will be made null and the
  returned `bson_doc` is now the responsibility of the caller.


Copying
#######

If you have an existing BSON object and want to duplicate it, `bson_new` can
also be used for that purpose:

.. literalinclude:: bson.example.cpp
  :start-after: [copying]
  :end-before: end.

C++ code can simply copy the `bson::document` object to get a distinct copy.


Reserving Memory
################

When creating a BSON document, it may be useful to reserve additional memory for
the document data that we will later insert data into. This can be done by
passing an integer number of bytes when constructing the instance with
`bson_new` or `bson::document`:

.. literalinclude:: bson.example.cpp
  :start-after: [reserve]
  :end-before: end.

.. note::

  Even though we allocate data for the document up-front, the returned object
  is still empty.
