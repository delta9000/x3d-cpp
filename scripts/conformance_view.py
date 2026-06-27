#!/usr/bin/env python3
"""Conformance view generator — the single zoom-out for X3D SDK conformance.

Joins CODE FACTS (auto-derived, cannot rot) with JUDGMENTS (authored in
docs/conformance/findings.yaml) into one machine model (model.json) and the
human signaling layer (INDEX.md + components/*.md).

  facts    : node exists?  extracts?  has a System wired?  component/level/profile?
             -> derived from generated_cpp_bindings + X3DInterfaceRegistry + runtime/
  judgments: behaves per ISO prose?  severity?  status?  clause?
             -> docs/conformance/findings.yaml  (the one file you edit)

Design: docs/superpowers/specs/2026-06-19-conformance-view-design.md

CLI:
  conformance_view.py generate           regenerate model.json + Markdown (in place)
  conformance_view.py check              gate: validate + regen to temp + diff (CI)
"""
from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys
import tempfile
from dataclasses import dataclass
from typing import Optional

import yaml

# --- behavioral abstract interfaces (the "behaves" axis applies to these) ---
BEHAVIORAL_INTERFACES = {
    "X3DInterpolatorNode",
    "X3DTimeDependentNode",
    "X3DSensorNode",
    "X3DPointingDeviceSensorNode",
    "X3DDragSensorNode",
    "X3DKeyDeviceSensorNode",
    "X3DNetworkSensorNode",
    "X3DEnvironmentalSensorNode",
    "X3DScriptNode",
    "X3DBindableNode",
    "X3DFollowerNode",
    "X3DSequencerNode",
    "X3DTriggerNode",
}

# Abstract interfaces whose registered System attaches to EVERY implementer
# (so a concrete dynamic_cast is never emitted per node). Keep this tiny and
# justified — most systems cast to concrete types or instantiate System<Concrete>.
WIRED_BY_ABSTRACT = {
    "X3DBindableNode",  # BindingSystem services all bindables (Viewpoint/Background/Fog/NavInfo/...)
}

# Behavioral nodes that implement NO behavioral abstract interface (they are
# X3DChildNode) but DO have event-driven semantics — so the "behaves" axis applies.
# EaseInEaseOut: §19.4.4 fraction modifier. Boolean{Filter,Toggle}: §30 event utilities.
BEHAVIORAL_NODES = {"EaseInEaseOut", "BooleanFilter", "BooleanToggle"}

SEVERITIES = {"critical", "major", "minor", "low"}
STATUSES = {"open", "deferred", "fixed", "closed"}
OPEN_STATUSES = {"open", "deferred"}
DONE_STATUSES = {"fixed", "closed"}
# Optional per-finding override of the derived "behaves" axis. The derivation
# infers inertness only from "no System wired it"; a node can also be inert for a
# reason the wiring heuristic can't see (declared-but-uncalled, no shader path).
# An OPEN finding may assert `behaves: inert` to force ✗ — see classify_behaves.
BEHAVES_OVERRIDES = {"inert"}
SEVERITY_ORDER = {"critical": 0, "major": 1, "minor": 2, "low": 3}

GLYPH = {
    "yes": "✓",
    "partial": "◑",
    "no": "✗",
    "inert": "✗",
    "conformant": "✓",
    "n/a": "—",
}


@dataclass
class NodeFact:
    name: str
    component: Optional[str]
    level: Optional[int]
    abstract: bool


# ===========================================================================
# Code-fact extractors (pure functions, fixture-tested)
# ===========================================================================
def parse_node_header(name: str, text: str) -> NodeFact:
    comp = re.search(r'componentName\(\)\s*\{\s*return\s*"([^"]+)"', text)
    lvl = re.search(r"componentLevel\(\)\s*\{\s*return\s*(\d+)", text)
    return NodeFact(
        name=name,
        component=comp.group(1) if comp else None,
        level=int(lvl.group(1)) if lvl else None,
        abstract=comp is None,  # abstract base types carry no componentName()
    )


def parse_registry_table(text: str) -> dict[str, set[str]]:
    """Parse `{"Name", {InterfaceId::A, InterfaceId::B}}` rows -> {name: {ifaces}}."""
    table: dict[str, set[str]] = {}
    for m in re.finditer(r'\{"([A-Za-z0-9_]+)",\s*\{([^}]*)\}\}', text):
        ifaces = set(re.findall(r"InterfaceId::([A-Za-z0-9_]+)", m.group(2)))
        table[m.group(1)] = ifaces
    return table


