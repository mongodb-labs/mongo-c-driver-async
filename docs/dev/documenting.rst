#####################
Documenting |amongoc|
#####################

.. warning:: |devdocs-page|


Authorship Guide
################

Consider the following when writing documentation for |amongoc|:

1. For user-facing documentation, follow the design rules of Diátaxis__ --
   Preserve the separation between the kinds of documentation. We also have this
   "developer documentation" section, which need not follow Diátaxis.
2. For reference material, ensure the |attr.transfer| attribute is always used
   if-and-only-if it applies to a function parameter.
3. Be aware of the reStructuredText substitutions that are defined in the
   ``rst_prolog`` (see ``conf.py``).

   a. Use ``|amongoc|`` when naming the project.
   b. Use e.g. ``|attr.transfer|`` (and other attribute substitutions) when
      refering to documentation attributes.
   c. Use the ``:math:`` substitutions to represent named values that are not
      named by function parameters or member variables.
   d. Include the ``|devdocs-page|`` warning at the top of every developer
      documentation page.

4. Don't duplicate documentation for C APIs in their C++ wrappers. Instead link
   from the C++ API to the C API using a ``:C API:`` field. (e.g.
   `amongoc::unique_handler::complete`)
5. Include a link from C APIs to their C++ equivalent with a ``:C++ API:`` field.
6. Document the members of a type below its documentation fields and description.
7. Separate API descriptions between *brief* description (one or two paragraphs)
   and an *expounding* description, if one is warranted.
8. Add the *brief* description of an entity *above* its documentation fields.
   Add an *expounding* description *below* the documentation fields.
9. If an API allocates memory and the API does not accept an allocator
   parameter, include an ``:allocation:`` field that specifies how memory
   allocation is performed (e.g. `amongoc_tie`).
10. Document header files using :rst:dir:`header-file` directive. Refer to them
    using the :rst:role:`header-file` inline role.
11. Use the ``:header:`` field on an API component to link to the header file
    that defines it. Use the :rst:role:`header-file` role to generate the link.
12. When adding documentation fields to an API, use the following order:

    a. ``:C API:`` / ``:C++ API:``
    b. ``:param:`` fields
    c. ``:return:`` fields
    d. ``:precondition:``
    e. ``:postcondition:``
    f. ``:allocation:``
    g. ``:header:``

13. **Don't** duplicate information from the description in the documentation
    fields. **Don't** include ``:param:`` and ``:return:`` fields if the entire
    API can be easily described in the description alone. These fields should
    instead be used to expression constraints about the values that are not
    obvious from the description or the API signature.
14. **Don't** create multiple documentation entries for very similar function
    overloads. Instead, document them together under the same directive and
    explain the differences between them. (e.g. :c:macro:`amongoc_box_init`)
15. Use admonition directives to announce important things that the reader might
    otherwise miss. Avoid using bold "**NOTE**" and "**WARNING**" inline in the
    text unless using an admonition would seriously break the flow of the text.


Supplements
###########

.. rst:directive:: .. header-file:: <filepath>

  Documents a header file at ``filepath``.

  .. note::

    *Don't* treat this like ``.. class`` and add the API components within the
    body of the directive, as that will lead to excessive indentation in the
    resulting document.

    Instead, use a back-reference on the API components by adding a ``:header:``
    documentation field.

.. rst:directive:: .. doc-attr:: <attr>

  Documents a documentation-only attribute. (e.g. |attr.transfer|, |attr.storage|)

.. rst:role:: header-file

  This inline text role generates a link to a header file documented using the
  :rst:dir:`header-file` directive.

.. rst:role:: doc-attr

  Creates a backlink to a documentation attribute from the :rst:dir:`doc-attr`
  directive.

  .. note:: Prefer to use the ``|attr.xyz|`` substitutions from the ``rst_prolog``, as using this role is cumbersome.

__ https://diataxis.fr/

