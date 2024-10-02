#############
UTF-8 Strings
#############

.. header-file:: bson/utf8.h

  Defines types and functions for handling views of UTF-8 characters.


Types
#####

.. struct:: bson_utf8_view

  .. member::
    const char* data
    uint32_t len

    Pointer to the character array of the string, and the length of that array.

  .. function::
    static bson_utf8_view from_str(const std__string_view&)

    Obtain a BSON UTF-8 view that views the given `std__string_view`

  .. function::
    operator std__string_view() const

    The `bson_utf8_view` is implicitly convertible to a `std__string_view`

  .. function::
    bool operator==(std__string_view sv) const

    Compare the UTF-8 string with another string


Functions & Macros
##################

The following functions are available for the creation of `bson_utf8_view`
objects.

.. function::
  bson_utf8_view bson_as_utf8(auto S)

  Coerce a string-like object to a `bson_utf8_view`.

  :param S: A string to be viewed. May be a `bson_utf8_view` or a C string (:cpp:`const char*`)

  .. note:: This is implemented as a macro.

.. function::
  bson_utf8_view bson_utf8_view_from_cstring(const char* s)
  bson_utf8_view bson_utf8_view_from_data(const char* s, uint32_t len)

  Obtain a `bson_utf8_view` from a character array pointed-to by `s`.

  :param s: A pointer to a character array. `bson_utf8_view_from_cstring`
    requires that this array have a nul terminator.
  :param len: The length of the array pointed-to by `s`.

  .. note::

    `bson_utf8_view_from_data` may be used to create UTF-8 views that contain
    embeded U+0000 ␀ NULL codepoints. This will work in many places, but
    attempting to use such a string for an element key will result in an element
    with a key that is truncated at the first embedded nul. See: `bson_utf8_view_chopnulls`


.. function::
  bson_utf8_view bson_utf8_view_autolen(const char* s, int32_t len)

  Create a `bson_utf8_view` that views an array of characters at `s`, whose
  length may be deduced. If `len` is negative, calls
  `bson_utf8_view_from_cstring`, otherwise uses `bson_utf8_view_from_data`.

.. function::
  bson_utf8_view bson_utf8_view_chopnulls(bson_utf8_view v)

  Return the UTF-8 string from `v` that is truncated at the first embedded
  U+0000 ␀ null character. If `v` does not contain an null characters, the
  returned string will be equal to `v`.

  This function is used on UTF-8 strings that are given as element keys when
  BSON is being modified.