def is_behavioral(ifaces) -> bool:
    return bool(set(ifaces) & BEHAVIORAL_INTERFACES)


def detect_system_targets(text: str, is_system_file: bool = False) -> set[str]:
    """Node types wired by a System.

    Always: concrete `dynamic_cast<Concrete *>` targets + `System<Concrete>`
    template instantiations. In *System files only*, also `== "NodeName"` string
    compares (e.g. ViewDependentSystem wires via nodeTypeName() equality) — scoped
    to System files so codec/reader string compares don't masquerade as wiring.
    """
    targets: set[str] = set()
    for m in re.finditer(r"dynamic_cast<\s*([A-Za-z0-9_]+)\s*\*>", text):
        targets.add(m.group(1))
    for m in re.finditer(r"[A-Za-z0-9_]*System<\s*([A-Za-z0-9_]+)", text):
        targets.add(m.group(1))
    if is_system_file:
        for m in re.finditer(r'==\s*"([A-Z][A-Za-z0-9]+)"', text):
            targets.add(m.group(1))
    targets.discard("NodeT")  # template parameter, not a concrete node
    return targets


def parse_recognized_geometry(text: str) -> set[str]:
    """The extractable geometry set from MeshBuilder::recognizedGeometryType()."""
    m = re.search(r"recognizedGeometryType\([^)]*\)\s*\{(.*?)\}", text, re.S)
    body = m.group(1) if m else ""
    return set(re.findall(r't ?== ?"([A-Za-z0-9]+)"', body))


# ===========================================================================
# Findings (judgments) — schema validation
# ===========================================================================
def validate_finding(f: dict) -> list[str]:
    errs = []
    for req in ("id", "summary"):
        if not f.get(req):
            errs.append(f"missing '{req}'")
    if f.get("severity") not in SEVERITIES:
        errs.append(f"bad severity {f.get('severity')!r} (want {sorted(SEVERITIES)})")
    if f.get("status") not in STATUSES:
        errs.append(f"bad status {f.get('status')!r} (want {sorted(STATUSES)})")
    if not f.get("nodes") and not f.get("interfaces"):
        errs.append("must tag at least one node or interface")
    bo = f.get("behaves")
    if bo is not None and bo not in BEHAVES_OVERRIDES:
        errs.append(f"bad behaves override {bo!r} (want {sorted(BEHAVES_OVERRIDES)})")
    return errs


def load_findings(path: pathlib.Path) -> list[dict]:
    if not path.exists():
        return []
    data = yaml.safe_load(path.read_text()) or []
    if not isinstance(data, list):
        raise ValueError(
            f"{path}: top-level YAML must be a list of findings, got {type(data).__name__}"
        )
    return data


# ===========================================================================
# Join / classification
# ===========================================================================
def classify_behaves(behavioral: bool, wired: bool, findings: list[dict]) -> str:
    if not behavioral:
        return "n/a"
    open_f = [f for f in findings if f.get("status") in OPEN_STATUSES]
    done_f = [f for f in findings if f.get("status") in DONE_STATUSES]
    # An OPEN finding may explicitly assert the node renders nothing. That is
    # authoritative: a closed finding on an UNRELATED concern (e.g. BIND-06 on
    # binding-stack detach) must not flip an inert node up to "partial".
    if any(f.get("behaves") == "inert" for f in open_f):
        return "inert"
    effective_wired = wired or bool(done_f)  # a closed finding asserts it's done
    if not effective_wired:
        return "inert"
    return "partial" if open_f else "conformant"


def node_profiles(component: Optional[str], level: Optional[int], profiles: dict) -> list[str]:
    if not component:
        return []
    lvl = level or 1
    # A profile dict may use "*" as a wildcard (Full includes every component).
    out = [p for p, comps in profiles.items()
           if comps.get(component, comps.get("*", 0)) >= lvl]
    return sorted(out, key=lambda p: list(profiles).index(p))


def findings_for(node: NodeFact, ifaces: set[str], findings: list[dict]) -> list[dict]:
    out = []
    for f in findings:
        fnodes = f.get("nodes") or []
        fifaces = set(f.get("interfaces") or [])
        if node.name in fnodes or (not fnodes and (fifaces & ifaces)):
            out.append(f)
    return out


