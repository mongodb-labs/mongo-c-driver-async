# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import re
from sphinx.application import Sphinx
from sphinx.environment import BuildEnvironment
from sphinx import addnodes

project = "amongoc"
copyright = "2024, MongoDB"
author = "MongoDB"
release = "0.1.0"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = []

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
nitpicky = True

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "sphinx_book_theme"

html_static_path = ["_static"]

rst_prolog = """

.. role:: cpp(code)
    :language: c++

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


def setup(app: Sphinx):
    app.add_object_type(
        "header-file",
        "header-file",
        "pair: header file; %s",
        parse_node=_parse_header_dir,
    )
    app.add_object_type(
        "doc-attr",
        "doc-attr",
        "pair: Documentation attribute; %s",
        parse_node=_parse_meta_attr,
    )
    app.add_css_file("styles.css")
