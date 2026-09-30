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


# Fields the published standard defines but the vendored UOM omits. Same overlay
# discipline as ERRATA (in-memory, never written back, self-disabling: an addition
# whose field already exists on the node is a no-op), but ERRATA rewrites an existing
# attribute and cannot express "this field is missing". Each record must cite the
# published text; `description` reproduces the standard's own wording, not a paraphrase.
FIELD_ADDITIONS: List[Dict[str, Any]] = [
    {
        "version": "4.0",
        "nodes": ["WaveShaper"],
        "field": "curve",
        "type": "MFFloat",
        "accessType": "inputOutput",
        "default": None,  # the standard's default is the empty array
        "description": (
            "The curve field is an array of floating-point numbers describing the "
            "distortion to apply."
        ),
        "source": (
            "ISO/IEC 19775-1 (X3D v4.0) §16.4.21 WaveShaper: "
            "`MFFloat [in,out] curve [] [-1,-1]`"
        ),
        "source_url": (
            "https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/"
            "Part01/components/sound.html"
        ),
        "reason": (
            "X3D 4.0 UOM omission: WaveShaper declares `oversample` but not `curve`, "
            "so a scene cannot author the transfer curve and the corpus audit flags "
            "WaveShaper.curve as FIELD_UNKNOWN_FOR_NODE."
        ),
        "evidence": (
            "X3dUnifiedObjectModel-4.0.xml ConcreteNode WaveShaper has no `curve` field; "
            "the standard's field table for the same node (the UOM's own "
            "specificationUrl) lists it as MFFloat [in,out]."
        ),
        "corrected_in": None,  # not fixed in the vendored 4.0 UOM; guard makes this self-disabling
    },
]


def additions_for(version: str) -> List[Dict[str, Any]]:
    """The field-addition records targeting this version (unfiltered by guard)."""
    return [a for a in FIELD_ADDITIONS if a["version"] == version]


def apply_field_additions_manifest(version: str, nodes: Dict[str, Any]) -> List[Dict[str, Any]]:
    """Add omitted fields to a manifest `nodes` dict IN PLACE; returns those applied.

    A node that is absent, or already has the field, is skipped (self-disabling).
    Field records use the manifest shape written by conformance.manifest."""
    applied: List[Dict[str, Any]] = []
    for a in additions_for(version):
        for node_name in a["nodes"]:
            fields = nodes.get(node_name, {}).get("fields")
            if fields is None or a["field"] in fields:
                continue
            fields[a["field"]] = {
                "acceptableNodeTypes": None,
                "accessType": a["accessType"],
                "default": a["default"],
                "maxInclusive": None,
                "minInclusive": None,
                "simpleType": None,
                "type": a["type"],
            }
            applied.append({**a, "node": node_name})
    return applied


def apply_field_additions_model(version: str, nodes: Dict[str, Any]) -> List[Dict[str, Any]]:
    """Add omitted fields to the generator's parsed model (name -> X3DNode) IN PLACE.

    The field is inserted in name order among the node's own fields, matching the
    UOM's alphabetical layout, so regenerating after upstream fixes the UOM is a no-op.
    Returns the additions applied; a node that already has the field is skipped."""
    from x3d_cpp_gen.emit.naming import sanitize_field_name
    from x3d_cpp_gen.parser import X3DField

    applied: List[Dict[str, Any]] = []
    for a in additions_for(version):
        for node_name in a["nodes"]:
            node = nodes.get(node_name)
            if node is None or any(f.x3d_name == a["field"] or f.name == a["field"]
                                   for f in node.fields):
                continue
            new = X3DField(
                name=sanitize_field_name(a["field"]),
                x3d_name=a["field"],
                type=a["type"],
                accessType=a["accessType"],
                default=a["default"],
                description=a["description"],
            )
            at = next((i for i, f in enumerate(node.fields)
                       if f.inherited_from is None and (f.x3d_name or f.name).lower() > a["field"].lower()),
                      len(node.fields))
            node.fields.insert(at, new)
            applied.append({**a, "node": node_name})
    return applied


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
