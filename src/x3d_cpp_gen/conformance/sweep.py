"""Run the L2 structural validator over a directory of .x3d files,
binding each to its declared OR inferred version's manifest (VP-2 ladder; floor 3.0 always binds)."""
from __future__ import annotations

import sys
from pathlib import Path
from typing import Dict

from lxml import etree

from x3d_cpp_gen.conformance.validate import validate_document
from x3d_cpp_gen.conformance.version_resolve import resolve_version, load_manifest


def sweep(root: str) -> Dict[str, int]:
    counts = {"validated": 0, "inferred": 0, "skipped_no_manifest": 0,
              "parse_error": 0, "errors": 0}
    cache: Dict[str, object] = {}
    for p in Path(root).rglob("*.x3d"):
        try:
            xml = p.read_bytes()
            res = resolve_version(xml)
        except etree.XMLSyntaxError:
            counts["parse_error"] += 1
            continue
        if res.version not in cache:
            try:
                cache[res.version] = load_manifest(res.version)
            except FileNotFoundError:
                cache[res.version] = None
        m = cache[res.version]
        if m is None:
            counts["skipped_no_manifest"] += 1  # version with no committed manifest
            continue
        try:
            findings = validate_document(xml, m)
        except Exception:
            counts["parse_error"] += 1
            continue
        counts["validated"] += 1
        if not res.declared:
            counts["inferred"] += 1
        counts["errors"] += sum(1 for f in findings if f.severity == 0)
    return counts


if __name__ == "__main__":
    print(sweep(sys.argv[1] if len(sys.argv) > 1 else "."))
