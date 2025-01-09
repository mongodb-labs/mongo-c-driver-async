######################
Development Guidelines
######################

.. note:: |devdocs-page|

.. seealso:: :doc:`documenting`


Macros & Preprocessor
#####################

When defining C preprocessor macros, adhere to the following:

Prefer ``snake_case``
*********************

- If the macro is intended to be used as an expression, statement, attribute,
  declaration, or declaration specifier, use ``snake_case``.
- If a :term:`function-like macro` performs nontrivial token manipulation, use
  ``SHOUTING_CASE``.


.. _macros.global-scope:

**Beware** the global-scope specifier
*************************************

**Avoid** writing expression macros that generate a syntax error in C++ if they
are expanded following a global-scope resolution operator::

  // Bad:
  #define function_macro() \
    (_calls_a_function(x, y, z))
  // Causes a syntax error:
  auto n = ::function_macro();

::

  // Okay:
  #define better_constant_value \
    _calls_a_function(x, y, z)
  // Okay:
  auto n = ::function_macro()

If the parenthesis are *absolutely necessary*, use
:c:macro:`mlib_parenthesized_expression`::

  #define function_macro(X, Y) mlib_parenthesized_expression((X) + (Y))
  // Okay:
  auto  z = ::function_macro(4, 5);


.. rubric:: Reasoning

The fact that a function is implemented using a macro is an implementation
detail. If a C++ user uses a global-scope qualifier on a non-macro |amongoc|
function, and we later refactor that function to be a macro, it would be a
breaking change for any C++ users that expect the ``::`` qualifier to continue
to work.


**Avoid** Conditional Compilation
*********************************

Use of conditional compilation should be avoided if possible. Conditional
attributes and specifiers should be hidden behind a macro that performs the
conditional expansion internally (e.g. :c:macro:`mlib_extern_c`). Conditional
compilation should be localized to the smallest lexical scope required.


Use terse conditional compilation
*********************************

Prever to use :c:macro:`MLIB_IF_CXX` and :c:macro:`MLIB_IF_NOT_CXX` for more
concise conditional compilation/macro expansions.


**Avoid** using :cpp:`#ifdef` or :cpp:`#if` with object macros
**************************************************************

Instead, use :cpp:`#if` with :term:`function-like macro`\ s

**Bad**::

  #ifdef __cpllusplus  // Oops: Spelling error
  // this block never compiles
  #endif

**Good**::

  #if mlib_is_cxx()
  // Okay
  #endif // C++

::

  #if milb_is_cxx()  // Compile-error: milb_is_cxx is not defined (typo)


Linkage Blocks
**************

**Prefer** to use :c:macro:`mlib_extern_c_begin` and
:c:macro:`mlib_extern_c_end` to conditional :cpp:`extern "C"`.

.. seealso:: :term:`language linkage`


**Beware** R-values that look like L-values
*******************************************

**Avoid** writing :term:`object-like macros <object-like macro>` that expand to
rvalues. Users may expect to be able to take the address of such an expression
since it looks like an :term:`lvalue`::

  #define my_special_constant 42

  const int* p = &my_special_constant;  // Error!

Instead, use the same idiom as used for C ``errno``::

  inline const int* _mySpecialConstantPtr() {
    const int value = 42;
    return &value;
  }

  #define my_special_constant mlib_parenthesized_expression(*_mySpecialConstantPtr())

Alternatively, write it as a :term:`function-like macro` that accepts no
arguments::

  #define my_special_constant() mlib_parenthesized_expression(42)

While this is less than ideal, it does not look like an :term:`lvalue` when it
appears in source code.


Declaration/Statement macros should require a semicolon
*******************************************************

If a macro is intended to be used like a statement or a declaration, it should
expand such that it requires a following semicolon. Bad examples::

  #define declare_an_int(Name) int Name = 0;
  #define declares_a_func(Name) int Name() { return 42; }
  #define early_return(Cond) \
    if (!(Cond)) { puts("Condition failed"); return EINVAL; }

Better::

  #define declare_an_int(Name) \
    int Name = 0
  #define declares_a_func(Name) \
    int Name() { return 42; } \
    mlib_static_assert(true, "")
  #define early_return(Cond) \
    if (!(Cond)) { puts("Condition failed"); return EINVAL; } \
    else ((void)0)

Requiring a semicolon prevents ambiguity and compiler warnings about extra
semicolons when a user adds a semicolon to a macro expansion that doesn't need
one.


Add comments to distant :cpp:`endif` and :cpp:`#else` directives
****************************************************************

If an :cpp:`#endif` or :cpp:`#else` directive is far away from its associated
conditional, add a comment explaining what it's for::

  #if mlib_is_cxx()

  // many many many many lines

  #else  // ↑ C++ / C ↓

  // many many many more lines

  #endif  // C


Utility Macros
##############

.. c:macro::
  MLIB_LANG_PICK

  A special :term:`function-like macro` that takes two argument lists in two
  sets of parenthesis. If compiled as C, the first argument list will be
  expanded. In C++, the second argument list will be expanded. The unused
  argument list will be discarded. Neither argument will undergo immediate macro
  expansion::

    puts(MLIB_LANG_PICK("I am compiled as C!")("I am compiled as C++!"));

.. c:macro::
  MLIB_IF_CXX(...)
  MLIB_IF_NOT_CXX(...)

  Expands to the given arguments when compiled in C++ or C, resepectively. Prefer
  :c:macro:`MLIB_LANG_PICK` if you have something to say in both languages.


