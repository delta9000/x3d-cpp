# UOM ↔ Spec-Prose Anchors Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a deterministic node→prose anchor map linking every UOM node to its governing ISO-19775-1 prose section, exposed as a `spec_rag.py` dev lookup and a conformance coverage cross-check.

**Architecture:** A pure-Python builder (`conformance/prose_anchors.py`) parses `<span id="…">` heading anchors from the prose markdown mirror and exact-matches them against the union of committed manifest node names, emitting a committed `prose_anchors.json`. `spec_rag.py` consumes that artifact for `node`/`field` lookups; field-level prose stays query-time (no nondeterministic data committed). Mirrors the existing `manifest.py` build / `validate.py` consume split.

**Tech Stack:** Python 3, stdlib only (`re`, `json`, `hashlib`, `glob`, `argparse`), pytest. No C++/codegen/golden impact.

**Spec:** `docs/superpowers/specs/2026-06-15-uom-prose-anchors-design.md`

**Key facts (grounded against the real mirror, 2026-06-15):**
- Prose mirror: `$X3D_SPEC_PROSE/*.md` (62 files), line 1 = `<!-- source: URL -->`.
- Node headings: `## <span id="Cone"></span> 13.3.2 Cone`; abstract types: `## <span id="X3DInterpolatorNode"></span> 19.3.1 *X3DInterpolatorNode*`.
- Manifests: `src/x3d_cpp_gen/conformance/manifests/x3d-{3.0,3.1,3.2,3.3,4.0,4.1}.json`, shape `{"nodes": {name: {abstract, baseType, component:{name,level}, containerField, fields:{…}}}, "provenance": {…}}`.
- Union node count = 364; exact-anchored = 336; unanchored = 28 (X3D statements + 4.1-new nodes absent from the 4.0 prose mirror).
- Run Python via the project venv so `import x3d_cpp_gen` resolves (e.g. `uv run python …` or the activated venv).

---

## File Structure

- **Create** `src/x3d_cpp_gen/conformance/prose_anchors.py` — builder, loader, coverage, CLI.
- **Create** `src/x3d_cpp_gen/conformance/prose_anchors.json` — committed derived artifact (generated in Task 4).
- **Modify** `scripts/spec_rag.py` — add `node` / `field` subcommands consuming the artifact.
- **Create** `tests/conformance/test_prose_anchors.py` — synthetic build tests + committed-artifact gates.

---

## Task 1: Module skeleton — constants, `union_node_set`, `prose_corpus_hash`

**Files:**
- Create: `src/x3d_cpp_gen/conformance/prose_anchors.py`
- Test: `tests/conformance/test_prose_anchors.py`

- [ ] **Step 1: Write the failing test**

```python
# tests/conformance/test_prose_anchors.py
import json
from pathlib import Path

from x3d_cpp_gen.conformance import prose_anchors as pa


def _write_manifest(dir_: Path, ver: str, nodes: dict):
    (dir_ / f"x3d-{ver}.json").write_text(json.dumps({"nodes": nodes, "provenance": {}}))


def test_union_node_set_merges_versions_and_abstract(tmp_path):
    _write_manifest(tmp_path, "4.0", {"Cone": {"abstract": False}})
    _write_manifest(tmp_path, "4.1", {"Cone": {"abstract": False},
                                      "Tangent": {"abstract": False},
                                      "X3DNode": {"abstract": True}})
    u = pa.union_node_set(manifest_dir=tmp_path)
    assert set(u) == {"Cone", "Tangent", "X3DNode"}
    assert u["Cone"]["versions"] == ["x3d-4.0", "x3d-4.1"]
    assert u["Tangent"]["versions"] == ["x3d-4.1"]
    assert u["X3DNode"]["abstract"] is True
    assert u["Cone"]["abstract"] is False


def test_prose_corpus_hash_is_stable_and_skips_index(tmp_path):
    (tmp_path / "a.md").write_text("# A\nbody\n")
    (tmp_path / "INDEX.md").write_text("ignored\n")
    h1 = pa.prose_corpus_hash(prose_dir=tmp_path)
    h2 = pa.prose_corpus_hash(prose_dir=tmp_path)
    assert h1 == h2 and len(h1) == 64
    (tmp_path / "a.md").write_text("# A\nCHANGED\n")
    assert pa.prose_corpus_hash(prose_dir=tmp_path) != h1
```

- [ ] **Step 2: Run test to verify it fails**

Run: `uv run pytest tests/conformance/test_prose_anchors.py -q`
Expected: FAIL — `ModuleNotFoundError` / `AttributeError: module 'x3d_cpp_gen.conformance.prose_anchors'`.

- [ ] **Step 3: Write minimal implementation**

