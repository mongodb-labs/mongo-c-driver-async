###############
Why Asynchrony?
###############

.. default-domain:: std
.. default-role:: any

This library presents an *asynchronous* MongoDB client library.

To begin, we define :term:`asynchronous`

.. glossary::

  asynchrony
  asynchronous

    An operation is *asynchronous* when its *initiation* ("what to do") is
    specified separately from its *continuation* ("what to do next"), and they
    are connected by an event that fires at an indeterminate time in the future.

    *Asynchrony* is the presence of asynchronous control flow.

While just about any operation can be made asynchronous, we are primarily concerned
with asynchronous *input and output* operations. I/O operations tend to be
extremely slow compared to all other operations, and their execution does not
impede other work on the system.


The Problem of Synchronous I/O
##############################

The overwhelming majority of all C APIs are synchronous, meaning that the
initiation of an operation is inexorably tied to waiting for its completion.
Consider the following example of a C++ function, :cpp:`lookup_user()`, defined
to find a user object from an unspecified resource::

  struct user {
    string name;
    int age;
  };
  auto lookup_user(int uid) -> user;

and a simple use case of such an API::

  vector<user> find_users(vector<int> uids) {
    vector<user> ret;
    for (auto uid : uids) {
      user u = lookup_user(uid);
      ret.push_back(u);
    }
    return ret;
  }

That is: Given a list of user ID numbers, transform them into a list of the
corresponding user objects. If ``lookup_user`` is a simple lookup to an
in-memory table, this function is perfectly reasonable. If, on the other hand,
``lookup_user`` performs requests to Amazon Glacier, this function is
catastrophically inefficient! While Glacier may be an extreme example, most
workloads will not be so “instantaneous”, and the performance of the function
will now depend on a lot of factors that the caller might not control. There are
a few inefficiencies to note:

- Each request must fully complete before the subsequent request is issued. This
  process scales linearly as the length of the array grows.
- The database client will be completely unaware that it is going to be issuing
  multiple requests to the same datastore. It is likely that such an expensive
  request could be batched together, saving the user time and money.
- The caller of ``find_users`` will need to wait until the entire list of users
  is fetched before being resumed. If the caller is updating a user interface,
  the UI will be blocked and unresponsive while the requests are in flight.


Threads: Not Good Enough
########################

It may be tempting to rewrite the function using threads. There are three basic
approaches that one could take when applying threads to this problem:

1. Spawn a single thread to perform each lookup sequentially, and return a
   ``std::future`` that resolves when the lookup completes.
2. Spawn one thread per lookup, and return an array of futures that resolve
   asynchronously, OR a single future that resolves to the array of user objects
   once all results have arrived.
3. Emulate behavior #2 by submitting the lookup tasks to a thread pool to
   perform the I/O operations.

While option #1 succeeds in unblocking the caller, it runs into the same issues
of serializing the operations that we saw with the prior implementation.

Option #2 is good in that every operation runs in parallel, but now we require
synchronization in the client and pay the overhead of using promise/future
pairs. Additionally, for large arrays of user IDs, we could potentially spawn an
arbitrary number of threads! No good!

Option #3 sounds promising, but is in fact the worst of both worlds: As the
number of requested items grows, the performance degrades to be the same as our
original serialized lookups, and we still need to add our synchronization.
Additionally, we now have another contended resource: the thread pool itself.

The threading approach still leaves us with some open questions:

1. How do we “cancel” an operation in progress?
2. How do we attach a “continuation” to an operation? That is, how can we react
   to the completion of an operation?

With threads, problem #1 can be solved with additional synchronization
primitives, except for in the case of thread pools, where a task may be queued
behind arbitrarily many pending tasks, meaning that we would have to bake the
idea of cancellation into the thread pool as well.

Problem #2 is extremely difficult to solve with threads. If we want to attach a
continuation, where do we put it? What if the operation has already finished by
the time we get around to attaching the continuation? How do we synchronize all
of this stuff?

And even with all of the above, we have now forced the choice upon our users:
Use threads and deal with all of the above problems alongside us, or you can't
use our library at all.


Does this Actually Matter?
##########################

It might also be tempting to ask why we care so much. I/O is reasonably fast,
right?

In the case of test environments and developer workstations, where the database
is running on the same host and we're only issuing a few requests at a time and
we always need to wait for it to complete, it is perfectly reasonable to use
synchronous operations. For many simple applications, synchrony is satisfactory.

For any workload where latency and responsiveness matter, this is a total
non-starter.

As shown in the prior example, even simple uses of the API can become
problematic. By providing a synchronous database API, we are forcing our users
to resort to the aforementioned “bad” pseudo-async designs by offloading our
client library into its own thread to get out of the way of their application.
