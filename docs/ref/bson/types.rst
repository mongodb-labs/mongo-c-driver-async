################
BSON Value Types
################

.. header-file:: bson/types.h

  Defines a type enumeration for BSON value types, as well as several utility
  types for holding BSON values.


Types
#####

Type Enumeration
****************

.. enum:: [[zero_initializable]] bson_type

  This enumeration corresponds to the types of BSON elements. Their numeric
  value is equal to the octet value that is encoded in the BSON data itself.

  .. enumerator::
    bson_type_eod = 0x00
    bson_type_double = 0x01
    bson_type_utf8 = 0x02
    bson_type_document = 0x03
    bson_type_array = 0x04
    bson_type_binary = 0x05
    bson_type_undefined = 0x06
    bson_type_oid = 0x07
    bson_type_bool = 0x08
    bson_type_date_time = 0x09
    bson_type_null = 0x0A
    bson_type_regex = 0x0B
    bson_type_dbpointer = 0x0C
    bson_type_code = 0x0D
    bson_type_symbol = 0x0E
    bson_type_codewscope = 0x0F
    bson_type_int32 = 0x10
    bson_type_timestamp = 0x11
    bson_type_int64 = 0x12
    bson_type_decimal128 = 0x13
    bson_type_maxkey = 0x7F
    bson_type_minkey = 0xFF


Element Value Types
*******************

The following custom struct types are defined for decoding certain element values.


.. struct::
  [[zero_initializable]] bson_eod
  [[zero_initializable]] bson_undefined
  [[zero_initializable]] bson_null
  [[zero_initializable]] bson_maxkey
  [[zero_initializable]] bson_minkey

  Empty struct types defined as placeholders for the BSON unit value types. The
  `bson_eod` struct is a special placeholder that represents the lack of value.


.. struct:: [[zero_initializable]] bson_binary_view

  .. member::
    const bson_byte* data;
    uint32_t data_len;

    Pointer to the binary data of the object, and the length of that data.

  .. member:: uint8_t subtype

    The binary data subtype tag


.. struct:: [[zero_initializable]] bson_oid

  .. member:: uint8_t bytes[12]

    The twelve octets of the object ID


.. struct:: [[zero_initializable]] bson_dbpointer_view

  .. member::
    mlib_str_view collection

    The collection name

  .. member:: bson_oid object_id

    The object ID within the collection


.. struct:: [[zero_initializable]] bson_regex_view

  .. member::
    mlib_str_view regex
    mlib_str_view options

    The regular expression string and options string.


.. struct:: [[zero_initializable]] bson_timestamp

  .. member::
    uint32_t increment
    uint32_t timestamp


.. struct:: [[zero_initializable]] bson_symbol_view

  .. member:: mlib_str_view utf8

    The symbol spelling string


.. struct:: [[zero_initializable]] bson_code_view

  .. member:: mlib_str_view utf8

    The code string


.. struct:: [[zero_initializable]] bson_decimal128

  .. member::
    uint8_t bytes[16]


.. struct:: [[zero_initializable]] bson_datetime

  .. member:: int64_t utf_ms_offset

    The offset from the Unix epoch as a count of milliseconds


.. type::
  bson::null = ::bson_null
  bson::undefined = ::bson_undefined
  bson::minkey = ::bson_minkey
  bson::maxkey = ::bson_maxkey

  Aliases of the C type within the ``bson`` namespace.
