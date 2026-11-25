"""The X3D version that introduced each node and field (binding model B, ADR-0077).

The bindings are generated from the newest UOM, so they are a superset of every
earlier version. Each node and each of its fields is tagged with the earliest X3D
version that defines it, read from the committed per-version manifests
(``conformance/manifests/x3d-<v>.json``, errata applied). The runtime uses the
tags to warn when a document uses something newer than the version it declares.

A node or field that no committed manifest defines (a synthetic test UOM, or a
spec newer than the last manifest) is tagged with the version being generated.
"""

from __future__ import annotations

from typing import Dict, Tuple

from x3d_cpp_gen.conformance.node_floor import X3D_VERSIONS, version_key
from x3d_cpp_gen.conformance.version_resolve import load_manifest

Version = Tuple[int, int]


def parse_version(v: str) -> Version:
    parts = [int(p) for p in v.split(".")[:2]]
    return (parts[0], parts[1] if len(parts) > 1 else 0)


class VersionFloors:
    """Earliest defining version per node and per (node, field)."""

    def __init__(self, target_version: str):
        self.target: Version = parse_version(target_version)
        self.nodes: Dict[str, Version] = {}
        self.fields: Dict[Tuple[str, str], Version] = {}
        for v in X3D_VERSIONS:  # ascending, so setdefault keeps the earliest
            if version_key(v) > version_key(target_version):
                break
            ver = parse_version(v)
            for name, node in load_manifest(v).nodes.items():
                self.nodes.setdefault(name, ver)
                for field in node.get("fields", {}):
                    self.fields.setdefault((name, field), ver)

    def node(self, name: str) -> Version:
        return self.nodes.get(name, self.target)

    def field(self, node: str, field: str) -> Version:
        return self.fields.get((node, field), self.target)
