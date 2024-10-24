##################################
Declaratively Parsing BSON Objects
##################################

|amongoc|'s BSON library contains a submodule for declaratively decomposing
BSON values and heirarchies declaratively.


High-Level Constructs
#####################

Parse State
***********

A parse rule has one of three result states after being tested on a value:

acceptance
  The rule successfully parsed the given value.

local error / soft failure / reject
  The rule rejects the given value. This will not necessarily reject the entire
  value.

global error / hard failure / error
  The rule rejects the given value, and all parent rules should reject
  immediately.


Parser Rules
************

.. struct::
  bson::parse::just_accept
  bson::parse::just_reject
  bson::parse::reject_others

  These special rules will parse anything and immediately *accept* or *reject*,
  respectively.

  These are useful e.g. to test for the existence/absence of a document element
  without inspecting its value.

  The `reject_others` rule is used with the `bson::parse::doc` rule combinator
  to enforce that the document does not contain any elements that were otherwise
  unexpected.


.. struct::
  template <typename T, rule<T> R> bson::parse::type_rule

  A :expr:`rule<bson_iterator::reference>` that checks that a document elements
  contains a value convertible to `T`, and that the value matches rule `R`. The
  wrapped rule will be invoked with the converted instance of `T`.

  Use the shorthand `bson::parse::type` to create `type_rule` instances.


.. function::
  template <typename T, rule<T> R = just_accept> \
  auto bson::parse::type(R&& rule = {}) -> type_rule<T, R>

  Create a `type_rule` rule for the type `T` and the parsing rule `rule`. If no
  `rule` is given, then the type rule will simply check the type without
  inspecting the value.


.. struct::
  template <typename F> bson::parse::action

  A parser rule that executes the function `F` with the value being inspected.
  Always accepts its argument


.. struct::
  template <typename T> bson::parse::store

  A parser rule that stores its operand in a target. The type `T` should usually
  be an l-value reference. If `T` is not just a `bson_iterator::reference`, then
  the rule will also check that the operand is of the correct type.

  Upon storing, accepts.


.. struct::
  template <typename R> bson::parse::must

  A special rule combinator that upgrades parse rejections to parse errors. If
  the wrapped rule rejects, then the resulting error becomes a global failure.

  .. note:: This rule has special meaning when used with `doc`


.. struct::
  template <rule<bson_iterator::reference> R> bson::parse::field

  Searches for an element within a document with the given key and matching the
  given rule `R`. This rule can be invoked with either an element reference to
  test a single element, or a `bson_view` to search for an element with the
  expected key.

  .. function:: field(std__string_view key, R r)

    Construct a field rule that searches for an element with `key` and matching
    the rule `r`.


.. function::
  template <rule<reference> R> must<field<R>> bson::parse::require(std__string_view key, R&& rule)

  Shorthand to create a :expr:`must<field<R>>` rule.


.. struct::
  template <typename... Rs> bson::parse::any

  A special rule combinator that accepts if any of its sub-rules accepts. Each
  sub-rule is tried in the order they are given. This is a short-circuiting
  operation: When any sub-rule accepts or errors, then the `any` rule
  immediately accepts or errors. If *all* sub-rules reject, then `any` will also
  reject.

  A generated rejection message will explain why each sub-rule was rejected.


.. struct::
  template <typename... Rs> bson::parse::all

  A rule combinator that accepts only if *all* of its sub-rules accept. Each
  sub-rule in `Rs` is tried in the order they are given as arguments. The
  operation is short-circuiting: If any sub-rule rejects or errors, no
  subsequent rules will be attempted. If any sub-rules rejects, then `all`
  will also reject.

.. struct::
  template <rule<reference> R> bson::parse::each

  A rule combinator for arrays or documents treated as an array of elements.

  For each element, tests the given rule. Stops and rejects if any sub-rule
  rejects.


.. struct::
  template <typename R> bson::parse::maybe

  A rule combinator that accepts if the inner rule does *not* generate a *hard
  failure*.


.. struct::
  template <rule<reference>... Rs> bson::parse::doc

  A parsing rule that expects a document or array and decomposes it according to
  the rules `Rs`.

  Given a document or array |D|:

  - For each element |E| in |D|:

    - If all rules in `Rs` have accepted an element, **accept** |D| and **stop**
      (parsing stops once all rules are satisfied)
    - For each rule |R| in `Rs`:

      - If |R| has already accepted any previous element, **skip** |R| (a rule
        will only be tried until it accepts at most once).
      - If |R| is a `must` rule that wraps a rule |R_1|, let |R_1| take the
        place of |R| in the following steps (unwraps a `must` rule: `must` is
        handled after the full loop over |D|).
      - If |R| **accepts** |E|, **skip** to the next element in |D| and restart
        the loop over `Rs`.
      - If |R| **errors** on |E|, **error** on |D| and **stop**.
      - If |R| is `reject_others`, **reject** |D| and **stop**. (`reject_others`
        should only be used as the final rule in `doc`, otherwise subsequent
        rules will be unreachable)
      - If |R| **rejects** |E|, continue.

    - (If no rule in `Rs` accepted or errored on |E|, then |E| will just be ignored.)

  - Finally, for each rule |R| in `Rs`:

    - If |R| is a `must` rule and it did not accept any element in |D|,
      **reject** |D| completely.

    .. note:: If any other rule in `Rs` did not match anything in |D|, the rule
      is silently ignored.

    .. note:: If `reject_others` is not present in the ruleset, unmatched elements
      are silently ignored.


Utilities
*********

.. function::
  template <typename T, rule<T> R> \
  void bson::parse::must_parse(const T& value, R&& rule)

  Attempt to parse `value` using `rule`. If the parser rejects or errors, throws
  a `std__system_error` with ``EPROTO`` containing the error message string
  describing the failure.


.. function::
  template <result_type Res> \
  std__string bson::parse::describe_error(const Res& res)

  Generate a string that describes the result `res` which was returned by a
  parse rule.


Low-Level Concepts
##################

.. concept:: template <typename R, typename Arg> bson::parse::rule

  A type `T` satisfies the requirements for `rule` if it is *invocable* with an
  `Arg` instance and such an invocation returns a type that satisfies
  `result_type`.


.. concept::
    template <typename T> bson::parse::result_type

  A type that is returned by a `rule` for attempting to perform a parse.

  .. rubric:: Given
  .. var::
    const T result
    std::output_iterator<char> auto out

    **Requires**:

    - :expr:`result.state()` returns a `pstate` value.
    - :expr:`result.format_to(out)` returns the same type as `out`. This
      function may write characters into the given output iterator to describe
      the result (e.g. an error message).


.. enum:: bson::parse::pstate

  .. enumerator::
    reject
    accept
    error

    Rule result states for local rejection, success, and global errors,
    respectively.
