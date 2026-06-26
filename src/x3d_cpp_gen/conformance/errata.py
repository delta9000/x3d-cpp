"""Cited, version-keyed corrections for known errata in the ISO UOM source.

The UOM is a Web3D-generated artifact and occasionally carries data bugs (a field's
accessType wrong in one revision, fixed in the next). Hand-patching the extracted
manifest would break the moat contract that the committed criteria JSON is
byte-faithful to the UOM (its `uomManifestHash` derives from the pure extraction).

Instead, errata are an OVERLAY applied at load time (`version_resolve.load_manifest`),
never written back to disk. Each erratum is GUARDED: it only fires when the field's
current value matches `from` (and the type guard, if any), so it is *self-disabling*
— the day Web3D fixes the source UOM and we re-extract, the guard misses and the
overlay becomes a no-op. Every erratum carries evidence + the revision that corrected
it, so the applied set is itself a citable conformance artifact.

To add an erratum: append a record below with full provenance. Keep them rare and
evidence-backed — this is a correction list for *proven* source bugs, not a place to
encode opinions about the spec.
"""
from __future__ import annotations

from typing import Any, Dict, List

# Each record: which version's manifest, which (node, field, attr) to correct, the
# guarded `from`→`to` change, an optional `type` guard, and full provenance.
ERRATA: List[Dict[str, Any]] = [
    {
        "version": "3.0",
        "nodes": ["Viewpoint", "GeoViewpoint"],  # both flatten X3DViewpointNode.orientation
        "field": "orientation",
        "attr": "accessType",
        "from": "initializeOnly",
        "to": "inputOutput",
        "type": "SFRotation",  # distinguishes from Extrusion.orientation (inputOnly MFRotation)
        "reason": (
            "X3D 3.0 UOM erratum: X3DViewpointNode.orientation declared "
            "initializeOnly but is inputOutput — the field is routable "
            "(animated viewpoints) and carries set_orientation/orientation_changed "
            "event aliases."
        ),
        "corrected_in": "3.1",
        "evidence": (
            "X3dUnifiedObjectModel-3.0.xml Viewpoint.orientation accessType="
            "initializeOnly vs 3.1 inputOutput; set_orientation + orientation_changed "
            "present as field-name enumerations in the same 3.0 UOM; 11 official "
            "3.0 corpus files route through it."
        ),
    },
]


def errata_for(version: str) -> List[Dict[str, Any]]:
    """The erratum records targeting this version's manifest (unfiltered by guard)."""
    return [e for e in ERRATA if e["version"] == version]


def apply_errata(version: str, nodes: Dict[str, Any]) -> List[Dict[str, Any]]:
    """Apply this version's errata to a manifest `nodes` dict IN PLACE.

    Returns the list of corrections actually applied (guard matched). An erratum
    whose guard misses (field absent, value already corrected, or type mismatch)
    applies nothing and is omitted from the result — keeping the overlay safe and
    self-disabling.
    """
    applied: List[Dict[str, Any]] = []
    for e in errata_for(version):
        for node_name in e["nodes"]:
            field = nodes.get(node_name, {}).get("fields", {}).get(e["field"])
            if field is None:
                continue
            if "type" in e and field.get("type") != e["type"]:
                continue
            if field.get(e["attr"]) != e["from"]:
                continue  # guard miss → self-disabling no-op
            field[e["attr"]] = e["to"]
            applied.append({**e, "node": node_name})
    return applied
