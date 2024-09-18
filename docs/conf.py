# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import re
from typing import Callable

from sphinx import addnodes
from sphinx.application import Sphinx
from sphinx.environment import BuildEnvironment

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

intersphinx_mapping = {
    "cmake": ("https://cmake.org/cmake/help/latest", None),
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

html_static_path = ["_static"]

rst_prolog = """

.. role:: cpp(code)
    :language: c++

.. role:: sh(code)
    :language: sh

.. role:: project-name(literal)
    :class: project-name

.. |amongoc| replace:: :project-name:`amongoc`
.. |attr.transfer| replace:: :doc-attr:`[[transfer]]`
.. |attr.type| replace:: :doc-attr:`[[type(…)]] <[[type(T)]]>`
.. |attr.storage| replace:: :doc-attr:`[[storage]]`
.. |C++ API| replace:: :strong:`(C++ API)`

.. |devdocs-page| replace::

    This page is for |amongoc| developers and the documented components and
    behavior are not part of any public API guarantees.

.. |A| replace:: :math:`A`
.. |A'| replace:: :math:`A'`
.. |B| replace:: :math:`B`
.. |E| replace:: :math:`E`
.. |H| replace:: :math:`H`
.. |Q| replace:: :math:`Q`
.. |R| replace:: :math:`R`
.. |S| replace:: :math:`S`
.. |T| replace:: :math:`T`
.. |V| replace:: :math:`V`
.. |R'| replace:: :math:`R'`
.. |S'| replace:: :math:`S'`
.. |O'| replace:: :math:`O'`
.. |S_ret| replace:: :math:`S_{\\tt ret}`
.. |x| replace:: :math:`x`

"""


def _parse_header_dir(
    env: BuildEnvironment, name: str, node: addnodes.desc_signature
) -> str:
    node += addnodes.desc_addname("", "<")
    node += addnodes.desc_name(name, name)
    node += addnodes.desc_addname("", ">")
    node += addnodes.desc_sig_space()
    node += addnodes.desc_annotation("", "(header file)")
    return name


def _parse_meta_attr(
    env: BuildEnvironment, name: str, node: addnodes.desc_signature
) -> str:
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

    def parse_node(
        env: BuildEnvironment, sig: str, signode: addnodes.desc_signature
    ) -> str:
        signode += addnodes.desc_name(sig, sig)
        signode += addnodes.desc_sig_space()
        signode += addnodes.desc_annotation("", f"({annot})")
        return sig

    return parse_node


def parse_earthly_artifact(
    env: BuildEnvironment, sig: str, signode: addnodes.desc_signature
) -> str:
    """
    Parse and render the signature of an '.. earthly-artifact::' signature"""
    mat = re.match(r"(?P<target>\+.+?)(?P<path>/.*)$", sig)
    if not mat:
        raise RuntimeError(
            f"Invalid earthly-artifact signature: {sig!r} (expected “+<target>/<path> string)"
        )
    signode += addnodes.desc_addname(mat["target"], mat["target"])
    signode += addnodes.desc_name(mat["path"], mat["path"])
    signode += addnodes.desc_sig_space()
    signode += addnodes.desc_annotation("", "(Earthly artifact)")
    return sig


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
