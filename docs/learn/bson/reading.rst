#################
Reading BSON Data
#################

Accessing BSON data is done with two types: `bson_view` and `bson_iterator`.


Getting a View
##############

For any function that expects a `bson_view`, a view can be created from a `bson_doc`
or `bson_mut` using `bson_as_view`:

.. literalinclude:: bson.example.cpp
  :start-after: [as-view]
  :end-before: end.


Obtaining an Iterator
#####################

A `bson_iterator` referring to the first document element can be obtained using
`bson_begin`. `bson_begin` can be passed a `bson_doc`, a `bson_view`, or a
`bson_mut`:

.. literalinclude:: bson.example.cpp
  :start-after: [iter-begin]
  :end-before: end.

An iterator created with `bson_begin()` will immediately refer to the first
element, or may refer to the past-the-end position if the document is empty.

An iterator referring to the past-the-end position is obtained using `bson_end`.
The `bson_end` iterator does not view any element, but instead refers to the
final position in the element sequence.


Iterating over a Document
#########################

Given a valid iterator, an iterator to the next element can be obtained by
calling `bson_next` with that iterator. This can be done in a loop to iterate
over the elements of a document, using `bson_stop` to check for an end or error
condition:

.. literalinclude:: bson.example.cpp
  :start-after: [for-loop]
  :end-before: end.


Easy Mode
*********

In C, the macro :c:macro:`bson_foreach` can be used to iterate over document elements:

.. literalinclude:: bson.example.cpp
  :start-after: [foreach]
  :end-before: end.

In C++, `bson_doc`, `bson_mut`, `bson_view`, `bson::document`, and
`bson::mutator` are all forward ranges and can be used with a regular C++ for
loop:

.. literalinclude:: bson.example.cpp
  :start-after: [c++-for]
  :end-before: end.

`bson_iterator` implements `std__forward_iterator`, and can be used with
standard algorithms.


Decoding Data
#############

Given an iterator referring to a valid element, we can ask its type using
`bson_iterator_type` and we can decode it using the iterator decoding functions:

.. literalinclude:: bson.example.cpp
  :start-after: [get-value]
  :end-before: end.


Example: Seek a Subdocument Element and Iterate It
##################################################

This example uses `bson_find` to seek out a specific element in a document,
checks if its is an array, and then iterates over its elements:

.. literalinclude:: bson.example.cpp
  :start-after: [subdoc-iter]
  :end-before: end.