def unresolved_findings(findings: list[dict], nodes: dict[str, NodeFact],
                        all_ifaces: Optional[set[str]] = None) -> list[dict]:
    all_ifaces = all_ifaces or set()
    out = []
    for f in findings:
        fnodes = set(f.get("nodes") or [])
        fifaces = set(f.get("interfaces") or [])
        if (fnodes & nodes.keys()) or (fifaces & all_ifaces):
            continue
        out.append(f)
    return out


# ===========================================================================
# Model builder
# ===========================================================================
def _read(p: pathlib.Path) -> str:
    return p.read_text(errors="ignore")


def build_model(repo: pathlib.Path) -> dict:
    bindings = repo / "generated_cpp_bindings"
    conf_dir = repo / "docs" / "conformance"

    # 1. nodes (exists) + concrete/abstract
    nodes: dict[str, NodeFact] = {}
    for hpp in sorted(bindings.glob("*.hpp")):
        name = hpp.stem
        nodes[name] = parse_node_header(name, _read(hpp))

    # 2. registry: node -> interfaces
    registry = parse_registry_table(_read(bindings / "X3DInterfaceRegistry.cpp"))
    all_ifaces = set().union(*registry.values()) if registry else set()

    # 3. System wiring (concrete + abstract-expansion)
    targets: set[str] = set()
    for hpp in (repo / "runtime").rglob("*.hpp"):
        path = hpp.as_posix()
        # The "behaves" axis is about behavioral event Systems (events/scene/script).
        # runtime/extract/ is the "extracts" axis — its *System files (MaterialSystem,
        # SceneExtractor) wire rendering, not behavior, so they don't count here.
        if "/tests/" in path or "/extract/" in path:
            continue
        is_sys = "system" in hpp.name.lower()
        targets |= detect_system_targets(_read(hpp), is_system_file=is_sys)
    # Restrict to real node types (drops non-node string-compare noise from the
    # System-file scan). NOTE: this is a heuristic — a behavioral node string-matched
    # in a System file for a non-wiring reason could read as wired; an open finding
    # corrects it to ◑, and wired_detected is emitted for audit.
    wired = {t for t in targets if t in registry}
    for abstract in WIRED_BY_ABSTRACT:
        wired |= {n for n, ifs in registry.items() if abstract in ifs}

    # 4. extraction
    mesh_builder = repo / "runtime" / "extract" / "MeshBuilder.hpp"
    handled_geom = parse_recognized_geometry(_read(mesh_builder)) if mesh_builder.exists() else set()

    # 5/6. findings + profiles
    findings = load_findings(conf_dir / "findings.yaml")
    finding_errors: dict[str, list[str]] = {}
    for i, f in enumerate(findings):
        errs = validate_finding(f)
        if errs:
            finding_errors[f.get("id") or f"#{i}"] = errs
    profiles_doc = yaml.safe_load(_read(conf_dir / "profiles.yaml")) if (conf_dir / "profiles.yaml").exists() else {}
    profiles = profiles_doc.get("profiles", {}) if isinstance(profiles_doc, dict) else {}

    # 7. per-node join (concrete nodes only)
    components: dict[str, dict] = {}
    for name, nf in nodes.items():
        # Concrete, instantiable nodes only: abstract X3D* base types carry no
        # componentName() OR appear as interface names in the registry.
        if nf.abstract or not nf.component or name in all_ifaces:
            continue
        ifaces = registry.get(name, set())
        behavioral = is_behavioral(ifaces) or name in BEHAVIORAL_NODES
        geometry = "X3DGeometryNode" in ifaces
        node_findings = findings_for(nf, ifaces, findings)
        extracts = ("n/a" if not geometry else ("yes" if name in handled_geom else "no"))
        behaves = classify_behaves(behavioral, name in wired, node_findings)
        comp = components.setdefault(nf.component, {
            "name": nf.component, "nodes": [], "levels": set(),
            "profiles": [], "findings": [],
        })
        comp["levels"].add(nf.level or 1)
        comp["nodes"].append({
            "name": name, "level": nf.level, "exists": True,
            "extracts": extracts, "behaves": behaves,
            "behavioral": behavioral, "geometry": geometry,
            "interfaces": sorted(ifaces),
            "profiles": node_profiles(nf.component, nf.level, profiles),
            "clause": _finding_clause(node_findings),
            "findings": sorted({f["id"] for f in node_findings if f.get("id")}),
        })

    # 8. attribute findings to components + finalize
    for f in findings:
        for comp_name in _finding_components(f, nodes, registry):
            if comp_name in components:
                components[comp_name]["findings"].append(f.get("id"))
    for comp in components.values():
        comp["levels"] = sorted(comp["levels"])
        comp["nodes"].sort(key=lambda n: n["name"])
        comp["findings"] = sorted(set(filter(None, comp["findings"])))
        comp["profiles"] = sorted(
            {p for n in comp["nodes"] for p in n["profiles"]},
            key=lambda p: list(profiles).index(p) if p in profiles else 99,
        )

    comp_list = sorted(components.values(), key=lambda c: c["name"])

    # 9. summary + integrity
    unresolved = unresolved_findings(findings, nodes, all_ifaces)
    open_by_sev: dict[str, int] = {}
    for f in findings:
        # severity-guarded so a schema-invalid finding can't crash the model before
        # cmd_check/cmd_generate report the schema error cleanly.
        if f.get("status") in OPEN_STATUSES and f.get("severity") in SEVERITIES:
            open_by_sev[f["severity"]] = open_by_sev.get(f["severity"], 0) + 1
    inert = [n["name"] for c in comp_list for n in c["nodes"] if n["behaves"] == "inert"]

    return {
        "meta": {
            "generator": "scripts/conformance_view.py",
            "node_count": sum(len(c["nodes"]) for c in comp_list),
            "wired_detected": sorted(t for t in wired if t in registry),
            "extractable_geometry": sorted(handled_geom),
            "finding_schema_errors": finding_errors,
        },
        "summary": {
            "component_count": len(comp_list),
            "finding_count": len(findings),
            "open_by_severity": open_by_sev,
            "closed_count": sum(1 for f in findings if f.get("status") in DONE_STATUSES),
            "deferred_count": sum(1 for f in findings if f.get("status") == "deferred"),
            "inert_behavioral_nodes": sorted(inert),
        },
        "components": comp_list,
        "findings": findings,
        "unresolved_findings": unresolved,
    }


