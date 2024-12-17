# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import itertools
import os
import re
import zlib
from pathlib import Path
from typing import Any, Callable, Iterable, Literal, NamedTuple

from sphinx import addnodes
from sphinx.application import Sphinx
from sphinx.domains import Domain, ObjType
from sphinx.roles import XRefRole
from sphinx.environment import BuildEnvironment
from pygments.lexer import inherit, words
from pygments.lexers.c_cpp import CppLexer  # type: ignore
from pygments import token

project = "amongoc"
copyright = "2024, MongoDB"
author = "MongoDB"
release = "0.1.0"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    "sphinx.ext.intersphinx",
    "sphinxcontrib.moderncmakedomain",
]


class InvItem(NamedTuple):
    refnames: list[str]
    "One or more names that are valid as link refs for the object"
    role: Literal[
        "cpp:class",
        "cpp:concept",
        "cpp:enum",
        "cpp:function",
        "cpp:member",
        "cpp:type",
        "c:macro",
        "std:term",
        "std:label",
        "std:doc",
    ]
    "The intended link role for the object"
    path: str
    "Relative path to the linked object from the doc root"
    disp_name: str | None = None
    "Display name for the object, if different from the link ref"


CPPREF_INVENTORY: list[InvItem] = [
    # We add a "std__" prefix to the link name for items in the `std`
    # namespace, because the cpp domain will not otherwise attempt to resolve any
    # names that live in the `std::` namespace. Use the `disp_name` to show the
    # proper name of the item instead.
    # XXX: also that hyperlinking seems to fail for templates when template arguments are provided?
    InvItem(
        ["std__coroutine_handle"],
        "cpp:class",
        "cpp/coroutine/coroutine_handle",
        "std::coroutine_handle",
    ),
    InvItem(["std__coroutine_traits"], "cpp:class", "cpp/coroutine/coroutine_traits", "std::coroutine_traits"),
    InvItem(["std__suspend_never"], "cpp:class", "cpp/coroutine/suspend_never", "std::suspend_never"),
    InvItem(["std__suspend_always"], "cpp:class", "cpp/coroutine/suspend_always", "std::suspend_always"),
    InvItem(["std__bad_alloc"], "cpp:class", "cpp/memory/new/bad_alloc", "std::bad_alloc"),
    InvItem(["std__error_code"], "cpp:class", "cpp/error/error_code", "std::error_code"),
    InvItem(["std__system_error"], "cpp:class", "cpp/error/system_error", "std::system_error"),
    InvItem(["std__errc"], "cpp:enum", "cpp/error/errc", "std::errc"),
    InvItem(["std__exception_ptr"], "cpp:class", "cpp/error/exception_ptr", "std::exception_ptr"),
    InvItem(["std__string_view"], "cpp:type", "cpp/string/basic_string_view", "std::string_view"),
    InvItem(["std__string"], "cpp:type", "cpp/string/basic_string", "std::string"),
    InvItem(["std__move"], "cpp:function", "cpp/utility/move", "std::move"),
    InvItem(["std__forward"], "cpp:function", "cpp/utility/forward", "std::forward"),
    InvItem(["size_t"], "cpp:type", "c/types/size_t"),
    InvItem(["std__size_t"], "cpp:type", "cpp/types/size_t", "std::size_t"),
    InvItem(["ptrdiff_t"], "cpp:type", "c/types/ptrdiff_t"),
    InvItem(["std__ptrdiff_t"], "cpp:type", "cpp/types/ptrdiff_t", "std::ptrdiff_t"),
    InvItem(["std__byte"], "cpp:type", "cpp/types/byte", "std::byte"),
    InvItem(["std__forward_iterator"], "cpp:concept", "cpp/iterator/forward_iterator", "std::forward_iterator"),
    InvItem(["std__ranges__forward_range"], "cpp:concept", "cpp/ranges/forward_range", "std::ranges::forward_range"),
    InvItem(["timespec"], "cpp:class", "c/chrono/timespec"),
    InvItem(["c/language/value_category"], "std:doc", "c/language/value_category", "Value categories (C)"),
    InvItem(["c/preprocessor/replace"], "std:doc", "c/preprocessor/replace", "Replacing test macros (C)"),
    InvItem(["cpp/language/value_category"], "std:doc", "cpp/language/value_category", "Value categories (C++)"),
    InvItem(["cpp/language/language_linkage"], "std:doc", "cpp/language/language_linkage", "Language linkage (C++)"),
    InvItem(
        ["cpp/language/elaborated_type_specifier"],
        "std:doc",
        "cpp/language/elaborated_type_specifier",
        "Elaborated type specifier (C++)",
    ),
    *itertools.chain.from_iterable(
        (
            InvItem([itype], "cpp:type", "c/types/integer", itype),
            InvItem([f"std__{itype}"], "cpp:type", "cpp/types/integer", f"std::{itype}"),
        )
        for itype in (
            "int8_t",
            "uint8_t",
            "int16_t",
            "uint16_t",
            "int32_t",
            "uint32_t",
            "int64_t",
            "uint64_t",
        )
    ),
]


