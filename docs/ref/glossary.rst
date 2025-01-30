###########
Terminology
###########

.. glossary::
  :sorted:

  C linkage
  C++ linkage
  language linkage

    The *language linkage* is a property of entities that have
    :term:`external linkage` in a C++ program. A function/variable that is
    intended to be usable from C must have C linkage specified when it is
    defined in C++.

    .. seealso:: :external:doc:`cpp/language/language_linkage`

  external linkage

    In C and C++ programs, if an entity (function, variable, or type) has
    *external linkage*, this means that all references to that entity have the
    same identity between all :term:`translation units <translation unit>`.

  elaborated name
  elaborated type specifier

    In C++, an *elaborated* type name/specifier is a type specifier that
    disambiguates with a preceding :cpp:`class/struct/union/enum` tag keyword.

    .. seealso:: :external:doc:`cpp/language/elaborated_type_specifier`

    In C, "elaborated type specifiers" are the "default" syntax for
    :cpp:`struct/union/enum` types. This can be changed by using an unqualified
    name for the equivalent type that was declared using a :cpp:`typedef`.
    Unless such a typedef exists, the tag-qualified name must be used.

  function-like macro

    A *function-like macro* is a preprocessor macro that requires parentheses in
    order to expand::

      #define ONE_MORE(N) (N + 1)  // "ONE_MORE" is a function macro

    This contrasts with :term:`object-like macros <object-like macro>`.

  lvalue
  lvalue expression

    An *lvalue* expression is an expression which carries the identity of some
    object. In particular, the name of a variable, dereferencing of a pointer, a
    subscript into an array, and accessing the member of an
    :cpp:`class/struct/union` are all lvalue expressions.

    .. seealso::

      - :external:doc:`c/language/value_category`
      - :external:doc:`cpp/language/value_category`

    As a rule of thumb: If you can take the address of the expression with the
    :cpp:`&` operator, it is an lvalue expression.

    A *modifiable* lvalue is an lvalue that can be used on the left-hand of an
    assignment operator. e.g. dereferencing a pointer-to-|const| is an lvalue,
    but it is not valid to assign to such an expression.

  object-like macro

    An *object macro* is a preprocessor macro that does not require parentheses
    in order to expand::

      #define FOO 42  // "FOO" is an object-macro

    This contrasts with :term:`function-like macros <function-like macro>`.

  C string

    A *C string* is a contiguous array of |char| that is terminated with a nul
    character (a |char| with integral value :cpp:`0`). A C string is typically
    represented with a pointer-to-|char| in C code, where the pointer refers to
    the first character in the character array. The "length" of a *C string* is
    the number of characters in the array that precede the nul terminator. i.e.
    a C string of length 3 will have three non-zero characters and one nul
    character, requiring an array of length 4.

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
