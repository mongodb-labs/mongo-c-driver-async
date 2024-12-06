#############
Query Objects
#############

.. warning:: |devdocs-page|

The C++ components contain *query objects*, which generalize the idea of getters
to invocables that can look up attributes of another object.

A query object |Q| can be invoked on an object to be queried: :math:`Q(x)`,
which will return the value associated with attribute |Q| on |x|, if such an
attribute is available.


.. namespace:: amongoc

.. class::
  get_allocator_fn
  get_stop_token_fn

  Look up the allocator or the stop token associated with an object, if one
  is available. These classes have an overloaded ``operator()`` method that
  allows them to be used as function objects.

  For a query object |Q| of type ``get_xyz_fn``, the overloaded ``operator()``
  does the following when passed an object |x|:

  1. If |x| has a method ``get_xyz()``, returns the result of that function.
  2. Otherwise, if |x| has a method ``query()`` that accepts an instance of
     ``get_xyz_fn``, returns the result of ``query(Q)``.
  3. Otherwise, the overloaded ``operator()`` is not available.

.. var::
  constexpr inline get_allocator_fn get_allocator
  constexpr inline get_stop_token_fn get_stop_token

  Global instances of the associated query object types.

.. concept::
  template <typename T> has_allocator
  template <typename T> has_stop_token

  Test whether the associated queries are valid for the type `T`

.. type::
  template <typename T> get_allocator_t
  template <typename T> get_stop_token_t

  Get the result type of applying the associated queries.

.. type::
  template <typename Q, typename T> \
  query_t = decltype(Q{}(std::declval<const T&>()))

  Obtains the result of applying query type `Q` to an object `T`. Requires: :expr:`valid_query_for<Q, T>`

.. concept::
  template <typename Q, typename T> valid_query_for

  .. var::
    Q q
    const T& t

    Requires that :expr:`q(t)` is a valid expression.


Query Forwarding Example
########################

Suppose we have a function-wrapping type which wraps an invocable and adds the
argument ``42`` as the last argument when the wrapper is invoked::

  template <typename F>
  struct also_42 {
    F _fn;

    auto operator()(auto&&... args) {
      return _fn(args..., 42);
    }
  };

It is possible that ``F`` has attributes that we want to expose on ``also_42``.
In that case, we would add a ``query`` method template with `valid_query_for`:

.. code-block::
  :emphasize-lines: 9-11

  template <typename F>
  struct also_42 {
    F _fn;

    auto operator()(auto&&... args) {
      return _fn(args..., 42);
    }

    auto query(valid_query_for<F> auto q) const {
      return q(_fn);
    }
  };

This allows any query object that is valid for ``F`` to be applied to
``also_42<F>``.


Differences from P2300
######################

P2300 also defines query objects and query types, but with a few important
differences:

1. Queryable objects in P2300 may define a separate *environment* object so that
   their queries can be performed on a separate instance from the underyling
   object. Our querying code does not use separate environments, as we don't yet
   have a need for this. This may be added later if the need arises.
2. P2300 queries can carry additional arguments to the ``query`` methods. We
   don't yet need this functionality, so it is omitted.
