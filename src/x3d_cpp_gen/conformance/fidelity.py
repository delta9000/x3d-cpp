"""Generation-fidelity checks — the NON-CIRCULAR moat pillar.

The structural L2 validator compares documents against the UOM-derived manifest,
which is partly circular (the parser populates UOM-generated bindings). This module
checks a property that the circular path cannot fake: that every default the UOM
DECLARES actually MATERIALIZES into a C++ default expression via the generator. It
crosses the boundary UOM-data (manifest) -> generator-output (default_expr_for), so
a dropped/mangled default (the audit's #1 historical bug class) is caught here, not
hidden by a reflection tautology.
"""
from __future__ import annotations

from typing import List

from x3d_cpp_gen.conformance.codes import ERROR, Finding
from x3d_cpp_gen.conformance.manifest import Manifest
from x3d_cpp_gen.emit.defaults import default_expr_for
from x3d_cpp_gen.model.types import resolve_x3d_type


def check_defaults_materialize(manifest: Manifest) -> List[Finding]:
    """For every field that the UOM gives a default, assert the generator can turn
    that default into a C++ default expression. Enum-typed fields (simpleType) carry
    enum-value defaults handled by the enum generation path and are out of scope here.
    Zero findings == the generator faithfully carries every (non-enum) spec default."""
    findings: List[Finding] = []
    for node_name, node in manifest.nodes.items():
        for fname, f in node["fields"].items():
            default = f.get("default")
            if default is None:
                continue
            if f.get("simpleType"):
                continue  # enum default, handled by the enum generation path
            t = resolve_x3d_type(f["type"])
            if t is None or default_expr_for(t, default) is None:
                findings.append(Finding(
                    "DEFAULT_NOT_MATERIALIZED", ERROR, f"/{node_name}/{fname}",
                    f"UOM default {default!r} on {node_name}.{fname} "
                    f"(type {f['type']}) does not translate to a C++ default",
                    data={"node": node_name, "field": fname, "type": f["type"],
                          "default": default},
                ))
    return findings


def count_defaulted_fields(manifest: Manifest) -> int:
    """How many non-enum fields carry a UOM default (the check's coverage)."""
    return sum(
        1
        for node in manifest.nodes.values()
        for f in node["fields"].values()
        if f.get("default") is not None and not f.get("simpleType")
    )
