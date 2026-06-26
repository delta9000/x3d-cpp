"""Compute the structural ripple between two version manifests. The moat's
future-proofing demo asserts: observed L2 criteria delta == this computed set."""
from __future__ import annotations

from typing import Any, Dict, List

from x3d_cpp_gen.conformance.manifest import Manifest


def manifest_diff(a: Manifest, b: Manifest) -> Dict[str, Any]:
    a_nodes, b_nodes = set(a.nodes), set(b.nodes)
    added = sorted(b_nodes - a_nodes)
    removed = sorted(a_nodes - b_nodes)

    fields_added: List[str] = []
    defaults_changed: List[Dict[str, Any]] = []
    for name in sorted(a_nodes & b_nodes):
        af = a.nodes[name]["fields"]
        bf = b.nodes[name]["fields"]
        for fname in sorted(set(bf) - set(af)):
            fields_added.append(f"{name}.{fname}")
        for fname in sorted(set(af) & set(bf)):
            ad, bd = af[fname].get("default"), bf[fname].get("default")
            if ad != bd:
                defaults_changed.append(
                    {"node": name, "field": fname, "from": ad, "to": bd}
                )
    return {
        "from_version": a.uom_version,
        "to_version": b.uom_version,
        "nodes_added": added,
        "nodes_removed": removed,
        "fields_added": fields_added,
        "defaults_changed": defaults_changed,
    }
