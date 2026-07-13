"""
MkDocs hook: remove the built-in fenced_code extension so that
pymdownx.superfences can handle fenced code blocks instead.
"""


def on_config(config):
    exts = config["markdown_extensions"]
    if "fenced_code" in exts:
        exts.remove("fenced_code")
    return config
