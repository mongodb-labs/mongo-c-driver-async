#############
``result<T>``
#############

.. namespace:: amongoc
.. class::
  template <typename T, typename E = amongoc_status> \
  result

  Implements a sum type over a success type `T` and an error type `E`. A result
  |R| holds either an instance of `T` or an instance of `E` (reference types
  *are* supported!)

  .. type::
    success_type = T
    error_type = T

    The success type and error type of the result, respectively.

  .. function:: result()

    Default-construct to a successful value, if possible.

  .. function:: template <typename U> \
    result(U&& arg)

    Conditionally-explicit converting constructor. Requires that `U` is
    explicit-convertbile to `T` *but not* explicit-convertible to `E`. This
    constructor is explicit unless `U` is implicit-convertbile to `T`.

  .. function::
    template <typename... Args> \
    result(success_tag<Args...> tag)
    template <typename... Args> \
    result(error_tag<Args...> tag)

    In-place constructs a success value or an error value, respectively.

    :param tag: The tag contains a tuple of bound constructor arguments that will
      be forwarded to consrtuct the underlying object.

    This requires that the corresponding contained type be constructible from
    the argument types that are bound within the tag.

  .. function::
    bool has_value() const
    bool has_error() const

    Test whether the result contains a value or an error, respectively.

  .. function::
    auto error_tag()

    Create an `amongoc::error_tag` object that can be used to construct a new
    `result` containing the same error as this result.

    :precondition: :expr:`has_error() == true`

    This function is cvref-overloaded to perfect-forward the contained error.

  .. function::
    auto value()
    auto operator*()

    Obtain the contained value, or throw an exception if the result is errant.

    :throw: Throws an error according to `error_traits`

    This function is cvref-overloaded to perfect-forward the contained value.

  .. function::
    auto error()

    Obtain the contained error.

    :precondition: :expr:`has_error() == true`

    This function is cvref-overloaded to perfect-forward the contained error.


.. class::
  template <typename...> success_tag
  template <typename...> error_tag

  Tag types that are used to construct a `result` with a set of arguments.

.. function::
  auto success(auto&&... args)
  auto error(auto&&... args)

  Returns a `success_tag` or `error_tag`, respectively, with the given
  argumounts bound. When the tags are then converted to a `result`, the bound
  arguments are used to construct the underlying success/error value for that
  `result`.

  .. note::

    The arguments are stored within the tag by-reference (including by r-value
    reference), so the tag should be immediately used to construct a `result` to
    avoid dangling references.


.. struct::
  template <typename E> error_traits

  Determines how to throw an exception for a `result`.

  .. function:: [[noreturn]] static void throw_exception(const E& e)

    Throws an exception based on `e`

  The following specializations are provided for `E`:

  `std__error_code`
    Throws a `std__system_error`

  `std__exception_ptr`
    Calls `std::rethrow_exception`

  Any type derived from `std::exception`
    Simply throws the exception

  `amongoc_status`
    Throws an `amongoc::exception` for the status


`result` Combinators and Helpers
################################

.. class::
  template <typename F> result_fmap

  Provides a monadic fmap-style combinator for the given function `F`.

  .. function::
    template <typename T, typename E> \
      requires std::invocable<F, T> \
    auto operator()(result<T, E> res) -> result<std::invoke_result_t<F, T>, E>

    Call the fmap object with a result `res` with a value type `T` and error
    type `E`.

    Requires that the wrapped `F` be invocable with a `T` and return a new value
    of type |R| (which may be a reference or ``void``).

    - If `res` has a value, then the underlying `F` will be invoked with that
      value, and the result will be wrapped in a new `result\<R, E>`.
    - If `res` *does not* hold a value, then `F` will not be invoked, and the
      error in `res` will be used to initialize a `result\<R, E>` with an errant
      state.

.. function::
  template <typename T, typename E> \
  result<T, E> result_join(result<result<T, E>, E> r)

  "Flattens" a result-of-result `r`.

  .. hint:: This is an invocable object type with the above signature, not an actual function template.


.. class::
  template <typename T, typename E> \
    requires nanosender<T> \
  nanosender_traits<result<T, E>>

  A specialization of `amongoc::nanosender_traits` that allows a result to act
  as a nanosender if its success value type is itself a nanosender.
