"""First-class X3D specification version.

A :class:`SpecVersion` bundles the version string (e.g. ``"4.0"`` / ``"4.1"``)
with the spec XML path it came from. The version is AUTO-DETECTED from the UOM
XML — it lives on the document root as the ``version`` attribute of the
``<X3dUnifiedObjectModel>`` element, e.g.::

    <X3dUnifiedObjectModel ... version="4.0" ...>

so a newer spec is ingested without any code change: detection is data-driven.

The version is threaded through parse -> model -> emit. Nothing in the pipeline
is logically locked to 4.0; 4.0 is merely the packaged DEFAULT spec path.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

from lxml import etree


@dataclass(frozen=True)
class SpecVersion:
    """An X3D spec version plus the path to the UOM XML it was read from."""

    version: str
    spec_path: Optional[Path] = None

    @property
    def major(self) -> int:
        return int(self.version.split(".")[0])

    @property
    def minor(self) -> int:
        parts = self.version.split(".")
        return int(parts[1]) if len(parts) > 1 else 0

    @property
    def slug(self) -> str:
        """Filesystem/namespace-safe form, e.g. ``"4.1"`` -> ``"4_1"``."""
        return self.version.replace(".", "_").replace("-", "_")

    @property
    def cpp_namespace(self) -> str:
        """Suggested C++ namespace component, e.g. ``"v4_1"``."""
        return f"v{self.slug}"

    @classmethod
    def detect(cls, spec_path) -> "SpecVersion":
        """Auto-detect the version from a UOM XML file.

        Reads the ``version`` attribute off the document root. Falls back to
        scanning the XML declaration comment (``... version 4.x of X3D``) if the
        attribute is absent, then raises a clear, actionable error.
        """
        path = Path(spec_path)
        version = detect_version_from_file(path)
        if version is None:
            raise ValueError(
                f"Could not auto-detect X3D spec version from {path}: no "
                f"'version' attribute on the <X3dUnifiedObjectModel> root and no "
                f"recognizable version comment. Pass --spec-version explicitly."
            )
        return cls(version=version, spec_path=path)


def detect_version_from_root(root) -> Optional[str]:
    """Return the version string from a parsed UOM root, or ``None``.

    Primary source: the ``version`` attribute on the root element. Fallback:
    the leading ``<!-- ... version 4.x of X3D -->`` descriptive comment.
    """
    attr = root.get("version")
    if attr:
        return attr.strip()
    # Fallback: scan preceding comments for a "version 4.x of X3D" marker.
    for sibling in root.itersiblings(preceding=True):
        if isinstance(sibling, etree._Comment):
            m = re.search(r"version\s+(\d+(?:\.\d+)?)\s+of\s+X3D", sibling.text or "")
            if m:
                return m.group(1)
    return None


def detect_version_from_file(spec_path) -> Optional[str]:
    """Parse a UOM XML file and auto-detect its version string, or ``None``."""
    try:
        tree = etree.parse(str(spec_path))
    except (etree.ParseError, IOError):
        return None
    return detect_version_from_root(tree.getroot())