def _finding_clause(node_findings: list[dict]) -> str:
    cls = [f.get("clause") for f in node_findings if f.get("clause")]
    return "; ".join(dict.fromkeys(cls))


def _finding_components(f: dict, nodes: dict[str, NodeFact], registry: dict) -> set[str]:
    if f.get("component"):
        return {f["component"]}
    comps = set()
    for n in f.get("nodes") or []:
        nf = nodes.get(n)
        if nf and nf.component:
            comps.add(nf.component)
    # Interface tag spreads to all implementers ONLY when no explicit nodes are
    # given (mirrors findings_for); an explicit node list scopes the finding.
    if not f.get("nodes"):
        for iface in f.get("interfaces") or []:
            for n, ifs in registry.items():
                if iface in ifs and (nf := nodes.get(n)) and nf.component:
                    comps.add(nf.component)
    return comps


# ===========================================================================
# Rendering (Markdown signaling layer)
# ===========================================================================
def _slug(comp: str) -> str:
    return comp


def render_index(model: dict) -> str:
    s = model["summary"]
    L = ["# X3D SDK — Conformance View", "",
         "_Generated by `scripts/conformance_view.py` — do not edit. "
         "Edit judgments in [`findings.yaml`](findings.yaml); facts are derived "
         "from the bindings/registry/runtime._", ""]

    # Bug picture
    L += ["## Bug picture", ""]
    sev = s["open_by_severity"]
    if sev:
        parts = [f"{sev[k]} {k}" for k in sorted(sev, key=lambda k: SEVERITY_ORDER.get(k, 9))]
        L.append(f"- **Open/deferred gaps:** {', '.join(parts)} "
                 f"({s['deferred_count']} deferred) · **closed:** {s['closed_count']}")
    else:
        L.append(f"- **Open gaps:** none · **closed:** {s['closed_count']}")
    if s["inert_behavioral_nodes"]:
        L.append(f"- **Inert behavioral nodes** (no System wired): "
                 f"{', '.join(s['inert_behavioral_nodes'])}")
    # worst offenders
    worst = sorted(
        [(c["name"], _open_findings(c, model)) for c in model["components"]],
        key=lambda x: (-len(x[1]),), )
    worst = [(n, fs) for n, fs in worst if fs][:6]
    if worst:
        L.append("- **Worst-offender components:** " + ", ".join(
            f"{n} ({len(fs)})" for n, fs in worst))
    if model["unresolved_findings"]:
        ids = ", ".join(f.get("id", "?") for f in model["unresolved_findings"])
        L.append(f"- ⚠️ **Stale findings** (reference unknown nodes/interfaces): {ids}")
    if model["meta"]["finding_schema_errors"]:
        L.append(f"- ⚠️ **Schema errors:** {model['meta']['finding_schema_errors']}")
    L.append("")

    # Matrix
    L += ["## Components", "",
          "| Component | Levels | Nodes | Extract | Behaves | Open gaps | Profiles |",
          "|-----------|--------|-------|---------|---------|-----------|----------|"]
    for c in model["components"]:
        geom = [n for n in c["nodes"] if n["geometry"]]
        beh = [n for n in c["nodes"] if n["behavioral"]]
        extract = (f"{sum(1 for n in geom if n['extracts'] == 'yes')}/{len(geom)}"
                   if geom else "—")
        behaves = _behaves_rollup(beh) if beh else "—"
        opens = _open_findings(c, model)
        gaps = _sev_summary(opens) if opens else "—"
        profs = ", ".join(_abbrev(p) for p in c["profiles"]) or "—"
        link = f"[{c['name']}](components/{_slug(c['name'])}.md)"
        L.append(f"| {link} | {','.join(map(str, c['levels']))} | {len(c['nodes'])} "
                 f"| {extract} | {behaves} | {gaps} | {profs} |")
    L += ["",
          "Legend: Extract = extractable/total geometry nodes · "
          "Behaves = ✓ conformant · ◑ partial · ✗ inert (of behavioral nodes) · "
          "— n/a.", ""]
    return "\n".join(L) + "\n"


