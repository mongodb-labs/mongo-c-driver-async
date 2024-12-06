##################################
Design Considerations in |amongoc|
##################################

Following up from :doc:`why-async` and before we proceed to :doc:`async-model`,
we'll explore the design considerations necessary to explain why certain
decisions were made.


.. _no-globals:

No Global Context
#################

There is no ``amongoc_initialize_the_whole_library_before_i_can_use_it()`` API,
nor an ``amongoc_shutdown_the_library_plz()``. All library constructs may be
declared within a scope in which they need to be used, and then destroyed when
we are finished with them. You may call `amongoc_default_loop_init` on as many
event loop objects as you wish, and run `amongoc_default_loop_run` on each of
them, and there will be no interference between them.

There is no implicit global event loop,
:ref:`nor is there a dedicated "event loop" thread <thread>`.


.. _thread:

No Threads Required
###################

In some asynchronous libraries, it is assumed that the user is bringing threads
to the table, and therefore the library will assume that it can shunt work into
a thread arbitrarily with strong forward progress guarantees.

This is particularly annoying to users who now need to care about
synchronization between their code and the library components: When will this
callback be called? When will my objects be destroyed? Could my object be
destroyed while I still have a pointer to them? What if more than one callback
is invoked simultaneously? What APIs are thread-safe?

|amongoc| takes the approach that multi-threading is not a foregone conclusion.
All APIs *are* reentrant, but only because
:ref:`they are written without shared state <no-globals>`.

Additionally, becuase we have :ref:`no prescribed event loop <prescribe-loop>`,
it is entirely up to the user on which thread(s) of execution their code will
execute. We do not require the user bring their own thread to pump any global
even loop.


.. _prescribe-loop:

No Prescribed Event Loop
########################

While |amongoc| does provide a fully-functional
`default event loop <amongoc_default_loop_init>`, it is possible for the user to
bring their own event loop by customizing an `amongoc_loop`.

All asynchronous APIs will go through an `amongoc_loop` to perform their work,
meaning the user has complete control over *how* the asynchronous I/O is
executed on the underlying system.


Air-Tight Ownership & Lifetime Semantics Are Mandatory
######################################################

Asynchronous programming -- especially in an environment without garbage
collection -- makes object lifetime management particularly challenging, as
objects need to move in/out of the event loop constantly. It is challenging to
know *when* it is safe to destroy an object, and how to convey information about
the destruction of objects across the program.

|amongoc| is written from the ground-up to have an extremely unambiguous object
lifetime model, with strong guarantees about the destruction of objects both
automatically by the systen and explicitly by the user.


Asynchrony Should Be Composable
###############################

Let's face it: bare function pointers and :c:expr:`void*` are *not* composable
abstractions.

|amongoc| has a strong emphasis on composable asynchrony. The |amongoc| APIs
that power the simple "asynchronously connect to a server" can be stacked
ad-infinitum to build a full, complex, asynchronous applications. Sure, function
pointers *will* be involved, but not in the traditional sense.

