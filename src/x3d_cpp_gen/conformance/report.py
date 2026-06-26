"""The citable conformance-report artifact — the M3 moat capstone.

Runs the L2 structural validator over a corpus, binding each `.x3d` file to its
OWN declared version's (errata-corrected) manifest (Vc := Vd), and aggregates a
deterministic, machine-readable scorecard:

  - per-version: validated / clean / clean% / findings-by-code + the UOM
    provenance (uomManifestHash + nodeCount) that anchors the oracle to the spec;
  - overall findings-by-code (count + distinct files);
  - the residual surfaced for transparency (top unknown nodes / fields);
  - the cited errata corrections that fired, per version.

The point it proves: a validator whose criteria are EXTRACTED from the official
ISO UOM (not hand-authored) judges the official 16.7k-file archive ~99.7%
structurally clean against each file's own-version oracle, with every residual
finding inspectable. Deterministic: files iterated in sorted order, all maps
emitted sorted — same corpus → byte-identical report.

CLI:  python -m x3d_cpp_gen.conformance.report <corpus_root> [--json OUT] [--md OUT]
"""
from __future__ import annotations

import collections
import json
from pathlib import Path
from typing import Any, Dict, List

from lxml import etree

from x3d_cpp_gen.conformance.codes import ERROR
from x3d_cpp_gen.conformance.errata import apply_errata
from x3d_cpp_gen.conformance.validate import validate_document
from x3d_cpp_gen.conformance.version_resolve import resolve_version, load_manifest

SCHEMA_VERSION = 1
_MANIFEST_DIR = Path(__file__).parent / "manifests"


def _errata_applied_for(version: str) -> List[Dict[str, Any]]:
    """Re-derive which cited corrections fire on this version's committed manifest
    (independent of the load_manifest cache, on a throwaway copy)."""
    path = _MANIFEST_DIR / f"x3d-{version}.json"
    if not path.exists():
        return []
    nodes = json.loads(path.read_text())["nodes"]
    return [{k: v for k, v in e.items() if k != "nodes"} for e in apply_errata(version, nodes)]


def build_report(corpus_root: str) -> Dict[str, Any]:
    files = sorted(Path(corpus_root).rglob("*.x3d"))
    manifest_cache: Dict[str, Any] = {}

    totals = {"validated": 0, "skippedNoManifest": 0, "parseError": 0, "structurallyClean": 0}
    per_version: Dict[str, Dict[str, Any]] = {}
    code_total = collections.Counter()
    code_files = collections.Counter()
    unknown_nodes = collections.Counter()
    unknown_fields = collections.Counter()
    versions_seen = set()
    stamp_tally = collections.Counter()
    declared_n = inferred_n = 0

    def _vbucket(v: str) -> Dict[str, Any]:
        return per_version.setdefault(v, {
            "validated": 0, "clean": 0, "findingsByCode": collections.Counter()})

    for p in files:
        try:
            xml = p.read_bytes()
            res = resolve_version(xml)
        except etree.XMLSyntaxError:
            totals["parseError"] += 1
            continue
        vd = res.version
        if vd not in manifest_cache:
            try:
                manifest_cache[vd] = load_manifest(vd)
            except FileNotFoundError:
                manifest_cache[vd] = None
        m = manifest_cache[vd]
        if m is None:
            totals["skippedNoManifest"] += 1  # version with no committed manifest
            continue
        try:
            findings = validate_document(xml, m)
        except Exception:
            totals["parseError"] += 1
            continue

        stamp_tally[res.stamp] += 1
        if res.declared:
            declared_n += 1
        else:
            inferred_n += 1

        versions_seen.add(vd)
        totals["validated"] += 1
        vb = _vbucket(vd)
        vb["validated"] += 1
        errs = [f for f in findings if f.severity == ERROR]
        if not errs:
            totals["structurallyClean"] += 1
            vb["clean"] += 1
        for code in {f.code for f in errs}:
            code_files[code] += 1
        for f in errs:
            code_total[f.code] += 1
            vb["findingsByCode"][f.code] += 1
            if f.code == "NODE_UNKNOWN_FOR_VERSION":
                unknown_nodes[f.message.split(">", 1)[0].lstrip("<")] += 1
            elif f.code == "FIELD_UNKNOWN_FOR_NODE" and f.data:
                unknown_fields[f"{f.data.get('node')}.{f.data.get('field')}"] += 1

    def _pct(num: int, den: int) -> float:
        return round(100.0 * num / den, 2) if den else 0.0

    per_version_out: Dict[str, Any] = {}
    for v in sorted(per_version):
        vb = per_version[v]
        prov = {}
        m = manifest_cache.get(v)
        if m is not None:
            prov = {"uomManifestHash": m.uom_manifest_hash, "nodeCount": len(m.nodes)}
        per_version_out[v] = {
            "validated": vb["validated"],
            "clean": vb["clean"],
            "cleanPct": _pct(vb["clean"], vb["validated"]),
            "findingsByCode": dict(sorted(vb["findingsByCode"].items())),
            "provenance": prov,
        }

    return {
        "schemaVersion": SCHEMA_VERSION,
        "corpus": {"root": str(corpus_root), "x3dFilesFound": len(files)},
        "totals": {
            **totals,
            "cleanPct": _pct(totals["structurallyClean"], totals["validated"]),
        },
        "perVersion": per_version_out,
        "versionResolution": {
            "declared": declared_n,
            "inferred": inferred_n,
            "byStamp": dict(sorted(stamp_tally.items())),
            "bareFloorLowTrust": stamp_tally["VERSION_INFERRED_BARE_FLOOR"],
        },
        "findingsByCode": {
            c: {"count": code_total[c], "files": code_files[c]}
            for c in sorted(code_total)
        },
        "topUnknownNodes": [[k, v] for k, v in unknown_nodes.most_common(30)],
        "topUnknownFields": [[k, v] for k, v in unknown_fields.most_common(30)],
        "errataApplied": {
            v: _errata_applied_for(v) for v in sorted(versions_seen)
            if _errata_applied_for(v)
        },
    }