def render_component(model: dict, comp: dict) -> str:
    L = [f"# {comp['name']} — conformance", "",
         f"_Generated. Levels {','.join(map(str, comp['levels']))} · "
         f"{len(comp['nodes'])} nodes · profiles: "
         f"{', '.join(comp['profiles']) or 'none'}._", "",
         "| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |",
         "|------|-----|--------|---------|---------|----------|------------|"]
    for n in comp["nodes"]:
        fids = ", ".join(n["findings"]) or "—"
        ifaces = ", ".join(i for i in n["interfaces"] if i != "X3DNode")
        L.append(f"| {n['name']} | {n['level'] or ''} | {GLYPH['yes']} "
                 f"| {GLYPH[n['extracts']]} | {GLYPH[n['behaves']]} | {fids} | {ifaces} |")
    L.append("")
    comp_findings = [f for f in model["findings"] if f.get("id") in set(comp["findings"])]
    if comp_findings:
        L += ["## Findings", ""]
        for f in sorted(comp_findings, key=lambda f: (
                f.get("status") not in OPEN_STATUSES,
                SEVERITY_ORDER.get(f.get("severity"), 9))):
            tag = f.get("status", "?").upper()
            commit = f" `{f['commit']}`" if f.get("commit") else ""
            clause = f" — §{f['clause']}" if f.get("clause") else ""
            L.append(f"- **{f['id']}** [{f.get('severity')}/{tag}{commit}]{clause}: "
                     f"{f.get('summary', '')}")
            if f.get("note"):
                L.append(f"  - {f['note']}")
        L.append("")
    return "\n".join(L) + "\n"


def _open_findings(comp: dict, model: dict) -> list[dict]:
    ids = set(comp["findings"])
    return [f for f in model["findings"]
            if f.get("id") in ids and f.get("status") in OPEN_STATUSES]


def _behaves_rollup(beh: list[dict]) -> str:
    c = sum(1 for n in beh if n["behaves"] == "conformant")
    p = sum(1 for n in beh if n["behaves"] == "partial")
    i = sum(1 for n in beh if n["behaves"] == "inert")
    bits = []
    if c:
        bits.append(f"{c}✓")
    if p:
        bits.append(f"{p}◑")
    if i:
        bits.append(f"{i}✗")
    return " ".join(bits) or "—"


def _sev_summary(findings: list[dict]) -> str:
    by = {}
    for f in findings:
        by[f["severity"]] = by.get(f["severity"], 0) + 1
    abbr = {"critical": "crit", "major": "maj", "minor": "min", "low": "low"}
    return ", ".join(f"{by[k]} {abbr[k]}"
                     for k in sorted(by, key=lambda k: SEVERITY_ORDER.get(k, 9)))