```python
# src/x3d_cpp_gen/conformance/prose_anchors.py
"""Deterministic node->prose anchor map linking UOM nodes to ISO-19775-1 prose.

Build:    python -m x3d_cpp_gen.conformance.prose_anchors build
Coverage: python -m x3d_cpp_gen.conformance.prose_anchors coverage --version 4.1

The committed artifact (prose_anchors.json) is derived purely from the committed
manifests + the on-disk prose mirror by exact <span id="..."> heading matching.
Nothing embedding-derived enters the committed file (field best-effort is query-time).
"""
from __future__ import annotations

import argparse
import glob
import hashlib
import json
import os
import pathlib
import re
from typing import Optional

HERE = pathlib.Path(__file__).parent
MANIFEST_DIR = HERE / "manifests"
ANCHORS_PATH = HERE / "prose_anchors.json"
PROSE_DIR = pathlib.Path(
    os.environ.get("X3D_SPEC_PROSE_DIR",
                   "$X3D_SPEC_PROSE")
)
INDEX_PAGES = {"INDEX.md", "nodeIndex.md", "componentIndex.md"}


def union_node_set(manifest_dir=MANIFEST_DIR) -> dict:
    """node name -> {abstract: bool (in any version), versions: [stem,...]}."""
    out: dict = {}
    for f in sorted(pathlib.Path(manifest_dir).glob("x3d-*.json")):
        data = json.loads(f.read_text())
        for name, rec in data["nodes"].items():
            e = out.setdefault(name, {"abstract": False, "versions": []})
            e["versions"].append(f.stem)
            e["abstract"] = e["abstract"] or bool(rec.get("abstract"))
    for e in out.values():
        e["versions"] = sorted(e["versions"])
    return out


def prose_corpus_hash(prose_dir=PROSE_DIR) -> str:
    """Content-address the prose mirror so drift is detectable."""
    h = hashlib.sha256()
    for p in sorted(pathlib.Path(prose_dir).glob("*.md")):
        if p.name in INDEX_PAGES:
            continue
        h.update(p.name.encode())
        h.update(hashlib.sha256(p.read_bytes()).hexdigest().encode())
    return h.hexdigest()
```

- [ ] **Step 4: Run test to verify it passes**

Run: `uv run pytest tests/conformance/test_prose_anchors.py -q`
Expected: PASS (2 passed).

- [ ] **Step 5: Commit**

```bash
git add src/x3d_cpp_gen/conformance/prose_anchors.py tests/conformance/test_prose_anchors.py
git commit -m "prose-anchors: module skeleton (union_node_set, prose_corpus_hash)"
```

---

## Task 2: Heading parsing — `parse_prose_headings` and title/section helpers

**Files:**
- Modify: `src/x3d_cpp_gen/conformance/prose_anchors.py`
- Test: `tests/conformance/test_prose_anchors.py`

- [ ] **Step 1: Write the failing test**

```python
def test_parse_prose_headings_extracts_anchors(tmp_path):
    (tmp_path / "geometry3D.md").write_text(
        '<!-- source: https://example.org/geometry3D.html -->\n'
        '# 13 Geometry3D component\n'
        '## <span id="Box"></span> 13.3.1 Box\n'
        'box prose\n'
        '## <span id="Cone"></span> 13.3.2 Cone\n'
        'cone prose\n'
        '## <span id="X3DGeometryNode"></span> 13.3.0 *X3DGeometryNode*\n'
        '## <span id="NurbsSet"></span> **27.4.8 NurbsSet**\n'
    )
    cands = pa.parse_prose_headings(prose_dir=tmp_path)
    assert set(cands) == {"Box", "Cone", "X3DGeometryNode", "NurbsSet"}
    cone = cands["Cone"][0]
    assert cone["file"] == "geometry3D.md"
    assert cone["sourceUrl"] == "https://example.org/geometry3D.html"
    assert cone["section"] == "13.3.2"
    assert cone["depth"] == 2
    assert cone["level"] == 2
    assert cone["title"] == "Cone"
    # emphasis (*...*) stripped from abstract-type titles
    assert cands["X3DGeometryNode"][0]["title"] == "X3DGeometryNode"
    # markdown bold (**) wrapping the WHOLE "number title" must not defeat parsing
    nurbs = cands["NurbsSet"][0]
    assert nurbs["section"] == "27.4.8"
    assert nurbs["title"] == "NurbsSet"
```

- [ ] **Step 2: Run test to verify it fails**

Run: `uv run pytest tests/conformance/test_prose_anchors.py::test_parse_prose_headings_extracts_anchors -q`
Expected: FAIL — `AttributeError: module … has no attribute 'parse_prose_headings'`.