def generate_sphinx_inventory(
    out_inv: os.PathLike[str] | str,
    project_name: str,
    project_version: str,
    items: Iterable[InvItem],
):
    """
    Generate a Sphinx object inventory for use with intersphinx.

    The Sphinx inventory format is under-documented, but seems to be fairly stable.
    See: https://sphobjinv.readthedocs.io/en/stable/syntax.html
    """
    dat = zlib.compress(
        b"".join(
            b"".join(f"{refname} {i.role} 1 {i.path} {i.disp_name or refname}\n".encode() for refname in i.refnames)
            for i in items
        )
    )
    newdat = (
        b"# Sphinx inventory version 2\n"
        + f"# Project: {project_name}\n".encode()
        + f"# Version: {project_version}\n".encode()
        + b"# The remainder of this file is compressed using zlib.\n"
        + dat
    )
    out_inv = Path(out_inv)
    try:
        cur = out_inv.read_bytes()
    except FileNotFoundError:
        cur = b""
    if newdat != cur:
        out_inv.write_bytes(newdat)


CPPREF_GENERATED_INV = "cppref.generated.inv"
generate_sphinx_inventory(CPPREF_GENERATED_INV, "cppreference", "0", CPPREF_INVENTORY)

intersphinx_mapping = {
    "cmake": ("https://cmake.org/cmake/help/latest", None),
    "cppref": ("https://en.cppreference.com/w/", CPPREF_GENERATED_INV),
    "mongodb": ("https://www.mongodb.com/docs/manual", None),
}

templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

cpp_index_common_prefix = [
    "amongoc_",
    "amongoc::",
]
cpp_id_attributes = []
cpp_maximum_signature_line_length = 80

toc_object_entries_show_parents = "hide"

highlight_language = "c++"
primary_domain = "cpp"
default_role = "any"

# XXX: Disabled until https://github.com/sphinx-doc/sphinx/issues/12845 is fixed
# nitpicky = True

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "sphinx_book_theme"
html_theme_options = {
    "path_to_docs": "docs",
    "repository_branch": "develop",
    "repository_url": "https://github.com/mongodb-labs/mongo-c-driver-async",
    "use_edit_page_button": True,
    "use_issues_button": True,
    "use_repository_button": True,
    "use_source_button": True,
}

html_static_path = ["_static"]