def format_markdown(report: Dict[str, Any]) -> str:
    t = report["totals"]
    lines = [
        "# X3D Structural Conformance Report",
        "",
        f"Corpus: `{report['corpus']['root']}` — {report['corpus']['x3dFilesFound']} `.x3d` files found.",
        "",
        f"**Validated {t['validated']} files against their own-version oracle "
        f"(Vc := Vd): {t['structurallyClean']} structurally clean "
        f"({t['cleanPct']}%).** "
        f"Skipped (no manifest / unversioned): {t['skippedNoManifest']}; "
        f"parse errors: {t['parseError']}.",
        "",
        "## Per-version scorecard",
        "",
        "| Version | Validated | Clean | Clean % | UOM manifest hash | Nodes |",
        "|--------:|----------:|------:|--------:|:------------------|------:|",
    ]
    for v in sorted(report["perVersion"]):
        pv = report["perVersion"][v]
        prov = pv.get("provenance", {})
        h = (prov.get("uomManifestHash") or "")[:12]
        lines.append(
            f"| {v} | {pv['validated']} | {pv['clean']} | {pv['cleanPct']}% | "
            f"`{h}` | {prov.get('nodeCount', '')} |"
        )
    lines += ["", "## Findings by code (residual)", "",
              "| Code | Total | Files |", "|:-----|------:|------:|"]
    for c, d in report["findingsByCode"].items():
        lines.append(f"| {c} | {d['count']} | {d['files']} |")

    if report["topUnknownNodes"]:
        lines += ["", "## Top unknown nodes (extension / version-skew / malformed)", ""]
        lines += [f"- `{n}` × {c}" for n, c in report["topUnknownNodes"][:15]]

    vr = report.get("versionResolution", {})
    if vr:
        lines += [
            "",
            "## Version resolution",
            "",
            f"- {vr['declared']} declared, {vr['inferred']} inferred "
            f"({vr['bareFloorLowTrust']} bare-floor low-trust)",
        ]

    errata = report.get("errataApplied", {})
    if errata:
        lines += ["", "## Cited UOM-errata corrections applied", ""]
        for v in sorted(errata):
            for e in errata[v]:
                lines.append(
                    f"- **{v}** `{e['node']}.{e['field']}.{e['attr']}` "
                    f"{e['from']} → {e['to']} (corrected upstream in {e['corrected_in']}) — {e['reason']}"
                )
    return "\n".join(lines) + "\n"


if __name__ == "__main__":
    import argparse

    ap = argparse.ArgumentParser(description="X3D structural conformance report")
    ap.add_argument("corpus_root")
    ap.add_argument("--json", dest="json_out")
    ap.add_argument("--md", dest="md_out")
    args = ap.parse_args()

    rep = build_report(args.corpus_root)
    if args.json_out:
        Path(args.json_out).write_text(
            json.dumps(rep, indent=2, sort_keys=True) + "\n")
    if args.md_out:
        Path(args.md_out).write_text(format_markdown(rep))
    if not (args.json_out or args.md_out):
        print(format_markdown(rep))