.. c:macro::
  MLIB_IF_CLANG(...)
  MLIB_IF_GCC(...)
  MLIB_IF_MSVC(...)
  MLIB_IF_GNU_LIKE(...)

  Function-like macros that expand to their arguments only when compiled using
  the associated compiler.


.. c:macro:: mlib_parenthesized_expression(...)

  In C, expands to ``(__VA_ARGS__)``. In C++, expands to
  ``mlib::identity{}(__VA_ARGS__)``. This allows the expression to be preceded
  by a global-scope name qualifier when the macro is expanded in C++. See:
  :ref:`macros.global-scope`


.. c:macro:: mlib_extern_c

  Expands to :cpp:`extern "C"` when compiling as C++, otherwise an empty
  attribute. Enforces :term:`C linkage` on an entity.


.. c:macro::
    mlib_extern_c_begin()
    mlib_extern_c_end()

  Declaration-like function macros that expand to the :cpp:`extern "C"` block
  for wrapping APIs with :term:`C linkage`. Note that these expand to
  *declarations* and require a following semicolon::

    mlib_extern_c_begin();

    extern int meow;

    mlib_extern_c_end();

  For declaring a single item, it may be more ergonomic to use
  :c:macro:`mlib_extern_c`.


.. c:macro::
  mlib_is_cxx()
  mlib_is_not_cxx()
  mlib_is_gcc()
  mlib_is_clang()
  mlib_is_msvc()
  mlib_is_gnu_like()

  Expression-macros that evaluate to ``0`` or ``1`` depending on the compiler
  and the compile language.


.. c:macro:: mlib_init(T)

  Usage of this macro is **mandatory** in C headers when writing a compound
  initializer expression. This is required because C++ does not support compound
  initializers, but does support the same syntax with brace initializers::

    my_struct get_thing(int a) {
      return mlib_init(my_struct){a, 42};
    }

  .. note:: The type ``T`` cannot use an :term:`elaborated name`, as that does not
      work with C++ brace initializers


.. c:macro:: mlib_static_assert(Cond, Msg)

  Expands to a static assertion appropriate for the current language.


Public Headers
##############

Header Sorting
**************

Headers should be generally sorted and grouped as follows:

1. Primary :cpp:`#include` directives (this will be the primary associated
   header(s) for a source file).
2. Implementation details
3. ``<amongoc/...>`` headers
4. ``<mlib/...>`` headers
5. Non-standard third-party headers
6. System headers
7. Standard library headers

This sort order is intended to minimize accidental dependencies by catching use
of required symbols as soon as possible.

The ``.clang-format`` file for |amongoc| will generally perform the appropriate
header sorting automatically. This automatic header sorting can be overriding by
placing a comment between :cpp:`#include` directives.


Inclusion Syntax
****************

Relative :cpp:`#include` directives should use double-quote style, and should
begin with a dot or dot-dot path element to emphasize the relative-style header
resolution.

Absolute inclusions should use angle-bracket style.

Run the ``make format`` Makefile target to automatically rewrite and sort all
:cpp:`#include` directives into the appropriate style.


Use Linkage Specifiers
**********************

All non-static C functions and global variable declarations should be annotated
with the appropriate linkage specifiers for C++ compatibility. Use
:c:macro:`mlib_extern_c_begin` and :c:macro:`mlib_extern_c_end` before and after
most C declarations.

In general, you should not use linkage specifiers around type definitions, since
they do not require it, and adding C++ members to types will break if it is
wrapped in :cpp:`extern "C"`.

.. seealso:: :term:`language linkage`


**Be aware** of C's |inline| rules
**********************************

If a C function is declared |inline| *and not* |static|, then there *must* exist
an :cpp:`extern inline` declaration of that function in *exactly one* C
translation unit. This differs from C++, where an |inline| function is emitted
in each TU in which it is used, and the linker merges them at the final step. In
C, this consolidation must be done explicitly using an :cpp:`extern inline`
declaration.

.. note:: There is no way to automatically verify this, because it will only
  generate an error if the compiler decides *not* to do the inlining and expects
  an external definition.


**Do not** use a |static| function in a non-|static| |inline| function in C
***************************************************************************

While probably benign, this will generate an unignorable compiler warning.


C++ Compatibility
*****************

All public headers that have a ``.h`` extension **must** be able to be compiled
as C11 *and* as C++17. Usage of C++20 features should either be moved to a
``.hpp`` header or guarded with ``mlib_have_cxx20()``.

To write a C++-only header, use the ``.hpp`` file extension. The ``.hpp``
headers *may* use C++20.


Inclusion of C++ APIs in C Headers
**********************************

Public C++ APIs *may* be included in C headers if they are not a substantial
portion of the file. These should be simple wrappers around the C types (e.g.
`amongoc::unique_emitter`)


**Avoid** adding C++ constructors to C structs
==============================================

This can create a semantic ambiguity when a C struct is constructed in a C
header.

**Instead, prefer** to use the named-constructor idiom: Use |static| member
functions that construct instances of the object (e.g. `amongoc_status::from`).


**Do not** add copy/move constructors to C structs, nor add a destructor to C structs
=====================================================================================

This will change the definition of inline functions defined in C headers that
use such types, leading to ODR violations.

For easier automatic destruction of C types, follow the rules in :doc:`deletion`.