rst_prolog = """

.. role:: cpp(code)
    :language: c++

.. role:: sh(code)
    :language: sh

.. role:: project-name(literal)
    :class: project-name

.. role:: dbcommand(mongodb:dbcommand)

.. |amongoc| replace:: :project-name:`amongoc`
.. |attr.transfer| replace:: :doc-attr:`[[transfer]]`
.. |attr.type| replace:: :doc-attr:`[[type(…)]] <[[type(T)]]>`
.. |attr.storage| replace:: :doc-attr:`[[storage]]`
.. |attr.nullable| replace:: :doc-attr:`[[nullable]]`
.. |attr.zero-init| replace:: :doc-attr:`[[zero_initializable]]`
.. |zero-initialized| replace:: :ref:`zero-initialized <zero-init>`
.. |C++ API| replace:: :strong:`(C++ API)`

.. |devdocs-page| replace::

    This page is for |amongoc| developers and the documented components and
    behavior are not part of any public API guarantees.

.. |inline| replace:: :cpp:`inline`
.. |extern| replace:: :cpp:`extern`
.. |static| replace:: :cpp:`static`
.. |const| replace:: :cpp:`const`
.. |void| replace:: :cpp:`void`
.. |A| replace:: :math:`A`
.. |A'| replace:: :math:`A'`
.. |B| replace:: :math:`B`
.. |D| replace:: :math:`D`
.. |D'| replace:: :math:`D'`
.. |E| replace:: :math:`E`
.. |F| replace:: :math:`F`
.. |H| replace:: :math:`H`
.. |I| replace:: :math:`I`
.. |M| replace:: :math:`M`
.. |Q| replace:: :math:`Q`
.. |R| replace:: :math:`R`
.. |R_1| replace:: :math:`R_1`
.. |S| replace:: :math:`S`
.. |T| replace:: :math:`T`
.. |V| replace:: :math:`V`
.. |R'| replace:: :math:`R'`
.. |S'| replace:: :math:`S'`
.. |O'| replace:: :math:`O'`
.. |S_ret| replace:: :math:`S_{\\tt ret}`
.. |x| replace:: :math:`x`
.. |true| replace:: :cpp:`true`
.. |false| replace:: :cpp:`false`

.. |macro-impl| replace:: This is implemented as a C preprocessor macro
.. |iff| replace:: *if-and-only-if*

"""


def _parse_header_dir(env: BuildEnvironment, name: str, node: addnodes.desc_signature) -> str:
    node += addnodes.desc_addname("", "<")
    node += addnodes.desc_name(name, name)
    node += addnodes.desc_addname("", ">")
    node += addnodes.desc_sig_space()
    node += addnodes.desc_annotation("", "(header file)")
    return name


def _parse_meta_attr(env: BuildEnvironment, name: str, node: addnodes.desc_signature) -> str:
    mat = re.match(r"^\[\[(.*)\]\]$", name)
    if not mat:
        raise ValueError(f"Invalid doc-attr name: {name}")
    spel = mat[1]
    node += addnodes.desc_addname("", "[[")
    node += addnodes.desc_name("", spel)
    node += addnodes.desc_addname("", "]]")
    node += addnodes.desc_sig_space()
    node += addnodes.desc_annotation("", "(documentation attribute)")
    return name


def annotator(
    annot: str,
) -> Callable[[BuildEnvironment, str, addnodes.desc_signature], str]:
    """
    Create a parse_node function that adds a parenthesized annotation to an object signature.
    """

    def parse_node(env: BuildEnvironment, sig: str, signode: addnodes.desc_signature) -> str:
        signode += addnodes.desc_name(sig, sig)
        signode += addnodes.desc_sig_space()
        signode += addnodes.desc_annotation("", f"({annot})")
        return sig

    return parse_node


def parse_earthly_artifact(env: BuildEnvironment, sig: str, signode: addnodes.desc_signature) -> str:
    """
    Parse and render the signature of an '.. earthly-artifact::' signature"""
    mat = re.match(r"(?P<target>\+.+?)(?P<path>/.*)$", sig)
    if not mat:
        raise RuntimeError(f"Invalid earthly-artifact signature: {sig!r} (expected “+<target>/<path> string)")
    signode += addnodes.desc_addname(mat["target"], mat["target"])
    signode += addnodes.desc_name(mat["path"], mat["path"])
    signode += addnodes.desc_sig_space()
    signode += addnodes.desc_annotation("", "(Earthly artifact)")
    return sig


