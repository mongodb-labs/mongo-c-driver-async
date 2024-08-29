###################
Connect to a Server
###################

This how-to guide will walk through the follow example program:

.. literalinclude:: connect.example.c
  :caption: ``connect.example.c``
  :linenos:

Headers
#######

We first include the "everything" library header:

.. literalinclude:: connect.example.c
  :caption: Header file inclusions
  :lineno-match:
  :start-at: #include
  :end-at: >

This will make all public APIs visible. Individual APIs can be included on a
header-by-header basis. Refer to the documentation on a component to learn what
header contains it.


Command-Line Arguments
######################

The first action we perform in ``main()`` is checking our arguments:

.. literalinclude:: connect.example.c
  :caption: Argument checking
  :lineno-match:
  :start-at: int main
  :end-at: const port

We will use ``argv[1]`` as the hostname and ``argv[2]`` as the port number


Initializing the Loop
#####################

The first "interesting" code will declare and initialize the default event loop:

.. literalinclude:: connect.example.c
  :lineno-match:
  :start-at: loop;
  :end-at: );

.. seealso:: `amongoc_loop` and `amongoc_default_loop_init`

.. hint::

  It is possible to provide your own event loop implementation, but that is out
  of scope for this page.


Declare the App State
#####################

We use a type ``app_state`` to store some state that is shared across the
application:

.. literalinclude:: connect.example.c
  :lineno-match:
  :start-at: struct app_state
  :end-at: } app_state;

This state needs to be stored in a way that it outlives the scope of each
sub-operation in the program. For this reason, we declare the instance in
``main()``:

.. literalinclude:: connect.example.c
  :lineno-match:
  :start-at: struct app_state state =
  :end-at: struct app_state state =

This will zero-initialize the contents of the struct. This is important so that
the declared `amongoc_connection` object is also null, ensuring that the later
call to `amongoc_conn_destroy` will be a no-op if that field is never
initialized.

We will pass the state around by-reference in a box using `amongoc_box_pointer`.


Create a Connect with a Timeout
###############################

We create a connect operation using `amongoc_conn_connect`, and then attach a
timeout using `amongoc_timeout`

.. literalinclude:: connect.example.c
  :lineno-match:
  :start-at: conn_connect
  :end-at: amongoc_timeout

We pass the emitter ``em`` to `amongoc_timeout` on the same line as as
overwriting it with the return value. This emitter-exchange pattern is common
and preferred for building composed asynchronous operations.


Attach the First Continuation
#############################

.. literalinclude:: connect.example.c
  :lineno-match:
  :start-at: amongoc_let
  :end-at: say_hello);

The `amongoc_let` function will attach a continuation to an operation in a way
that lets us initiate a subsequent operation. We use
`amongoc_async_forward_errors` to tell `amongoc_let` that it should call our
continuation *only* if the input operation resolves with a non-error status. If
`amongoc_conn_connect` fails with an error (including a timeout), then
``after_connect_say_hello`` will not be called, and the errant result will
simply be passed to the next step in the chain.


The First Continuation
######################

The first step, after connecting to a server, is ``after_connect_say_hello``, a
continuation function given to `amongoc_let`:

.. literalinclude:: connect.example.c
  :lineno-match:
  :start-at: ** after_connect
  :end-at: ) {

The `amongoc_status` parameter is unused: Because we passed
`amongoc_async_forward_errors` in `amongoc_let`, our continuation will only be
called if the `amongoc_status` would be zero, so there is no need to check it.


.. rubric:: Capture the Client

Upon success, the operation from `amongoc_conn_connect` will resolve with an
`amongoc_connection` in its boxed result value. We move the connection by-value
from the box and store it in our application state:

.. literalinclude:: connect.example.c
  :lineno-match:
  :start-at: after_connect_say_hello(amongoc_box
  :end-at: take

The :c:macro:`amongoc_box_take` macro is used to transfer the ownership from a
type-erased `amongoc_box` into a typed storage for the object that it holds. The
object that was owned by the box is now owned by the storage destination.


.. rubric:: Build and Prepare a Command

.. literalinclude:: connect.example.c
  :lineno-match:
  :start-at: Create a "hello" command
  :end-at: bson_mut_delete

We build a MongoDB command ``{ hello: "1", $db: "test" }`` and prepare to send
it as a command with `amongoc_conn_command`. We can delete the prepared BSON
document here as a copy of the data is now stored within the prepared operation
in ``em``.


.. rubric:: Attach the Second Continuation

.. literalinclude:: connect.example.c
  :lineno-match:
  :start-at: em = amongoc_then(
  :end-at: after_hello);

We set the next step in the process to call ``after_hello`` once the command has
completed. We use `amongoc_then` instead of `amongoc_let`, because
``after_hello`` does not return a new operation to continue, just a new result
value.

The `amongoc_async_forward_errors` flag tells `amongoc_then` to *not* call
``after_hello`` if the operation enounters an error, and will instead forward
the error to the next operation in the chain.


The Second Continuation
#######################

The second continuation after we receive a response from the server is very
simple:

.. literalinclude:: connect.example.c
  :lineno-match:
  :start-at: after_hello()
  :end-before: end.

We simply extract a `bson_view` of the response data from the ``resp_data`` box
that was provided by `amongoc_conn_command` and print it to standard output.
After we finish printing it, we destroy the data and return `amongoc_nil`. This
is the final value for the program.


Tie the Final Result
####################

Going back to ``main()``, after our call to `amongoc_let` in which we attached
the first continuation, we use `amongoc_tie` to convert the emitter to an
`amongoc_operation`:

.. literalinclude:: connect.example.c
  :lineno-match:
  :start-at: fin_status
  :end-at: amongoc_tie

This will allow us to see the final result status of the program in
``fin_status`` after the returned operation ``op`` completes. We pass ``NULL``
for the `amongoc_tie::value` parameter, indicating that we do not care what the
final result value will be (in a successful case, this would just be the
`amongoc_nil` returned from ``after_hello``).


Start the Operation, Run the Loop, and Clean Up
###############################################

.. literalinclude:: connect.example.c
  :lineno-match:
  :start-at: amongoc_start
  :end-at: default_loop_destroy

We now launch the composed operation using `amongoc_start`. This will enqueue
work on the event loop. We then give the event loop control to run the program
using `amongoc_default_loop_run`. After the event loop returns, all asynchronous
work has completed, and we destroy the operation with
`amongoc_operation_destroy`. This will free resources associated with the
operation.

We also now have a copy of the connection that was created with
`amongoc_conn_connect`. We destroy that with `amongoc_conn_destroy`. If the
connect operation failed, the connection object will have remained
zero-initialized and the call will be a no-op.

Finally, we are done with the event loop, and we can destroy it with
`amongoc_default_loop_destroy`.


Print the Final Result
######################

.. literalinclude:: connect.example.c
  :lineno-match:
  :start-at: is_error
  :end-before: end.

Finally, we inspect the `amongoc_status` that was produced by our operation and
updated using `amongoc_tie`. This will tell us if there were any errors during
the execution of the program. If so, we print the error message to standard
error and exit non-zero from the program.
