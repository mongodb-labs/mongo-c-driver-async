######################
Strings & String Views
######################

.. header-file:: mlib/str.h

  Defines types and functions for handling strings and views of strings of
  characters.


Types
#####

.. struct:: mlib_str_view

  .. member::
    const char* data

    Pointer to the character array of the string.

  .. function::
    static mlib_str_view from(const std__string_view&)

    (C++) Obtain a view that views the given `std__string_view`

  .. function::
    operator std__string_view() const

    (C++) The `mlib_str_view` is implicitly convertible to a `std__string_view`

  .. function::
    bool operator==(std__string_view sv) const

    Compare the string with another string


Functions & Macros
##################

The following functions are available for the creation of `mlib_str_view`
objects.

.. function::
  mlib_str_view mlib_as_str_view(auto S)

  Coerce a string-like object to a `mlib_str_view`.

  :param S: A string to be viewed. May be a `mlib_str_view` or a C string (:cpp:`const char*`)

  .. note:: This is implemented as a macro.

.. function::
  mlib_str_view mlib_str_view_data(const char* s, uint32_t len)

  Obtain a `mlib_str_view` from a character array pointed-to by `s`.

  :param s: A pointer to a character array.

  .. note::

    `mlib_str_view_from_data` may be used to create UTF-8 views that contain
    embeded U+0000 ␀ NULL codepoints. This will work in many places, but
    attempting to use such a string for an element key will result in an element
    with a key that is truncated at the first embedded nul. See:
    `mlib_str_view_chopnulls`


.. function::
  mlib_str_view mlib_str_view_chopnulls(mlib_str_view v)

  Return the UTF-8 string from `v` that is truncated at the first embedded
  U+0000 ␀ null character. If `v` does not contain an null characters, the
  returned string will be equal to `v`.

  This function is used on UTF-8 strings that are given as element keys when
  BSON is being modified.
