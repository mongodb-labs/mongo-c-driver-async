###########################
Creating Asynchronous Loops
###########################

The control flow loop is one of the most fundamental programming constructs. The
abstractions in |amongoc| let you define asynchronous loops, where each loop
step is itself an asynchronous operation. The core API that enables looping is
`amongoc_let`.


The Example Program
###################

The behavior of the sample program is very simple: Accept a positive integer as
a command line argument, and delay for that many seconds, printing a message and
accumulating a sum each second as the countdown runs. This guide will explain
the following program:

.. literalinclude:: looping.example.c
  :linenos:
  :caption: ``looping.example.c``


Initiating the Program
######################

We declare our shared app state as a struct type. This is passed through the
program using a pointer:

.. literalinclude:: looping.example.c
  :start-at: typedef struct
  :end-at: state;
  :lineno-match:

After basic command-line argument processing, we initialize the event loop and
application state for the program:

.. literalinclude:: looping.example.c
  :start-at: default loop
  :end-at: app_state =
  :lineno-match:

In the sample program, the looping operation is initiated here:

.. literalinclude:: looping.example.c
  :start-at: Start the loop
  :end-at: loop_step
  :linenos:
  :lineno-match:

We pass `amongoc_okay` and `amongoc_nil` as the initial "result" values. We pass
a pointer to our application state as the first argument using
`amongoc_box_pointer`.


Loop Stepping
#############

In this example, the ``loop_step`` function is called both *initialy* and
*subsequently* for each sub-operation. The signature is written in such a way
that it is valid as an `amongoc_let_transformer` function. It is possible that
your program will need a separate function to initiate your loop.

.. literalinclude:: looping.example.c
  :lineno-match:
  :start-at: loop_step()
  :end-before: // Print

Our loop stepping function is called both to initiate the loop and to continue
the loop after the sub-operation has completed. Because our initiation and our
operation (`amongoc_schedule_later`) both give us `amongoc_nil`, we do not need
to handle the result value (passed as the third parameter) and we can simply
discard it.


Printing a Mesage & Accumulating a Sum
**************************************

We then extract our application state, perform some arithmetic, and print a
progress message to the user.

.. literalinclude:: looping.example.c
  :lineno-match:
  :start-at: Print our
  :end-at: fprintf

This will assure the user that the application is running.


Checking the Stop Condition
***************************

While we *could* loop forever, our program only wants to wait a limited amount
of time. We use a branch and `amongoc_just` to terminate the loop when our
countdown reaches zero.

.. literalinclude:: looping.example.c
  :lineno-match:
  :start-at: countdown == 0
  :end-at: }

The `amongoc_just` function creates a pseudo-async operation that resolves
immediately with the given result. Here, we create a successful status with
`amongoc_okay` and use `amongoc_box_uint64` to create a box that stores the
final calculation. This result value box will appear at the end of our loop.


Starting a Timer
****************

If our countdown is not finished, we use `amongoc_schedule_later` to start
another delay:

.. literalinclude:: looping.example.c
  :lineno-match:
  :start-at: Decrement
  :end-at: schedule_later

`amongoc_schedule_later` creates an operation that will start a timer for the
given duration and then complete with `amongoc_nil` when the timer fires.


Continuing the Loop
*******************

The core looping behavior is handled by `amongoc_let`, a building-block of
asynchrony that creates allows the completion of an asynchronous operation to
initiate a new operation to continue the program:

.. literalinclude:: looping.example.c
  :lineno-match:
  :start-at: Connect the sleep
  :end-at: amongoc_let

This connects the completion of the timeout from `amongoc_schedule_later` to a
continuation via ``loop_step``. The operation returned by ``loop_step`` becomes
the result of the emitter returned by `amongoc_let`.

Since our ``loop_step`` can return another operation initiated by `amongoc_let`,
the emitter from `amongoc_let` will loop back on itself until the returned
emitter resolves in some other fashion (in our case, `amongoc_just`, or if
there is an error condition, specified by `amongoc_async_forward_errors`).


Starting and Running the Loop
#############################

Our looping operation has a final result value, so we will want to create and
tie storage for it. We do this using `amongoc_tie`, which will also create the
final `amongoc_operation` from the looping emitter we created from initiating
the loop.

.. literalinclude:: looping.example.c
  :start-at: Tie the final
  :end-at: amongoc_tie
  :lineno-match:

`amongoc_tie` will attach a handler to the emitter that will store the final
result and status in the pointed-to location when the operation completes.


Starting the Operation
**********************

When next execute `amongoc_start` on the returned operation object:

.. literalinclude:: looping.example.c
  :start-at: amongoc_start
  :end-at: amongoc_start
  :lineno-match:

This will launch the operation on the event loop. The program isn't running yet,
though, as the default event loop requires a thread to execute it.


Running the Loop and Finalizing the Operation
*********************************************

.. literalinclude:: looping.example.c
  :start-at: Run the program
  :end-at: default_loop_destroy
  :lineno-match:

The call to `amongoc_default_loop_run` will execute the operations stored in
the event loop and return when all operations are complete. We are then safe to
destroy the operation with `amongoc_operation_delete`, and we can discard the
event loop with `amongoc_default_loop_destroy`.


Error Handling and Printing the Final Result
############################################

After the emitter used with `amongoc_tie` has completed, its final result
status and value are stored in the pointed-to storage, ready to be read by the
program. We check for errors, either printing the error message or printing the
final result:

.. literalinclude:: looping.example.c
  :start-at: is_error
  :end-at: return 0;
  :lineno-match:

We use `amongoc_is_error` to test the final status for an error condition. If it
is an error, we get and print the error message to stderr, and we must destroy
the final result box because it may contain an unspecified value related to the
error, but we don't want to do anything with it.

In the success case, we extract the value returned in `amongoc_just` as a
``uint64_t`` and print it to stdout. Note that because the box returned by
`amongoc_box_uint64` is :ref:`trivial <box.trivial>`, we can safely discard the
box without destroying it.


.. code-block:: console
  :caption: Looping Example Output

  $ looping.example.exe 15
  15 seconds remain, current value: 0
  14 seconds remain, current value: 1
  13 seconds remain, current value: 1
  12 seconds remain, current value: 2
  11 seconds remain, current value: 3
  10 seconds remain, current value: 5
  9 seconds remain, current value: 8
  8 seconds remain, current value: 13
  7 seconds remain, current value: 21
  6 seconds remain, current value: 34
  5 seconds remain, current value: 55
  4 seconds remain, current value: 89
  3 seconds remain, current value: 144
  2 seconds remain, current value: 233
  1 seconds remain, current value: 377
  0 seconds remain, current value: 610
  Got final value: 610
