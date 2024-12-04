######################
Strings & String Views
######################

.. header-file:: mlib/str.h

  Defines types and functions for handling strings and views of strings of
  characters.


Types
#####

.. struct:: [[zero_initializable]] mlib_str_view

  :zero-initialized: |attr.zero-init| A zero-initialized `mlib_str_view`
    represents a *null* string view. It does not point to any string, including
    any empty string. Testing for a null string view can be done by testing
    whether the `data` member is a null pointer. A null string view is not
    equal to any non-null string view. Two null strings views are considered
    equivalent.

    .. note:: Converting a null `mlib_str_view` to a C++ `std__string_view`
      will create a `std__string_view` that views an unspecified empty string.

  .. member::
    const char* data
    size_t len

    Pointer to the character array of the string, and the length of the
    pointed-to string as a count of :cpp:`char`.

    .. note::

      The pointed-to array at `data` is *not* guaranteed to be null-terminated,
      and the pointed-to array may contain embedded nul characters.

  .. function::
    static mlib_str_view from(const std__string_view&)

    (C++) Obtain a view that views the given `std__string_view`

  .. function::
    operator std__string_view() const

    (C++) The `mlib_str_view` is implicitly convertible to a `std__string_view`

  .. function::
    bool operator==(std__string_view sv) const

    Compare the string with another string


.. struct:: [[zero_initializable]] mlib_str

  Represents a dynamically allocated immutable null-terminated string.

  :zero-initialized: |attr.zero-init| A zero-initialized `mlib_str` represent a
    *null* string. This is **distinct** from an empty string. Testing for a null
    string can be done by comparing the `data` member to a null pointer. Calling
    `mlib_str_delete` on a null string is a no-op. Creating an `mlib_str_view`
    from a null string will create a null `mlib_str_view`. Using a null string
    for any other operation is undefined behavior.

  .. member:: const char* data

    Pointer to the array of characters. This is guaranteed to be
    null-terminated, but may contain embedded nulls.

  .. member:: mlib_allocator alloc

    The allocator used for this string

  To get the length of the string, use `mlib_strlen`

  .. note:: This type supports the creation of strings that contain embedded null characters!


.. struct:: mlib_str_mut

  A mutable string type. Convertible to `mlib_str` by accessing the `str`
  member.

  Internally, this has an anonymous union member to implement the type pun.

  Creating a new string is done using `mlib_str_new` or `mlib_str_copy`.

  .. member::
    char* data

    Pointer to the mutable array of characters. This is guaranteed to be
    null-terminated, but may contain embedded null characters.

  .. member::
    mlib_allocator alloc

    The memory allocator used by this string.

  .. member::
    mlib_str str

    Accessing this member of the struct will yield an equivalent read-only
    `mlib_str` instance. Copying this member is effectively the same as moving
    the mutable string into an immutable copy.


.. type:: __string_convertible

  A `__string_convertible` parameter is any type that can be converted to an
  `mlib_str_view`. The following types are supported in C and C++ code:

  - `mlib_str_view`
  - `mlib_str`
  - `mlib_str_mut`
  - :cpp:`char [const]*` (null terminated C strings, inluding string literals)

  From C++ code, any type convertible to `std__string_view` may be used.


Globals & Constants
###################

.. var::
  const mlib_str_view mlib_str_view_null
  const mlib_str mlib_str_null

  A null `mlib_str_view` and `mlib_str`.

  .. note:: |macro-impl|


Functions & Macros
##################

String View Creation
********************

.. function::
  mlib_str_view mlib_as_str_view(__string_convertible S)

  Coerce a string-like object `S` to a `mlib_str_view`.

  .. note:: |macro-impl|

.. function::
  mlib_str_view mlib_str_view_data(const char* s, size_t len)

  Obtain a `mlib_str_view` from a character array pointed-to by `s`.

  :param s: A pointer to a character array.

  .. note::

    `mlib_str_view_from_data` may be used to create UTF-8 views that contain
    embeded U+0000 ␀ NULL codepoints. This will work in many places, but
    attempting to use such a string for an element key will result in an element
    with a key that is truncated at the first embedded nul. See:
    `mlib_str_view_chopnulls`


.. function::
  mlib_str_view mlib_str_view_chopnulls(__string_convertible v)

  Return the longest prefix of `v` that does not contain any embedded U+0000 ␀
  null character. If `v` does not contain an null characters, the returned
  string will be equal to `v`.

  This function is used on UTF-8 strings that are given as element keys when
  BSON is being modified.

  .. note:: |macro-impl|


String Creation
***************