def _abbrev(p: str) -> str:
    return {"Interchange": "Inter", "Interactive": "Iact", "Immersive": "Imm",
            "Full": "Full", "Core": "Core", "CADInterchange": "CAD",
            "MedicalInterchange": "Med"}.get(p, p)


# ===========================================================================
# Output + CLI
# ===========================================================================
def write_outputs(repo: pathlib.Path, model: dict, out_dir: pathlib.Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "model.json").write_text(json.dumps(model, indent=2, sort_keys=True) + "\n")
    (out_dir / "INDEX.md").write_text(render_index(model))
    comp_dir = out_dir / "components"
    comp_dir.mkdir(exist_ok=True)
    # clear stale component files
    for old in comp_dir.glob("*.md"):
        old.unlink()
    for comp in model["components"]:
        (comp_dir / f"{_slug(comp['name'])}.md").write_text(render_component(model, comp))


def _gate_paths(out_dir: pathlib.Path) -> list[pathlib.Path]:
    paths = [out_dir / "model.json", out_dir / "INDEX.md"]
    paths += sorted((out_dir / "components").glob("*.md"))
    return paths


def cmd_generate(repo: pathlib.Path) -> int:
    model = build_model(repo)
    if model["meta"]["finding_schema_errors"]:
        print("findings.yaml schema errors:", file=sys.stderr)
        for fid, errs in model["meta"]["finding_schema_errors"].items():
            print(f"  {fid}: {errs}", file=sys.stderr)
        return 1
    write_outputs(repo, model, repo / "docs" / "conformance")
    s = model["summary"]
    print(f"conformance: {s['component_count']} components, {model['meta']['node_count']} nodes, "
          f"{s['finding_count']} findings ({sum(s['open_by_severity'].values())} open, "
          f"{s['closed_count']} closed).")
    if model["unresolved_findings"]:
        print("WARNING: stale findings:",
              [f.get("id") for f in model["unresolved_findings"]], file=sys.stderr)
    return 0


def cmd_check(repo: pathlib.Path) -> int:
    model = build_model(repo)
    errs = model["meta"]["finding_schema_errors"]
    if errs:
        print("CONFORMANCE GATE: findings.yaml schema errors:", file=sys.stderr)
        for fid, e in errs.items():
            print(f"  {fid}: {e}", file=sys.stderr)
        return 1
    if model["unresolved_findings"]:
        print("CONFORMANCE GATE: stale findings reference unknown nodes/interfaces:",
              file=sys.stderr)
        for f in model["unresolved_findings"]:
            print(f"  {f.get('id')}: nodes={f.get('nodes')} interfaces={f.get('interfaces')}",
                  file=sys.stderr)
        return 1
    committed = repo / "docs" / "conformance"
    with tempfile.TemporaryDirectory() as tmp:
        tmp_dir = pathlib.Path(tmp)
        write_outputs(repo, model, tmp_dir)
        drift = []
        for rel in _gate_paths(tmp_dir):
            relp = rel.relative_to(tmp_dir)
            cpath = committed / relp
            if not cpath.exists():
                drift.append(f"MISSING in committed: docs/conformance/{relp}")
            elif cpath.read_text() != rel.read_text():
                drift.append(f"DRIFT: docs/conformance/{relp}")
        for cpath in sorted((committed / "components").glob("*.md")):
            if not (tmp_dir / "components" / cpath.name).exists():
                drift.append(f"EXTRA (uncommitted-stale): docs/conformance/components/{cpath.name}")
    if drift:
        print("CONFORMANCE DRIFT DETECTED:", file=sys.stderr)
        print("\n".join("  " + d for d in drift), file=sys.stderr)
        print("\nRegenerate and commit:\n  mise run conformance", file=sys.stderr)
        return 1
    print("Conformance view OK: committed docs/conformance/ matches sources.")
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("command", choices=["generate", "check"])
    ap.add_argument("--repo", default=None, help="repo root (default: auto)")
    args = ap.parse_args(argv)
    repo = pathlib.Path(args.repo) if args.repo else pathlib.Path(__file__).resolve().parents[1]
    return cmd_generate(repo) if args.command == "generate" else cmd_check(repo)


if __name__ == "__main__":
    sys.exit(main())