- [ ] **Step 3: Write minimal implementation**

Append to `prose_anchors.py`:

```python
_SPAN_ID = re.compile(r'<span id="([A-Za-z0-9_]+)">')
_HTML = re.compile(r'<[^>]+>')
_SECNUM = re.compile(r'^\s*(\d+(?:\.\d+)*)\s*')


def _source_url(path: pathlib.Path) -> str:
    first = path.read_text(errors="replace").split("\n", 1)[0]
    m = re.search(r'source:\s*(\S+)', first)
    return m.group(1) if m else ""


def _clean_title(raw: str) -> str:
    t = _HTML.sub("", raw)          # drop <span>/<img> tags
    t = t.replace("*", "")          # drop markdown emphasis (** bold / * italic)
    t = _SECNUM.sub("", t)          # drop leading "13.3.2 "
    return t.strip()


def _section_number(raw: str) -> str:
    t = _HTML.sub("", raw).replace("*", "")  # emphasis can wrap the number: **27.4.8 NurbsSet**
    m = _SECNUM.match(t)
    return m.group(1) if m else ""


def parse_prose_headings(prose_dir=PROSE_DIR) -> dict:
    """anchorId -> [ {file, sourceUrl, section, depth, level, title, order}, ... ]."""
    cands: dict = {}
    for p in sorted(pathlib.Path(prose_dir).glob("*.md")):
        if p.name in INDEX_PAGES:
            continue
        url = _source_url(p)
        for i, ln in enumerate(p.read_text(errors="replace").splitlines()):
            hm = re.match(r'^(#{1,4})\s+(.*)', ln)
            if not hm:
                continue
            sid = _SPAN_ID.search(ln)
            if not sid:
                continue
            sec = _section_number(hm.group(2))
            cands.setdefault(sid.group(1), []).append({
                "file": p.name, "sourceUrl": url, "section": sec,
                "depth": sec.count("."), "level": len(hm.group(1)),
                "title": _clean_title(hm.group(2)), "order": i,
            })
    return cands
```

- [ ] **Step 4: Run test to verify it passes**

Run: `uv run pytest tests/conformance/test_prose_anchors.py::test_parse_prose_headings_extracts_anchors -q`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add src/x3d_cpp_gen/conformance/prose_anchors.py tests/conformance/test_prose_anchors.py
git commit -m "prose-anchors: heading parser (anchor id + section + clean title)"
```

---

## Task 3: `build_anchor_map` — exact match, ambiguity, fuzzy, unanchored

**Files:**
- Modify: `src/x3d_cpp_gen/conformance/prose_anchors.py`
- Test: `tests/conformance/test_prose_anchors.py`

- [ ] **Step 1: Write the failing test**

```python
def _prose(tmp_path, name="c.md", body=""):
    (tmp_path / name).write_text("<!-- source: https://ex.org/c.html -->\n" + body)


def test_build_anchor_map_classifies(tmp_path):
    mdir = tmp_path / "manifests"; mdir.mkdir()
    pdir = tmp_path / "prose"; pdir.mkdir()
    _write_manifest(mdir, "4.1", {
        "Cone": {"abstract": False},        # clean exact match
        "Script": {"abstract": False},      # ambiguous (two files)
        "fontstyle": {"abstract": False},   # fuzzy: prose anchor is "FontStyle"
        "Tangent": {"abstract": False},     # unanchored (no prose)
    })
    _prose(pdir, "geometry3D.md",
           "## <span id=\"Cone\"></span> 13.3.2 Cone\nbody\n")
    _prose(pdir, "core.md",
           "## <span id=\"Script\"></span> 7.2.9 Script\nshallow\n")
    _prose(pdir, "scripting.md",
           "## <span id=\"Script\"></span> 29.4.1 Script\ndeep\n"
           "## <span id=\"FontStyle\"></span> 16.4.2 FontStyle\nfs\n")
    art = pa.build_anchor_map(prose_dir=pdir, manifest_dir=mdir)
    assert art["anchors"]["Cone"]["file"] == "geometry3D.md"
    assert art["anchors"]["Cone"]["fragment"] == "Cone"
    # ambiguity resolved to the deeper section, others recorded
    assert art["anchors"]["Script"]["section"] == "29.4.1"
    assert art["anchors"]["Script"]["ambiguous"] == [{"file": "core.md", "section": "7.2.9"}]
    # fuzzy (case-only) match is NOT a clean anchor
    assert "fontstyle" not in art["anchors"]
    assert art["fuzzy"]["fontstyle"]["matchedAnchor"] == "FontStyle"
    assert art["unanchored"] == ["Tangent"]
    assert art["provenance"]["nodeCount"] == 4
    assert art["provenance"]["anchoredCount"] == 2
```

- [ ] **Step 2: Run test to verify it fails**

Run: `uv run pytest tests/conformance/test_prose_anchors.py::test_build_anchor_map_classifies -q`
Expected: FAIL — no attribute `build_anchor_map`.

- [ ] **Step 3: Write minimal implementation**

Append to `prose_anchors.py`:

```python
def _neg_section(sec: str) -> tuple:
    """Section number as a negated int tuple so the LARGER (more specific, dedicated
    component) section sorts first. dot-count `depth` can't distinguish equal-depth
    sections like 7.2.9 (a shallow Core mention) from 29.4.1 (the node reference)."""
    return tuple(-int(x) for x in sec.split(".")) if sec else ()


def build_anchor_map(prose_dir=PROSE_DIR, manifest_dir=MANIFEST_DIR) -> dict:
    nodes = union_node_set(manifest_dir)
    cands = parse_prose_headings(prose_dir)
    lower: dict = {}
    for a in cands:
        lower.setdefault(a.lower(), []).append(a)

    anchors: dict = {}
    fuzzy: dict = {}
    unanchored: list = []
    for name in sorted(nodes):
        abstract = nodes[name]["abstract"]
        if name in cands:
            # tiebreak: exact-title first, then the largest (most-specific, dedicated
            # component) section number, shallowest heading, then file/order — all
            # deterministic over sorted inputs.
            cs = sorted(cands[name], key=lambda c: (
                c["title"] != name, _neg_section(c["section"]),
                c["level"], c["file"], c["order"]))
            best = cs[0]
            anchors[name] = {
                "file": best["file"], "sourceUrl": best["sourceUrl"],
                "fragment": name, "section": best["section"],
                "title": best["title"], "abstract": abstract,
                "ambiguous": [{"file": c["file"], "section": c["section"]} for c in cs[1:]],
            }
        elif name.lower() in lower:
            a = sorted(lower[name.lower()])[0]
            c = cands[a][0]
            fuzzy[name] = {"matchedAnchor": a, "file": c["file"], "section": c["section"]}
        else:
            unanchored.append(name)

    return {
        "provenance": {
            "proseCorpusHash": prose_corpus_hash(prose_dir),
            "builtFromManifests": sorted(
                p.stem for p in pathlib.Path(manifest_dir).glob("x3d-*.json")),
            "nodeCount": len(nodes),
            "anchoredCount": len(anchors),
            "fuzzyCount": len(fuzzy),
            "unanchoredCount": len(unanchored),
        },
        "anchors": anchors,
        "fuzzy": fuzzy,
        "unanchored": unanchored,
    }
```

- [ ] **Step 4: Run test to verify it passes**

Run: `uv run pytest tests/conformance/test_prose_anchors.py::test_build_anchor_map_classifies -q`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add src/x3d_cpp_gen/conformance/prose_anchors.py tests/conformance/test_prose_anchors.py
git commit -m "prose-anchors: build_anchor_map (exact/ambiguous/fuzzy/unanchored)"
```

---

## Task 4: `build` CLI + generate & commit the real `prose_anchors.json`

**Files:**
- Modify: `src/x3d_cpp_gen/conformance/prose_anchors.py`
- Create: `src/x3d_cpp_gen/conformance/prose_anchors.json`

- [ ] **Step 1: Add the serializer, loader, and CLI `build` to `prose_anchors.py`**

Append:

```python
def dumps_artifact(art: dict) -> str:
    """Stable, human-diffable serialization (sorted keys, 2-space indent, trailing nl)."""
    return json.dumps(art, sort_keys=True, indent=2, ensure_ascii=True) + "\n"


def load_anchors(path=ANCHORS_PATH) -> dict:
    return json.loads(pathlib.Path(path).read_text())


def _cmd_build(args) -> int:
    art = build_anchor_map()
    ANCHORS_PATH.write_text(dumps_artifact(art))
    p = art["provenance"]
    print(f"wrote {ANCHORS_PATH}  nodes={p['nodeCount']} "
          f"anchored={p['anchoredCount']} fuzzy={p['fuzzyCount']} "
          f"unanchored={p['unanchoredCount']}")
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="prose_anchors")
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("build", help="build & write prose_anchors.json")
    args = ap.parse_args(argv)
    if args.cmd == "build":
        return _cmd_build(args)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 2: Generate the real artifact**

Run: `uv run python -m x3d_cpp_gen.conformance.prose_anchors build`
Expected output (numbers should match the grounded build): `nodes=364 anchored=336 fuzzy=<n> unanchored=<28-n>`.

- [ ] **Step 3: Eyeball the artifact**

Run: `uv run python -c "import json;a=json.load(open('src/x3d_cpp_gen/conformance/prose_anchors.json'));print(a['provenance']);print('Cone',a['anchors']['Cone']);print('unanchored',a['unanchored'])"`
Expected: `Cone` anchors to `geometry3D.md §13.3.2`; `unanchored` is the statements + 4.1-new bucket (e.g. `ROUTE`, `IS`, `ProtoDeclare`, `Tangent`, `EnvironmentLight`, …).

- [ ] **Step 4: Commit**

```bash
git add src/x3d_cpp_gen/conformance/prose_anchors.py src/x3d_cpp_gen/conformance/prose_anchors.json
git commit -m "prose-anchors: build CLI + committed prose_anchors.json (336/364 anchored)"
```

---

## Task 5: `coverage` cross-check + CLI + calibration gate

**Files:**
- Modify: `src/x3d_cpp_gen/conformance/prose_anchors.py`
- Test: `tests/conformance/test_prose_anchors.py`

- [ ] **Step 1: Write the failing test**

```python
def test_coverage_buckets_for_version(tmp_path):
    mdir = tmp_path / "manifests"; mdir.mkdir()
    pdir = tmp_path / "prose"; pdir.mkdir()
    _write_manifest(mdir, "4.1", {
        "Cone": {"abstract": False, "component": {"name": "Geometry3D", "level": 1}},
        "Tangent": {"abstract": False, "component": {"name": "Rendering", "level": 1}},
    })
    _prose(pdir, "geometry3D.md", "## <span id=\"Cone\"></span> 13.3.2 Cone\nb\n"
                                  "## <span id=\"Newish\"></span> 13.3.9 Newish\nx\n")
    cov = pa.coverage("4.1", prose_dir=pdir, manifest_dir=mdir)
    assert cov["version"] == "4.1"
    assert cov["anchored"] == ["Cone"]
    assert cov["unanchored"] == [{"node": "Tangent", "abstract": False,
                                  "component": "Rendering"}]
    # prose anchor matching no manifest node (version-newer / mis-named)
    assert "Newish" in cov["proseOnly"]
```

- [ ] **Step 2: Run test to verify it fails**

Run: `uv run pytest tests/conformance/test_prose_anchors.py::test_coverage_buckets_for_version -q`
Expected: FAIL — no attribute `coverage`.

- [ ] **Step 3: Write minimal implementation**

Append to `prose_anchors.py` (and extend `main`):

```python
def coverage(version: str, prose_dir=PROSE_DIR, manifest_dir=MANIFEST_DIR) -> dict:
    man = json.loads((pathlib.Path(manifest_dir) / f"x3d-{version}.json").read_text())
    art = build_anchor_map(prose_dir, manifest_dir)
    anchored, unanchored, fuzzy = [], [], []
    for name, rec in man["nodes"].items():
        if name in art["anchors"]:
            anchored.append(name)
        elif name in art["fuzzy"]:
            fuzzy.append(name)
        else:
            unanchored.append({"node": name, "abstract": bool(rec.get("abstract")),
                               "component": (rec.get("component") or {}).get("name")})
    cands = parse_prose_headings(prose_dir)
    allnodes = set(union_node_set(manifest_dir))
    prose_only = sorted(a for a in cands if a not in allnodes and a[:1].isupper())
    return {"version": version,
            "anchored": sorted(anchored),
            "fuzzy": sorted(fuzzy),
            "unanchored": sorted(unanchored, key=lambda d: d["node"]),
            "proseOnly": prose_only}


def _cmd_coverage(args) -> int:
    cov = coverage(args.version)
    if args.json:
        print(json.dumps(cov, indent=2))
        return 0
    print(f"# Prose coverage — X3D {cov['version']}")
    print(f"  anchored:   {len(cov['anchored'])}")
    print(f"  fuzzy:      {len(cov['fuzzy'])}  {cov['fuzzy']}")
    print(f"  unanchored: {len(cov['unanchored'])}")
    for u in cov["unanchored"]:
        kind = "abstract" if u["abstract"] else "concrete"
        print(f"    - {u['node']}  ({kind}, {u['component']})")
    print(f"  prose-only (no manifest node): {cov['proseOnly']}")
    return 0
```

In `main`, replace the `build`-only parser block with:

```python
    sub.add_parser("build", help="build & write prose_anchors.json")
    cov = sub.add_parser("coverage", help="report node->prose coverage for a version")
    cov.add_argument("--version", default="4.1")
    cov.add_argument("--json", action="store_true")
    args = ap.parse_args(argv)
    if args.cmd == "build":
        return _cmd_build(args)
    if args.cmd == "coverage":
        return _cmd_coverage(args)
    return 1
```

- [ ] **Step 4: Add the calibration gate test (pins the real unanchored set)**

```python
# Statements (prose lives in core/concepts, not node-reference sections) + 4.1-new
# nodes absent from the 4.0 prose mirror. A node leaving/joining this set must be a
# CONSCIOUS recommit — this gate fails otherwise.
EXPECTED_UNANCHORED = {
    "EXPORT", "EnvironmentLight", "ExternProtoDeclare", "FontLibrary", "HAnimPose",
    "IMPORT", "IS", "InlineGeometry", "ProtoBody", "ProtoDeclare", "ProtoInstance",
    "ProtoInterface", "ROUTE", "RenderedTexture", "Scene", "SceneGraphStructureStatement",
    "Tangent", "X3D", "X3DStatement", "X3DTangentNode", "X3DUrlOutputObject",
    "component", "connect", "field", "fieldValue", "head", "meta", "unit",
}


def test_committed_unanchored_is_calibrated():
    art = pa.load_anchors()
    committed = set(art["unanchored"]) | set(art["fuzzy"])
    assert committed == EXPECTED_UNANCHORED, (
        "prose coverage changed; reconcile EXPECTED_UNANCHORED and recommit the artifact")
```

> Note: if the real build classifies some of these as `fuzzy` rather than `unanchored`
> (lowercase anchors like `field`/`head`/`meta`), the union on both sides keeps the gate
> correct. Run the build (Task 4) first, then reconcile `EXPECTED_UNANCHORED` to the actual
> `unanchored ∪ fuzzy` before committing.

- [ ] **Step 5: Run tests**

Run: `uv run pytest tests/conformance/test_prose_anchors.py -q`
Expected: PASS (all). If `test_committed_unanchored_is_calibrated` fails, edit `EXPECTED_UNANCHORED` to match the actual `unanchored ∪ fuzzy` printed by `coverage`, confirm each entry is a statement or a 4.1-new node, then re-run.

- [ ] **Step 6: Commit**

```bash
git add src/x3d_cpp_gen/conformance/prose_anchors.py tests/conformance/test_prose_anchors.py
git commit -m "prose-anchors: coverage cross-check + calibrated unanchored gate"
```

---

## Task 6: `section_text` live slice + committed-artifact schema test

**Files:**
- Modify: `src/x3d_cpp_gen/conformance/prose_anchors.py`
- Test: `tests/conformance/test_prose_anchors.py`

- [ ] **Step 1: Write the failing test**

```python
def test_section_text_slices_until_next_same_or_higher_heading(tmp_path):
    (tmp_path / "g.md").write_text(
        "## <span id=\"Cone\"></span> 13.3.2 Cone\n"
        "line one\n### <span id=\"Sub\"></span> 13.3.2.1 Sub\nsub line\n"
        "## <span id=\"Sphere\"></span> 13.3.3 Sphere\nnope\n"
    )
    anchor = {"file": "g.md", "fragment": "Cone"}
    txt = pa.section_text(anchor, prose_dir=tmp_path)
    assert "Cone" in txt and "line one" in txt and "sub line" in txt
    assert "Sphere" not in txt and "nope" not in txt


def test_section_text_returns_none_when_mirror_absent(tmp_path):
    assert pa.section_text({"file": "missing.md", "fragment": "Cone"},
                           prose_dir=tmp_path) is None


def test_committed_artifact_schema_is_valid():
    art = pa.load_anchors()
    assert set(art) >= {"provenance", "anchors", "fuzzy", "unanchored"}
    for name, a in art["anchors"].items():
        assert a["fragment"] == name
        assert a["file"].endswith(".md")
        assert a["sourceUrl"].startswith("http")
        assert a["section"] and a["section"][0].isdigit()  # no empty/corrupt section
```

- [ ] **Step 2: Run test to verify it fails**

Run: `uv run pytest tests/conformance/test_prose_anchors.py -k "section_text or schema" -q`
Expected: FAIL — no attribute `section_text`.

- [ ] **Step 3: Write minimal implementation**

Append to `prose_anchors.py`:

```python
def section_text(anchor: dict, prose_dir=PROSE_DIR) -> Optional[str]:
    """Live slice of the prose section for an anchor, heading to next same/higher heading.

    Returns None if the prose mirror file is absent (citation-only degrade).
    """
    p = pathlib.Path(prose_dir) / anchor["file"]
    if not p.exists():
        return None
    lines = p.read_text(errors="replace").splitlines()
    needle = f'id="{anchor["fragment"]}"'
    start = next((i for i, ln in enumerate(lines)
                  if needle in ln and ln.lstrip().startswith("#")), None)
    if start is None:
        return None
    lvl = len(re.match(r'^(#+)', lines[start].lstrip()).group(1))
    out = [lines[start]]
    for ln in lines[start + 1:]:
        m = re.match(r'^(#{1,4})\s', ln)
        if m and len(m.group(1)) <= lvl:
            break
        out.append(ln)
    return "\n".join(out).strip()
```

- [ ] **Step 4: Run tests**

Run: `uv run pytest tests/conformance/test_prose_anchors.py -k "section_text or schema" -q`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add src/x3d_cpp_gen/conformance/prose_anchors.py tests/conformance/test_prose_anchors.py
git commit -m "prose-anchors: section_text live slice + committed-artifact schema test"
```

---

## Task 7: `spec_rag.py node <Name>` lookup

**Files:**
- Modify: `scripts/spec_rag.py`

- [ ] **Step 1: Add imports + `cmd_node` to `scripts/spec_rag.py`**

Add near the top imports (after the existing `import json, re, subprocess, sys, pathlib`):

```python
from x3d_cpp_gen.conformance.prose_anchors import (
    load_anchors, section_text, MANIFEST_DIR,
)


def _load_manifest(version):
    p = MANIFEST_DIR / f"x3d-{version}.json"
    if not p.exists():
        print(f"unknown X3D version {version!r} (have 3.0-4.1)"); sys.exit(1)
    return json.loads(p.read_text())


def cmd_node(name, version="4.1", rag_k=0):
    man = _load_manifest(version)
    rec = man["nodes"].get(name)
    if not rec:
        print(f"unknown node {name!r} in X3D {version}")
        return
    comp = rec.get("component") or {}
    print(f"# {name}  [X3D {version}]"
          f"{'  (abstract)' if rec.get('abstract') else ''}")
    print(f"  component: {comp.get('name')} (L{comp.get('level')})  "
          f"containerField: {rec.get('containerField')}  base: {rec.get('baseType')}  "
          f"fields: {len(rec.get('fields', {}))}")
    art = load_anchors()
    a = art["anchors"].get(name)
    if not a:
        fz = art["fuzzy"].get(name)
        extra = f" (fuzzy: {fz['matchedAnchor']})" if fz else ""
        print(f"  prose: [no node-reference anchor]{extra}")
    else:
        print(f"  prose: {a['file']} §{a['section']} {a['title']}  "
              f"-> {a['sourceUrl']}#{a['fragment']}")
        txt = section_text(a)
        print("\n" + (txt if txt else "  [prose mirror not present]"))
    if rag_k:
        print("\n--- related prose ---")
        query(name, rag_k)
```

- [ ] **Step 2: Wire the subcommand into `__main__`**

Replace the existing `if __name__ == "__main__":` block in `spec_rag.py` with:

```python
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__); sys.exit(1)
    cmd = sys.argv[1]
    rest = sys.argv[2:]
    version = "4.1"
    if "--version" in rest:
        i = rest.index("--version"); version = rest[i + 1]; del rest[i:i + 2]
    if cmd == "ingest":
        ingest()
    elif cmd == "node":
        rag_k = 0
        if "--rag" in rest:
            i = rest.index("--rag"); rag_k = int(rest[i + 1]); del rest[i:i + 2]
        cmd_node(rest[0], version, rag_k)
    else:
        k = 6
        args = sys.argv[1:]
        if "-k" in args:
            i = args.index("-k"); k = int(args[i + 1]); del args[i:i + 2]
        query(" ".join(args), k)
```

- [ ] **Step 3: Manually verify the lookup**

Run: `uv run python scripts/spec_rag.py node Cone`
Expected: prints the Cone manifest summary, the `geometry3D.md §13.3.2 Cone` citation with the `…#Cone` URL, and the Cone prose section text (assuming the mirror is present).

Run: `uv run python scripts/spec_rag.py node Tangent`
Expected: manifest summary + `prose: [no node-reference anchor]` (4.1-new node, absent from the 4.0 mirror).

- [ ] **Step 4: Commit**

```bash
git add scripts/spec_rag.py
git commit -m "spec_rag: 'node <Name>' lookup (manifest summary + linked prose + --rag)"
```

---

## Task 8: `spec_rag.py field <Node>.<field>` best-effort lookup

**Files:**
- Modify: `scripts/spec_rag.py`

- [ ] **Step 1: Add `cmd_field` to `scripts/spec_rag.py`**

```python
def cmd_field(spec, version="4.1"):
    node, _, field = spec.partition(".")
    if not field:
        print("usage: spec_rag.py field <Node>.<field>")
        return
    man = _load_manifest(version)
    rec = man["nodes"].get(node)
    if not rec or field not in rec.get("fields", {}):
        print(f"unknown field {spec!r} in X3D {version}")
        return
    f = rec["fields"][field]
    print(f"# {node}.{field}  [X3D {version}]")
    print(f"  type: {f['type']}  access: {f['accessType']}  default: {f['default']}  "
          f"range: [{f['minInclusive']}, {f['maxInclusive']}]  "
          f"accepts: {f['acceptableNodeTypes']}")
    art = load_anchors()
    a = art["anchors"].get(node)
    print("\n  prose (best-effort):")
    txt = section_text(a) if a else None
    hit = None
    if txt:
        for ln in txt.splitlines():
            if re.search(rf'\b{re.escape(field)}\b', ln):
                hit = ln.strip()
                break
    if hit:
        print("   " + hit)
    else:
        query(f"{node} {field}", 3)
```

- [ ] **Step 2: Wire `field` into `__main__`**

In the `__main__` dispatch added in Task 7, add an `elif` before the final `else`:

```python
    elif cmd == "field":
        cmd_field(rest[0], version)
```

- [ ] **Step 3: Manually verify**

Run: `uv run python scripts/spec_rag.py field Cone.bottomRadius`
Expected: the `bottomRadius` manifest record (`type SFFloat`, `accessType initializeOnly`, default + range), then a best-effort prose line mentioning `bottomRadius`, or a 3-hit RAG fallback if no exact sentence is found.

Run: `uv run python scripts/spec_rag.py field Material.transparency`
Expected: field record + best-effort prose (RAG fallback acceptable).

- [ ] **Step 4: Commit**

```bash
git add scripts/spec_rag.py
git commit -m "spec_rag: 'field <Node>.<field>' best-effort prose lookup"
```

---

## Task 9: Rebuild-gate test + full verification

**Files:**
- Modify: `tests/conformance/test_prose_anchors.py`

- [ ] **Step 1: Add the rebuild-gate test (skips without the mirror)**

```python
import pytest


@pytest.mark.skipif(not pa.PROSE_DIR.exists(),
                    reason="prose mirror not present (external x3d-render)")
def test_committed_anchor_map_matches_fresh_build():
    fresh = pa.dumps_artifact(pa.build_anchor_map())
    assert pa.ANCHORS_PATH.read_text() == fresh, (
        "prose_anchors.json drifted from a fresh build; "
        "rerun `python -m x3d_cpp_gen.conformance.prose_anchors build` and recommit")
```

- [ ] **Step 2: Run the full prose-anchors suite**

Run: `uv run pytest tests/conformance/test_prose_anchors.py -q`
Expected: PASS (the rebuild-gate runs locally where the mirror exists; skips in CI without it).

- [ ] **Step 3: Run the whole Python suite (no regressions)**

Run: `uv run pytest -q`
Expected: PASS; total count rises by the new tests (no other suite affected).

- [ ] **Step 4: Confirm zero golden / codegen impact**

Run: `bash scripts/check_golden.sh`
Expected: golden tree byte-identical (this work touched no codegen, headers, or template).

- [ ] **Step 5: Commit**

```bash
git add tests/conformance/test_prose_anchors.py
git commit -m "prose-anchors: rebuild-gate (committed == fresh build, skips w/o mirror)"
```

---

## Self-Review

**Spec coverage:**
- Anchor-build algorithm (spec Component 1) → Tasks 1–3. ✓
- `prose_anchors.json` schema (Component 2) → Task 4 artifact + Task 6 schema test. ✓
- Dev lookup `node`/`field` (Component 3) → Tasks 7–8. ✓
- Coverage cross-check (Component 4) → Task 5. ✓
- Testing & impact (synthetic build, schema, coverage floor, rebuild-gate skip, golden-untouched) → Tasks 1/3/5/6/9. ✓
- Non-goals (no vendoring, no MCP, no baked field data, golden-untouched) → respected; rebuild-gate skips without the external mirror (Task 9). ✓

**Placeholder scan:** No TBD/TODO; every code step shows complete code; the one calibration value (`EXPECTED_UNANCHORED`) is pre-populated from the grounded build with an explicit reconcile instruction. ✓

**Type consistency:** `build_anchor_map`, `union_node_set`, `parse_prose_headings`, `prose_corpus_hash`, `coverage`, `dumps_artifact`, `load_anchors`, `section_text`, `MANIFEST_DIR`, `ANCHORS_PATH`, `PROSE_DIR` are referenced with consistent names/signatures across tasks; anchor record keys (`file`/`sourceUrl`/`fragment`/`section`/`title`/`abstract`/`ambiguous`) match between builder (Task 3), schema test (Task 6), and `spec_rag` consumers (Tasks 7–8). ✓
