"""Meta-validator: no Manifest is admitted unless the UOM it came from has the
shape the extractor depends on. The single guard against silent oracle rot — a
drifted UOM schema extracts a SMALLER, more-permissive manifest, which without
this check is invisible (a green run against a subtly-wrong oracle)."""
from __future__ import annotations

import re
from pathlib import Path

from x3d_cpp_gen.conformance.manifest import Manifest
from x3d_cpp_gen.generator import FIELD_TYPE_MAPPING, XS_TYPES

# Per-version node-count floors. A manifest below its floor is rejected: this is
# the only defense against `findall == []` looking identical to an empty section.
NODE_COUNT_FLOOR = {"4.0": 240, "4.1": 240, "3.3": 200, "3.2": 200, "3.1": 200, "3.0": 180}
DEFAULT_FLOOR = 180


class MetaValidationError(Exception):
    pass


def validate_manifest(m: Manifest) -> None:
    floor = NODE_COUNT_FLOOR.get(m.uom_version, DEFAULT_FLOOR)
    if len(m.nodes) < floor:
        raise MetaValidationError(
            f"node-count floor: x3d-{m.uom_version} extracted {len(m.nodes)} "
            f"nodes (< floor {floor}) — likely a drifted/renamed UOM element"
        )
    # Shape: a meaningful fraction of nodes must carry a component, and every node
    # must carry a fields dict. Absent component on ALL nodes => the componentInfo
    # XPath drifted.
    with_component = sum(1 for n in m.nodes.values() if n.get("component"))
    if with_component == 0:
        raise MetaValidationError(
            f"shape: zero nodes carry component info in x3d-{m.uom_version} — "
            f"componentInfo extraction drifted"
        )
    for name, n in m.nodes.items():
        if "fields" not in n or not isinstance(n["fields"], dict):
            raise MetaValidationError(f"shape: node {name!r} has no fields dict")


def assert_field_types_mapped(uom_file: str) -> None:
    """Strict-mode extractor guard: every <field type=...> in the UOM must be a type
    the extractor models. The lenient parser SILENTLY DROPS an unmapped type (it
    `print`+`continue`s), shrinking the oracle invisibly — so when admitting a
    manifest we scan for any unmapped type and raise, forcing a deliberate extractor
    extension when a future spec introduces a new field type."""
    text = Path(uom_file).read_text()
    types = set(re.findall(r'<field[^>]*\btype="([^"]+)"', text))
    unmapped = sorted(t for t in types if t not in FIELD_TYPE_MAPPING and t not in XS_TYPES)
    if unmapped:
        raise MetaValidationError(
            f"unmapped field types in {uom_file}: {unmapped} — the lenient extractor "
            f"would SILENTLY DROP these fields; extend FIELD_TYPE_MAPPING / XS_TYPES"
        )
