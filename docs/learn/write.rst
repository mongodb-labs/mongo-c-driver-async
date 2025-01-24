############
Writing Data
############

This tutorial will cover the basics of writing data into a MongoDB server
asynchronously. This page will assume that you have read the content of
the :doc:`connect` tutorial. It will assume that you have a program that
includes the |amongoc| headers and know how to connect to a server.


Declaring Application State Type
################################

This program will require that certain objects outlive the sub-operations, so we
will need to persist them outside the event loop and pass them to our
continuation. We declare a simple aggregate type to hold these objects:

.. literalinclude:: write.example.c
  :caption: Application State Struct
  :start-at: // Application state
  :end-at: app_state;
  :lineno-match:


Starting with a Connection
##########################

We'll start with the basics of setting up an event loop and creating a
connection :term:`emitter`:

.. literalinclude:: write.example.c
  :caption: Setting up an event loop
  :start-at: int main
  :end-at: amongoc_client_new
  :lineno-match:


Declare the Application State Object
####################################

Declare an instance of ``app_state``, which will be passed through the program
by address. We zero-initialize it so that the pointer members are null, making
deletion a no-op in case they are never initialized later.

.. literalinclude:: write.example.c
  :caption: Application state object
  :start-at: // Application state object
  :end-at: ;
  :lineno-match:


Create the First Continuation
#############################

We have the pending connection object and the application state, so now we can
set the first continuation:

.. literalinclude:: write.example.c
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

.. literalinclude:: write.example.c
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

.. literalinclude:: write.example.c
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

.. literalinclude:: write.example.c
  :caption: Set up the Collection
  :start-at: Create a new collection handle
  :end-at: }
  :lineno-match:

`amongoc_collection_new` does not actually communicate with the server: It only
creates a client-side handle that can be used to perform operations related to
that collection. The server-side collection will be created automatically when
we begin writing data into it.

We store the returned collection handle in the state struct so that we can later
delete it.


Create Some Data to Insert
**************************

To insert data, we first need data to be inserted. We can create that with the
BSON APIs:

.. literalinclude:: write.example.c
  :caption: Create a Document
  :start-at: // Create a document
  :end-before: // Insert the single

.. seealso:: :doc:`bson/index` for tutorials on the BSON APIs.


Create the Insert Operation
***************************

Inserting a single document can be done with `amongoc_insert_one`:

.. literalinclude:: write.example.c
  :caption: Insert the Data
  :start-at: // Insert the single
  :end-at: return insert_em
  :lineno-match:

`amongoc_insert_one` will take a copy of the data, so we can delete it
immediately. We then return the resulting insert emitter from ``on_connect`` to
tell `amongoc_let` to continue the composed operation.


The Full Continuation
*********************

Here is the full ``on_connect`` function:

.. literalinclude:: write.example.c
  :caption: The ``on_connect`` function
  :start-at: // Continuation after
  :end-before: end:on_connect
  :lineno-match:


Tie, Start, and Run the Operation
#################################

Going back to ``main``, we can now tie the operation, start it, and run the
event loop:

.. literalinclude:: write.example.c
  :caption: Run the Program
  :start-at: amongoc_tie
  :end-at: loop_destroy
  :lineno-match:

`amongoc_tie` tells the emitter to store its final status in the given pointer
destination, and returns an operation that can be initiated with
`amongoc_start`. We then give control to the event loop with
`amongoc_default_loop_run`. After this returns, the operation object is
finished, so we delete it with `amongoc_operation_delete`.

We are also finished with the collection handle and the client, so we delete
those here as well. Those struct members will have been filled in by
``on_connect``.


Error Checking
##############

The ``status`` object will have been filled during `amongoc_default_loop_run`
due to the call to `amongoc_tie`. We can check it for errors and print them out
now:

.. literalinclude:: write.example.c
  :caption: Check the Final Status
  :start-at: // Print the final
  :end-at: return 0;
  :lineno-match:


The Full Program
################

Here is the full sample program, in its entirety:

.. literalinclude:: write.example.c
  :caption: ``write.example.c``
  :linenos:
