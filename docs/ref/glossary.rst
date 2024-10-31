###########
Terminology
###########

.. glossary::

  lvalue

    An *lvalue* expression is an expression which carries the identity of some
    object. In particular, the name of a variable, dereferencing of a pointer,
    or a subscript into an array are all lvalue expressions.

    .. seealso::

      - :external:doc:`c/language/value_category`
      - :external:doc:`cpp/language/value_category`

    As a rule of thumb: If you can take the address of the expression with the
    :cpp:`&` operator, it is an lvalue expression.

    A *modifiable* lvalue is an lvalue that can be used on the left-hand of an
    assignment operator. e.g. dereferencing a pointer-to-|const| is an lvalue,
    but it is not valid to assign to such an expression.

  translation unit

    In C and C++, a *translation unit* consists of the total textual input given
    to a compiler when a source file is compiled. This includes the main source
    file, as well as all headers that are included by that file.

  emitter

    An *emitter* is the core of the |amongoc| async model. It is an object that
    defines a prepared asynchronous operation (i.e. "what to do"), but does not
    yet have a continuation. It must be *connected* to a :term:`handler` and
    this will produce the :term:`operation state` object.

    The |amongoc| emitter is defined by the `amongoc_emitter` type.

    See: :ref:`Emitters`

  handler

    A *handler* is an object that defines the continuation of an asynchronous
    operation (i.e. "what to do next"). A handler also tells an operation how to
    handle cancellation.

    Generally, an |amongoc| user won't be working with handlers directly, as
    they are used as the building blocks for more complete asynchronous
    algorithms.

    The |amongoc| handler is defined by the `amongoc_handler` type.

    See: :ref:`model.handlers`

  operation state

    The *operation state* is an object created by connecting an :term:`emitter`
    to a :term:`handler`. It must be explicitly started to launch the operation.

    This is defined by the `amongoc_operation` type. Typically, you will only
    have a few of these in an application, possibly only one for the entire
    program.
