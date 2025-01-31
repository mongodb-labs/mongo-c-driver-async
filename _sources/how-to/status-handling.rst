##############################
How to Handle `amongoc_status`
##############################

The `amongoc_status` type is an extensible status-reporting type, allowing
different subsystems to share integral error code values and understand each
other's error conditions without universal coordination on the meaning of opaque
integral values. A status object has two salient properties: A
`code <amongoc_status::code>`, and a `category <amongoc_status::category>`.
Generally, user code should not inspect the ``code`` without also respecting the
``category``.


Checking for Errors
###################

To test whether a particular opaque status represents a failure condition, **do
not** be tempted to inspect the code value directly. Instead, use
`amongoc_is_error`::

  amongoc_status some_status = some_function();

  // BAD:
  if (some_status.code) {
    // ...
  }

  // GOOD:
  if (amongoc_is_error(some_status)) {
    // ...
  }

While it is conventional that an integer value zero represents a successful
status, it is possible that the status' category may have more than one success
code value, and they may be non-zero. `amongoc_is_error` consults the status
category to determine whether the status code indicates an error.


Obtaining an Error Message
##########################

The status category also knows how to convert an integer value into a
human-readable status message. The `amongoc_message` function will obtain a
:term:`C string` that represents the message for a status::

  const char* msg = amongoc_message(some_status, NULL, 0);
  printf("Status message: %s\n", msg);

Supporting Dynamic Messages
***************************

A status category may need to dynamically generate a message string based on the
integer value. In order to do this, it needs space to write the message string.
This is the purpose of the second and third parameters to `amongoc_message`::

  char msgbuf[128];
  const char* msg = amongoc_message(some_status, msgbuf, sizeof msgbuf);
  printf("Status message: %s\n", msg);

The second parameter must be a pointer to a modifiable array of |char|, and the
third parameter must be the number of characters available to be written at that
pointer. *If* `amongoc_message` needs to dynamically generate a message string,
it will use the provided buffer and return a pointer to that same buffer.
`amongoc_message` *might* or *might not* use the message buffer to generate the
string, so you **should not** expect the character array to be modified by
`amongoc_message`. Always use the returned :term:`C string` as the status
message.

.. note::

  If you do not provide a writable buffer for the message, `amongoc_message` may
  return a fallback string that loses information about the status, so it is
  best to always provide the writable buffer.


A Shorthand
***********

Because declaring a writable buffer and calling `amongoc_message` is so common,
a shorthand macro :c:macro:`amongoc_declmsg` is defined that can do this
boilerplate in a single line::

  amongoc_declmsg(msg_var, some_status);
  printf("Status msesage: %s\n", msg_var);

Internally, this will declare a small writable buffer and call `amongoc_message`
for us. The first parameter is the name of a variable to be declared, and the
second parameter is the `amongoc_status` to be inspected.


Easy Error Handling
###################

You may find yourself writing code like this, repeatedly::

  amognoc_status status = some_function();
  if (amongoc_is_error(status)) {
    amongoc_declmsg(msg, status);
    fprintf(stderr, "The function failed: %s\n", msg);
    return 42;
  } else {
    fprintf(stderr, "The function succeeded\n");
    return 0;
  }

A macro, :c:macro:`amongoc_if_error`, can be used to do this more concisely::

  amongoc_if_error (some_function(), msg) {
    fprintf(stderr, "The function failed: %s\n", msg);
    return 42;
  } else {
    fprintf(stderr, "The function succeeded\n");
    return 0;
  }
