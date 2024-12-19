##############################
Connecting to a MongoDB Server
##############################

This page will show how to asynchronously connect to a MongoDB server from a C
program.


Including the |amongoc| APIs
############################

To make all |amongoc| APIs visible, include the everything-header:

.. literalinclude:: connect.example.c
  :caption: Headers
  :start-at: #include
  :end-before: end:headers
  :lineno-match:
  :dedent:


Creating an Event Loop
######################

The first thing to do before connecting to a program is to create an event loop.
|amongoc| includes a basic default single-threaded event loop suitable for basic
programs:

.. literalinclude:: connect.example.c
  :caption: Create a default event loop
  :start-at: int main
  :end-at: default_loop_init
  :lineno-match:
  :dedent:


Creating a Client
#################

A client is created and initialized asynchronously and is associated with an
event loop, done using `amongoc_client_new`:

.. literalinclude:: connect.example.c
  :caption: Create a client emitter
  :start-at: client_new
  :end-at: ;
  :lineno-match:
  :dedent:

.. hint:: The URI string above is a simple default for a locally-running server
  without TLS enabled. You should replace it with your own if your URI is
  running in a different location.


Create the Continuation
#######################

An `amongoc_emitter` object is not a client. Rather, it is an object that
represents an asynchronous operation. To get the client, we need to attach a
continuation:

.. literalinclude:: connect.example.c
  :caption: Continuation prototype
  :start-at: on_connect
  :end-at: ;
  :lineno-match:
  :dedent:

.. literalinclude:: connect.example.c
  :caption: Attach the continuation with `amongoc_then`
  :start-at: amongoc_then
  :end-at: ;
  :lineno-match:
  :dedent:

The continuation function ``on_connect`` looks like this:

.. literalinclude:: connect.example.c
  :caption: Continuation
  :start-after: on_connect def
  :end-before: on_connect end
  :lineno-match:
  :dedent:


Create the Operation State
##########################

When we are done defining the entire asynchronous control flow, we need to
convert the `amongoc_emitter` to an operation and enqueue it with the event
loop. There are several ways to do this, but the simplest is `amongoc_detach_start`:

.. literalinclude:: connect.example.c
  :caption: Launch the operation
  :start-at: detach_start
  :end-at: detach_start
  :lineno-match:
  :dedent:

This will enqueue the associated program with the event loop, and will
automatically release resources associated with the operation when the operation
completes.

.. note:: `amongoc_detach_start` will "consume" the emitter object. The ``em``
  emitter object is "poisoned" and cannot be manipulated further.


Run the Program
###############

The `amongoc_detach_start` call only enqueues the operation, but does not
execute it. We need to actually give control of the main thread to the event
loop. This is done using `amongoc_default_loop_run`:

.. literalinclude:: connect.example.c
  :caption: Run the program
  :start-at: default_loop_run
  :end-at: default_loop_run
  :lineno-match:
  :dedent:


Clean up the Event Loop
#######################

After `amongoc_default_loop_run` returns, there is no more pending work in the
event loop, so we are done. Before returning, we need to destroy the event loop
object:

.. literalinclude:: connect.example.c
  :caption: Destroy the event loop
  :start-at: default_loop_destroy
  :end-at: default_loop_destroy
  :lineno-match:
  :dedent:


The Whole Program
#################

Here is the complete program:

.. literalinclude:: connect.example.c
  :caption: ``connect.example.c``
  :lineno-match:
