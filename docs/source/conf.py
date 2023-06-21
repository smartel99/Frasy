# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = "Frasy"
copyright = "2023, Samuel Martel, Paul Thomas"
author = "Samuel Martel, Paul Thomas"
release = "0.1"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    "sphinx.ext.todo",
    "sphinxcontrib.luadomain",
    "sphinx_lua",
    "sphinx.ext.autodoc",
    "sphinx.ext.autosummary",
    "sphinx_rtd_theme",
]

templates_path = ["_templates"]
exclude_patterns = []

todo_include_todos = True
numfig = True

github_url = "https://github.com/smartel99/Frasy"

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "sphinx_rtd_theme"
html_static_path = ["_static"]

html_logo = "assets/img/frasy_logo.svg"
html_favicon = "assets/img/icon.ico"

html_theme_options = {
    "navigation_with_keys": True,
}


# -- Lua configuration
lua_source_path = ["../Frasy/lua/"]
lua_source_encoding = "utf8"
lua_source_comment_prefix = "---"
lua_source_use_emmy_lua_syntax = True
lua_source_private_prefix = "_"
