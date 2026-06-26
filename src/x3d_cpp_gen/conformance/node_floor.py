"""Earliest-defining-version per node, derived from the committed per-version
manifests (data only — no UOM re-parse). Consumed by the VP-2 inference ladder's
node-floor rung (§1 step 2)."""
from __future__ import annotations

import functools
from typing import Dict, Tuple

from x3d_cpp_gen.conformance.version_resolve import load_manifest

# Ascending; the only X3D versions with a committed manifest. Floor = first entry.
X3D_VERSIONS = ["3.0", "3.1", "3.2", "3.3", "4.0", "4.1"]
FLOOR_VERSION = X3D_VERSIONS[0]


def version_key(v: str) -> Tuple[int, ...]:
    """Numeric sort key so '3.10' > '3.3' (lexicographic would invert them)."""
    return tuple(int(p) for p in v.split("."))


@functools.lru_cache(maxsize=1)
def node_floor_map() -> Dict[str, str]:
    """{nodeName: earliest X3D version (of X3D_VERSIONS) whose manifest defines it}."""
    floor: Dict[str, str] = {}
    for v in X3D_VERSIONS:  # ascending → setdefault keeps the earliest
        for name in load_manifest(v).nodes:
            floor.setdefault(name, v)
    return floor