.. function::
  mlib_str_mut mlib_str_new()
  mlib_str_mut mlib_str_new(size_t n)
  mlib_str_mut mlib_str_new(size_t n, mlib_allocator alloc)

  Construct a new `mlib_str_mut`.

  :param n: The number of characters to allocate for the new string.
  :param alloc: A memory allocator to imbue in the new string.

  New characters are zero-initialized.

  .. important:: The created string must eventually be cast to `mlib_str`
    and deleted with `mlib_str_delete`.

  .. note:: |macro-impl|

.. function::
  mlib_str_mut mlib_str_copy(__string_convertible s)
  mlib_str_mut mlib_str_copy(__string_convertible s, mlib_allocator alloc)

  Create a copy of the given string `s`.

  :param s: A string from which to copy characters.
  :param alloc: A memory allocator for the new string.

  .. important:: The created string must eventually be cast to `mlib_str`
    and deleted with `mlib_str_delete`.

  .. note:: |macro-impl|

.. function:: void mlib_str_delete(mlib_str [[transfer]] s)

  Delete and deallocate any associated memory for the string `s` |attr.transfer|.


String Manipulation & Modification
**********************************

.. function:: void mlib_str_assign(mlib_str* s, mlib_str [[transfer]] from)

  Delete the string pointed-to by `s` and take the new value from `from`.

  :param s: A non-null pointer to a valid `mlib_str` object. A zero-initialized `mlib_str`
    is considered valid.
  :param from: A string that will be moved into `s`.

  Ownership of the resource in `from` is considered to be transferred into `s`.
  Any resources previously owned by `s` are released. This function is
  equivalent to::

    mlib_str_delete(s);
    s = from;

  This is intended as a convenience for rebinding a `mlib_str` in a single
  statement from an expression returning a new `mlib_str`, which may itself use
  `s`, without requiring a temporary variable, for example::

    mlib_str s = get_string();
    mlib_str_assign(&s, convert_to_uppercase_copy(s));


.. function:: bool mlib_str_mut_resize(mlib_str_mut* mut, size_t new_len)

  Resize the mutable string `mut` to have length `new_len`. Returns |false| in
  case of allocation failure.

  The original contents of `mut` are unmodified. New characters are
  zero-initialized.


.. function:: mlib_str mlib_str_splice(__string_convertible str, size_t pos, size_t del_count, __string_convertible insert, mlib_allocator alloc = mlib_default_allocator)

  Create a new string based on `str` with the given modifications.

  :param str: The string from which we will copy.
  :param pos: The zero-based index at which to perform the splice.
  :param del_count: The number of code units that will be deleted from the
    string. This will be clamped to the string length.
  :param insert: A string that will be inserted at `pos`.
  :param alloc: An allocator for the new string.

  If `del_count` is zero, this simply inserts the string. If `insert` is empty,
  this will only delete characters from the string. If `del_count` is zero and
  `insert` is empty, simply returns a copy of the string.

  Returns a null string on allocation failure.


.. function::
  mlib_str mlib_str_append(__string_convertible s, __string_convertible suffix)
  mlib_str mlib_str_prepend(__string_convertible s, __string_convertible prefix)

  Append or prepend an affix to another string.


.. function::
  mlib_str mlib_str_insert(__string_convertible s, size_t pos, __string_convertible infix)

  Insert the string `infix` into a copy of string `s` at position `pos`.


.. function::
  mlib_str mlib_str_erase(__string_convertible s, size_t pos, size_t count)

  Erase `count` code units in a new copy of `s` at position `pos`.


.. function::
  mlib_str mlib_str_remove_prefix(__string_convertible s, size_t count)
  mlib_str mlib_str_remove_suffix(__string_convertible s, size_t count)

  Remove `count` characters from the beginning/end of a copy of `s`.

.. function::
  mlib_str mlib_substr(__string_convertible s, size_t pos, size_t len)

  Create a new string copy of the substring of `s` beginning at `pos` and
  continuing for at most `len` code units.


String Observers
****************

.. function:: bool mlib_str_eq(__string_convertible a, __string_convertible b)

  Test whether two strings are equal. `a` and `b` are considered equal if they
  have the same length and equal code units at each position in the string.


.. function:: char mlib_str_at(__string_convertible S, ptrdiff_t off)

  Obtain the character at the given offset, with negative index wrapping.

  :param off: The zero-based offset if non-negative. If negative, accesses from
    the end of the string such that :cpp:`-1` is the last character, :cpp:`-2`
    is the second-last character, etc.


.. function::
  int mlib_str_find(__string_convertible haystack, __string_convertible needle)
  int mlib_str_rfind(__string_convertible haystack, __string_convertible needle)

  Find the zero-based offset of an occurrence of `needle` within `haystack`. If
  the substring is not found, returns an unspecified negative value.

  `mlib_str_find` finds the *first* occurrence of `needle`, while
  `mlib_str_rfind` finds the *last* occurrence of `needle`.


.. function::
  size_t mlib_strlen(__string_convertible S)

  Obtain the length of the given string as a number of :cpp:`char` code units.
