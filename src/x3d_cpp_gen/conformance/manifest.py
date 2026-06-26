"""Per-version X3D conformance criteria, extracted from the ISO UOM as DATA.

A Manifest is the version-tagged, hashable structural oracle for one X3D spec
version. It reuses the existing UOM parser so the criteria DERIVE from the spec
artifact with no hand-authored tables (the moat). Validating a document against
its own version's manifest needs no recompile.
"""
from __future__ import annotations

import hashlib
import json
from dataclasses import dataclass
from typing import Any, Dict

from x3d_cpp_gen.generator import FIELD_TYPE_MAPPING, XS_TYPES
from x3d_cpp_gen.model.version import SpecVersion
from x3d_cpp_gen.parser import parse_x3d_model


@dataclass
class Manifest:
    uom_version: str
    nodes: Dict[str, Dict[str, Any]]  # nodeName -> {component, containerField, abstract, baseType, fields}
    uom_manifest_hash: str
    source_file: str

    def to_json(self) -> str:
        return _canonical_json(
            {
                "provenance": {
                    "uomVersion": self.uom_version,
                    "uomManifestHash": self.uom_manifest_hash,
                    "sourceFile": self.source_file,
                    "nodeCount": len(self.nodes),
                },
                "nodes": self.nodes,
            }
        )


def _canonical_json(obj: Any) -> str:
    # Stable, sorted, compact — so the same content always serializes identically.
    return json.dumps(obj, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def _node_to_dict(node) -> Dict[str, Any]:
    fields: Dict[str, Any] = {}
    for f in node.fields:
        wire = f.x3d_name or f.name
        fields[wire] = {
            "type": f.type,
            "accessType": f.accessType,
            "default": f.default,
            "minInclusive": f.min_inclusive,
            "maxInclusive": f.max_inclusive,
            "acceptableNodeTypes": f.acceptable_node_types,
            "simpleType": f.simple_type,
        }
    return {
        "component": (
            {"name": node.component.name, "level": node.component.level}
            if node.component is not None
            else None
        ),
        "containerField": node.container_field.default if node.container_field else None,
        "abstract": bool(node.is_abstract),
        "baseType": node.base_type,
        "fields": fields,
    }


def extract_manifest(uom_file: str) -> Manifest:
    version = SpecVersion.detect(uom_file).version
    parsed = parse_x3d_model(uom_file, FIELD_TYPE_MAPPING, XS_TYPES)
    nodes = {name: _node_to_dict(node) for name, node in parsed.items()}
    # Hash the canonical nodes payload only (provenance excluded so the hash is
    # purely content-addressed).
    content_hash = hashlib.sha256(_canonical_json(nodes).encode("ascii")).hexdigest()
    return Manifest(
        uom_version=version,
        nodes=nodes,
        uom_manifest_hash=content_hash,
        source_file=uom_file,
    )