class MongoDBDomain(Domain):
    name = "mongodb"
    object_types = {
        "dbcommand": ObjType("Database Command", "dbcommand"),
    }
    roles = {
        "dbcommand": XRefRole(),
    }

    def merge_domaindata(self, docnames: list[str], otherdata: dict[str, Any]) -> None:
        pass


# Special identifiers that we want to be lexed and highlighted:
_TYPES = [
    *(
        f"amongoc_{t}"
        for t in (
            "box",
            "status",
            "loop",
            "emitter",
            "operation",
            "handler",
            "default_loop",
            "client",
            "collection",
        )
    ),
    *(
        f"bson_{t}"
        for t in (
            "doc",
            "view",
            "mut",
            "value",
            "value_ref",
            "iterator",
        )
    ),
    *(
        f"mlib_{t}"
        for t in (
            "str",
            "str_view",
            "duration",
        )
    ),
]
_FUNCS = [
    *(
        f"amongoc_{f}"
        for f in (
            "just",
            "let",
            "start",
            "tie",
            "then",
            "timeout",
            "schedule_later",
            "is_error",
        )
    ),
    *(
        f"bson_{f}"
        for f in (
            "new",
            "delete",
            "as_view",
            "mutate",
            "insert",
            "data",
            "size",
            "ssize",
            "begin",
            "end",
            "key",
            "key_eq",
        )
    ),
]
_CONSTANTS = [
    *(f"amongoc_{c}" for c in ("okay", "nil")),
    *(f"mlib_{c}" for c in ("default_allocator", "terminating_allocator")),
    *(f"bson_{c}" for c in ("view_null", "array_view_null")),
]


class CustomCppLexer(CppLexer):
    tokens = {
        "keywords": [
            (words(_CONSTANTS, suffix=r"\b"), token.Name.Constant),
            (words(_TYPES, suffix=r"\b"), token.Keyword.Type),
            (words(_FUNCS, suffix=r"\b"), token.Name.Function),
            # Lex every other name of the form `<type>_<words>` as a function name
            (rf"{words(_TYPES).get()}_\w+\b", token.Name.Function),  # type: ignore
            # Enumerators
            (r"amongoc_async_\w+\b", token.Name.Constant),
            (r"amongoc_\w_errc_\w+\b", token.Name.Constant),
            (r"bson_type_\w+\b", token.Name.Constant),
            # Other macro
            (r"bson_foreach\w*\b", token.Keyword),
            inherit,
        ],
    }


def setup(app: Sphinx):
    app.add_object_type(  # type: ignore
        "header-file",
        "header-file",
        "pair: header file; %s",
        parse_node=_parse_header_dir,
    )
    app.add_object_type(  # type: ignore
        "doc-attr",
        "doc-attr",
        "pair: Documentation attribute; %s",
        parse_node=_parse_meta_attr,
    )
    app.add_css_file("styles.css")
    app.add_object_type(  # type: ignore
        "earthly-target",
        "earthly-target",
        indextemplate="pair: Earthly target; %s",
        parse_node=annotator("Earthly target"),
    )
    app.add_object_type(  # type: ignore
        "earthly-artifact",
        "earthly-artifact",
        indextemplate="pair: Earthly artifact; %s",
        parse_node=parse_earthly_artifact,
    )
    app.add_object_type(  # type: ignore
        "file",
        "file",
        indextemplate="repository file; %s",
        parse_node=annotator("repository file"),
    )
    app.add_domain(MongoDBDomain)
    app.add_lexer("c", CustomCppLexer)
    app.add_lexer("cpp", CustomCppLexer)
    app.add_lexer("c++", CustomCppLexer)
