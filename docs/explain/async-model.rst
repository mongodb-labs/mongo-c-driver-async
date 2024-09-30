####################
Our Asynchrony Model
####################

.. default-domain:: std
.. default-role:: any

The previous pages (:doc:`why-async` and :doc:`consider`) explored *why* we want
to provide :term:`asynchrony`, and this page will explain *how* we provide
asynchrony.


Emitters & Handlers
###################

The asynchrony model in |amongoc| is defined by :term:`emitters <emitter>` and
:term:`handlers <handler>`.

.. glossary::

  emitter

    An *emitter* is the core of the |amongoc| async model. It is an object that
    defines a prepared asynchronous operation (i.e. "what to do"), but does not
    yet have a continuation. It must be *connected* to a :term:`handler` and
    this will produce the :term:`operation state` object.

    The |amongoc| emitter is defined by the `amongoc_emitter` type.

    See: `Emitters`

  handler

    A *handler* is an object that defines the continuation of an asynchronous
    operation (i.e. "what to do next"). A handler also tells an operation how to
    handle cancellation.

    Generally, an |amongoc| user won't be working with handlers directly, as
    they are used as the building blocks for more complete asynchronous
    algorithms.

    The |amongoc| handler is defined by the `amongoc_handler` type.

  operation state

    The *operation state* is an object created by connecting an :term:`emitter`
    to a :term:`handler`. It must be explicitly started to launch the operation.

    This is defined by the `amongoc_operation` type. Typically, you will only
    have a few of these in an application, possibly only one for the entire
    program.


.. _Emitters:

Emitters
********

An :term:`emitter` object, defined by `amongoc_emitter`, represents a pending
asynchronous operation, where "operation" may be anything from "send some data
to a server" to *your entire application*.

An emitter is said to *resolve* or *complete* when its associated asynchronous
operation finishes. In |amongoc|, a resolving emitter *emits* two objects: an
`amongoc_status` representing the success/error/etc of the operation, and an
`amongoc_box` that encloses the final "result value" for the operation. The
meaning of the result value depends on the asynchronous operation of the emitter
and the value of the result status.

For example, the `amongoc_client_new` function returns an `amongoc_emitter`
that resolves with an `amongoc_client` upon success (this is represented in
the documentation with the |attr.type| attribute).


Accessing an Emitter's Result
=============================

There is no way to "ask" an emitter for its result value. Instead, an emitter
must be *connected* to a :term:`handler` that acts on the completion of the
emitter.

However, using `amongoc_handler` directly is a very low-level and error-prone
process. For this reason, |amongoc| provides convenience functions for the
purpose of composing emitters automatically without needing to create and juggle
`amongoc_handler`\ s oneself, such as `amongoc_then`, `amongoc_let`,
`amongoc_detach`, and `amongoc_tie`.


Prior Art - Senders & Receivers
###############################

The asynchronous model provided by |amongoc| is based on a highly-simplified
version of `P2300 - std::execution`__, the leading proposal for defining a
universal asynchronous execution design for C++.

__ https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2300r10.html

The differences are many, mostly by omission, but the most important changes
are as follows (if you are unfamiliar with P2300, you can ignore these details):

1. In |amongoc|, *senders* and *receivers* are called *emitters* and *handlers*,
   respectively. This name change is designed to prevent confusion between the
   two designs.
2. In |amongoc|, the *scheduler* mechanism of senders is absent.
3. In |amongoc|, because we are a C library, all emitters and handlers are
   type-erased to single struct types, `amongoc_emitter` and `amongoc_handler`.
4. Emitters always emit two values: an `amongoc_status` and an `amongoc_box`,
   which also type-erases the result type. The actual emitted result type is a
   matter of documentation for the associated operation.
5. In |amongoc|, emitters have one completion channel, whereas senders have
   three ("value", "error", and "stopped"). Emitters transmit the
   success/error/cancellation state via their `amongoc_status` value.
6. |amongoc| has the concept of operation cancellation, but does not use stop
   tokens. Instead, an `amongoc_handler` uses
   `amongoc_handler_vtable::register_stop` to connect stop callbacks for an
   operation.
