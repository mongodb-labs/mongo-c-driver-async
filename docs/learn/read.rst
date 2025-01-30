############
Reading Data
############

This tutorial will show the basics of reading data from a MongoDB server. It
assumes that you have read the :doc:`connect` tutorial, and that you have a
MongoDB server that contains data you wish to read.


Declaring Application State Type
################################

This program will require that certain objects outlive the sub-operations, so we
will need to persist them outside the event loop and pass them to our
continuation. We declare a simple aggregate type to hold these objects:

.. literalinclude:: read.example.c
  :caption: Application State Struct
  :start-at: // Application state
  :end-at: app_state;
  :lineno-match:


Parsing Command Arguments
#########################

Our program will take three parameters: A MongoDB server URI, a database name,
and a collection name. We can parse those as the start of ``main``:

.. literalinclude:: read.example.c
  :caption: Argument Handling
  :start-at: int main
  :end-at: };
  :lineno-match:

We store the database name and the collection name in the shared state struct
so that we can access it in subsequent operations.


Starting with a Connection
##########################

We next do the basics of setting up an event loop and creating a connection
:term:`emitter`:

.. literalinclude:: read.example.c
  :caption: Setting up an event loop
  :start-at: amongoc_loop loop;
  :end-at: amongoc_client_new
  :lineno-match:


Create the First Continuation
#############################

We have the pending connection object and the application state, so now we can
set the first continuation:

.. literalinclude:: read.example.c
  :caption: The first continuation
  :start-at: Set the continuation
  :end-at: on_connect);
  :lineno-match:


The arguments are as follows:

1. Passing the emitter we got from `amongoc_client_new`, and replacing it with
   the new operation emitter returned by `amongoc_let`.
2. `amongoc_async_forward_errors` is a behavior control flag for `amongoc_let`
   that tells the operation to skip our continuation if the input operation
   generates an error.
3. We pass the address of the ``state`` object by wrapping it with
   `amongoc_box_pointer`, passing it as the ``userdata`` parameter of
   `amongoc_let`.
4. ``on_connect`` is the name of the function that will handle the continuation.


Define the Continuation
#######################

Now we can look at the definition of ``on_connect``:

.. literalinclude:: read.example.c
  :caption: Continuation signature
  :start-at: // Continuation after connection
  :end-at: (void)status;
  :lineno-match:

The ``state_`` parameter is the userdata box that was given to `amongoc_let` in
the previous section. The ``client`` parameter is a box that contains the
`amongoc_client` pointer from the connection operation. The ``status`` parameter
here is the status of the connection operation, but we don't care about this
here since we used `amongoc_async_forward_errors` to ask `amongoc_let` to skip
this function if the status would otherwise indicate an error (i.e.
``on_connect`` will only be called if the connection actually succeeds).


Update the Application State
****************************

``on_connect`` is passed the pointer to the application state as well as a box
containing a pointer to the `amongoc_client`. We can extract the box value and
store the client pointer within our application state struct:

.. literalinclude:: read.example.c
  :caption: Update the Application State
  :start-at: // Store the client
  :end-at: amongoc_box_take
  :lineno-match:

We store it in the application state object so that we can later delete the
client handle when the program completes.


Create a Collection Handle
**************************

Now that we have a valid client, we can create a handle to a collection on
the server:

.. literalinclude:: read.example.c
  :caption: Set up the Collection
  :start-at: Create a new collection handle
  :end-before: Initiate a read operation.
  :lineno-match:

`amongoc_collection_new` does not actually communicate with the server: It only
creates a client-side handle that can be used to perform operations related to
that collection.

We store the returned collection handle in the state struct so that we can later
delete it.


Initiate a Read Operation
*************************

There are several different reading operations and reading parameters. For this
program, we'll simply do a "find all" operation:

.. literalinclude:: read.example.c
  :caption: Start a "Find All" Operation
  :start-at: Initiate a read
  :end-at: return
  :lineno-match:

The `amongoc_find` function will return a new `amongoc_emitter` that represents
the pending read from a collection. The first parameter :cpp:`state->collection`
is a pointer to a collection handle. The second parameter `bson_view_null` is a
filter for the find operation. In this case, we are passing no filter, which is
equivalent to an empty filter, telling the database that we want to find all
documents in the collection. The third parameter is a set of common named
parameters for find operations. In this case, we are passing :cpp:`NULL`,
because we don't care to specify any more parameters.

We attach a second continuation using `amongoc_let` to a function ``on_find``.


Read from The Cursor
********************

When the `amongoc_find` emitter resolves, it resolves with an `amongoc_cursor`
object. We'll extract that in ``on_find``:

.. literalinclude:: read.example.c
  :caption: Handle Data
  :start-at: Continuation when we get data
  :end-at: box_take
  :lineno-match:

We can print out the cursor's batch of data:

.. literalinclude:: read.example.c
  :caption: Print the Records
  :start-at: Print the data
  :end-at: batch_num++
  :lineno-match:


Read Some More
**************

A single read may not return the full set of data, so we may need to initiate
a subsequent read with `amongoc_cursor_next`:

.. literalinclude:: read.example.c
  :caption: Check for more
  :start-at: Do we have more
  :end-before: end:on_find
  :lineno-match:

By passing `on_find` to `amongoc_let` again, we create a loop that continually
calls ``on_find`` until the cursor ID is zero (indicating a finished read).

If we have no more data, we destroy the cursor object and return a null result
immediately with `amongoc_just`.


Tie, Start, and Run the Operation
#################################

Going back to ``main``, we can now tie the operation, start it, and run the
event loop:

.. literalinclude:: read.example.c
  :caption: Run the Program
  :start-at: Run the program
  :end-at: loop_destroy
  :lineno-match:

`amongoc_tie` tells the emitter to store its final status and result value in
the given destinations, and returns an operation that can be initiated with
`amongoc_start`. We then give control to the event loop with
`amongoc_default_loop_run`. After this returns, the operation object is
finished, so we delete it with `amongoc_operation_delete`.

We are also finished with the collection handle and the client, so we delete
those here as well. Those struct members will have been filled in by
``on_connect``.


Check for Errors
################

If any sub-operation failed, the error status will be propagated to the final
status, so we check that before exiting:

.. literalinclude:: read.example.c
  :caption: Check for Errors
  :start-at: Check for errors
  :end-at: }
  :lineno-match:


The Full Program
################

Here is the full sample program, in its entirety:

.. literalinclude:: read.example.c
  :caption: ``read.example.c``
  :linenos:
