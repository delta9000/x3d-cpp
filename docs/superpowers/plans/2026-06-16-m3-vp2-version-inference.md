# M3 VP-2 — Version-Inference Ladder + VRML97→3.0 Floor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Give the 133 unversioned `.x3d` corpus files a real per-version oracle (instead of `skipped_no_manifest`) via a deterministic, evidence-stamped inference ladder, and stop the runtime ever emitting the invalid `#X3D V2.0` header by flooring VRML97 (and any sub-3.0 version) to X3D 3.0 at read and write time.

**Architecture:** Two independent surfaces, both **codegen-free** (golden hash must stay byte-identical):
1. **Python conformance** (`src/x3d_cpp_gen/conformance/`) — a new `node_floor.py` deriving each node's earliest version from the six committed manifests, and a `resolve_version()` ladder in `version_resolve.py` (declared → schema/DTD citation → node-floor → profile-floor → bare 3.0), surfaced through `sweep.py` and `report.py` with tiered confidence stamps.
2. **C++ runtime** (`runtime/parse/`, `runtime/codecs/`) — VRML97→X3D 3.0 floor + a sub-3.0 clamp in `ClassicVrmlReader::parseHeaderLine` / `Vrml97Reader::onHeaderLine` (read-time remap, the §8 root fix), plus a defensive clamp in `VrmlWriter` so a programmatically-built `doc.version="2.0"` still serializes a valid header.

**Tech Stack:** Python 3 + lxml + pytest (conformance/`); header-only C++20 + the project's `check()`-based test harness wired through the root `CMakeLists.txt`; `mise run golden` / `mise run test` / `cmake --build build -j4` + ctest for verification.

**Ratified design source:** `docs/superpowers/specs/2026-06-13-m3-versioning-design.md` §1 (inference ladder, tiered stamps), §8 (VRML97→3.0 floor + read-time remap), §9c open-decisions 2 & 3 (RESOLVED). Floor = **3.0**, NOT 4.0. Valid X3D versions = {3.0, 3.1, 3.2, 3.3, 4.0, 4.1}.

**Spec gap flagged (resolve during Task 2):** §1 prose says the stamp "is one of three" (`VERSION_INFERRED_XSD_CITED`, `_NODE_FORCED`, `_BARE_FLOOR`) but the ladder has a 4th rung (profile-floor, step 3) with no enumerated stamp. This plan adds `VERSION_INFERRED_PROFILE_FLOOR` (INFO, medium confidence) to keep stamps 1:1 with rungs. This is a faithful completion, not a design change — note it in the Task 2 commit message.

**Forward-compat guard (do not clamp the future down):** the sub-3.0 clamp fires ONLY for versions whose major < 3 (i.e. "1.0"/"2.0"). A version ≥ 3.0 — including a future "4.2" we have no manifest for — is left untouched (§6d graceful forward-compat). Never clamp a high version to the floor.

---

## File Structure

| File | Responsibility | Action |
|---|---|---|
| `src/x3d_cpp_gen/conformance/node_floor.py` | Derive `{nodeName: earliestVersion}` from the 6 committed manifests, ascending. | Create |
| `src/x3d_cpp_gen/conformance/version_resolve.py` | Add `VersionResolution` dataclass + `resolve_version()` ladder; keep `detect_document_version`/`load_manifest`. | Modify |
| `src/x3d_cpp_gen/conformance/codes.py` | Append 4 `VERSION_INFERRED_*` codes. | Modify |
| `src/x3d_cpp_gen/conformance/sweep.py` | Use `resolve_version`; count by stamp tier; floor always binds a manifest. | Modify |
| `src/x3d_cpp_gen/conformance/report.py` | Bind via `resolve_version`; aggregate per-stamp; flag BARE_FLOOR low-trust. | Modify |
| `tests/conformance/test_version_inference.py` | Unit-test every ladder rung + node_floor map. | Create |
| `runtime/parse/ClassicVrmlReader.hpp` | `parseHeaderLine`: `#VRML*` → 3.0; sub-3.0 version → 3.0. | Modify (`:161-170`) |
| `runtime/parse/Vrml97Reader.hpp` | `onHeaderLine`: set `doc.version="3.0"` in every VRML97 branch (incl. missing/garbled header). | Modify (`:102-145`) |
| `runtime/codecs/VrmlWriter.hpp` | Defensive: emit a floored version in the header line. | Modify (`:45`) |
| `runtime/parse/tests/version_floor_test.cpp` | New C++ test for read-time floor + writer clamp. | Create |
| `CMakeLists.txt` | Register `x3d_version_floor` test executable. | Modify |

---

## Task 1: Node-floor map (Python)

Derive each node's earliest defining version from the six committed manifests. The ladder's step 2 (node-floor) consumes this. Manifests are loaded in ascending version order; `setdefault` keeps the first (earliest) version that defines a node.

**Files:**
- Create: `src/x3d_cpp_gen/conformance/node_floor.py`
- Test: `tests/conformance/test_version_inference.py`

- [ ] **Step 1: Write the failing test**

Create `tests/conformance/test_version_inference.py`:

```python
from x3d_cpp_gen.conformance.node_floor import node_floor_map, X3D_VERSIONS, version_key


def test_x3d_versions_ascending():
    assert X3D_VERSIONS == ["3.0", "3.1", "3.2", "3.3", "4.0", "4.1"]
    assert version_key("3.10") > version_key("3.3")  # numeric, not lexicographic


def test_node_floor_box_is_3_0():
    # Box is a Core/Geometry3D primitive present since X3D 3.0.
    assert node_floor_map()["Box"] == "3.0"


def test_node_floor_is_monotone_and_covers_all_versions():
    fm = node_floor_map()
    assert fm  # non-empty
    assert set(fm.values()) <= set(X3D_VERSIONS)
    # A 4.x-era node must floor above 3.0 (sanity that not everything collapses to 3.0).
    assert any(v != "3.0" for v in fm.values())
```

- [ ] **Step 2: Run test to verify it fails**

Run: `uv run pytest tests/conformance/test_version_inference.py -v`
Expected: FAIL — `ModuleNotFoundError: No module named 'x3d_cpp_gen.conformance.node_floor'`

- [ ] **Step 3: Write minimal implementation**

Create `src/x3d_cpp_gen/conformance/node_floor.py`:

```python
"""Earliest-defining-version per node, derived from the committed per-version
manifests (data only — no UOM re-parse). Consumed by the VP-2 inference ladder's
node-floor rung (§1 step 2)."""
from __future__ import annotations

import functools
from typing import Dict, Tuple

from x3d_cpp_gen.conformance.version_resolve import load_manifest

# Ascending; the only X3D versions with a committed manifest. Floor = first entry.
X3D_VERSIONS = ["3.0", "3.1", "3.2", "3.3", "4.0", "4.1"]
FLOOR_VERSION = X3D_VERSIONS[0]


def version_key(v: str) -> Tuple[int, ...]:
    """Numeric sort key so '3.10' > '3.3' (lexicographic would invert them)."""
    return tuple(int(p) for p in v.split("."))


@functools.lru_cache(maxsize=1)
def node_floor_map() -> Dict[str, str]:
    """{nodeName: earliest X3D version (of X3D_VERSIONS) whose manifest defines it}."""
    floor: Dict[str, str] = {}
    for v in X3D_VERSIONS:  # ascending → setdefault keeps the earliest
        for name in load_manifest(v).nodes:
            floor.setdefault(name, v)
    return floor
```

- [ ] **Step 4: Run test to verify it passes**

Run: `uv run pytest tests/conformance/test_version_inference.py -v`
Expected: PASS (3 tests)

- [ ] **Step 5: Commit**

```bash
git add src/x3d_cpp_gen/conformance/node_floor.py tests/conformance/test_version_inference.py
git commit -m "conformance: VP-2 node-floor map (earliest version per node, from manifests)"
```

---

## Task 2: Inference codes + `resolve_version` ladder (Python)

The deterministic, evidence-stamped ladder replacing today's silent skip. Order (strongest first, per §1): declared `version` attr → schema/DTD citation → node-floor → profile-floor → bare 3.0.

**Files:**
- Modify: `src/x3d_cpp_gen/conformance/codes.py:11-18` (append codes)
- Modify: `src/x3d_cpp_gen/conformance/version_resolve.py` (add dataclass + ladder)
- Test: `tests/conformance/test_version_inference.py`

- [ ] **Step 1: Write the failing test**

Append to `tests/conformance/test_version_inference.py`:

```python
from x3d_cpp_gen.conformance.version_resolve import resolve_version


def _r(xml):
    return resolve_version(xml.encode("utf-8"))


def test_declared_version_wins():
    res = _r('<X3D version="4.0" profile="Immersive"><Scene/></X3D>')
    assert (res.version, res.stamp, res.declared) == ("4.0", "VERSION_DECLARED", True)


def test_xsd_citation_inferred():
    xml = ('<X3D profile="Immersive" '
           'xmlns:xsd="http://www.w3.org/2001/XMLSchema-instance" '
           'xsd:noNamespaceSchemaLocation='
           '"http://www.web3d.org/specifications/x3d-3.3.xsd"><Scene/></X3D>')
    res = _r(xml)
    assert (res.version, res.stamp, res.declared) == ("3.3", "VERSION_INFERRED_XSD_CITED", False)


def test_node_floor_inferred():
    # No version, no citation, no profile — but a node forces a floor.
    # MetadataDouble is a 3.1-added node (absent in 3.0).
    res = _r('<X3D><Scene><MetadataDouble/></Scene></X3D>')
    assert res.stamp == "VERSION_INFERRED_NODE_FORCED"
    assert res.version == "3.1"
    assert res.declared is False


def test_profile_floor_inferred():
    # No version, no citation, no recognized node — only a profile.
    res = _r('<X3D profile="Immersive"><Scene/></X3D>')
    assert (res.version, res.stamp) == ("3.0", "VERSION_INFERRED_PROFILE_FLOOR")


def test_bare_floor_inferred():
    res = _r('<X3D><Scene/></X3D>')
    assert (res.version, res.stamp) == ("3.0", "VERSION_INFERRED_BARE_FLOOR")


def test_future_version_not_clamped():
    res = _r('<X3D version="4.2"><Scene/></X3D>')
    assert res.version == "4.2"  # forward-compat: never floor a high version
```

> Verify the chosen node-floor fixture before relying on it: `MetadataDouble` must be in `x3d-3.1.json` and absent from `x3d-3.0.json`. Check with:
> `uv run python -c "import json,pathlib as p; d=p.Path('src/x3d_cpp_gen/conformance/manifests'); print('3.0', 'MetadataDouble' in json.loads((d/'x3d-3.0.json').read_text())['nodes']); print('3.1', 'MetadataDouble' in json.loads((d/'x3d-3.1.json').read_text())['nodes'])"`
> Expected: `3.0 False` / `3.1 True`. If not, pick another node whose `node_floor_map()` value is `3.1` and update the test's expected version + node name to match.

- [ ] **Step 2: Run test to verify it fails**

Run: `uv run pytest tests/conformance/test_version_inference.py -k "declared or xsd or floor or future" -v`
Expected: FAIL — `ImportError: cannot import name 'resolve_version'`

- [ ] **Step 3: Append the codes**

In `src/x3d_cpp_gen/conformance/codes.py`, inside the `CODES = {...}` dict (after `"VERSION_DECLARED": INFO,`), append:

```python
    "VERSION_INFERRED_XSD_CITED": INFO,     # version inferred from x3d-N.xsd/.dtd citation (high conf)
    "VERSION_INFERRED_NODE_FORCED": INFO,   # version inferred from highest-floor node used (high conf)
    "VERSION_INFERRED_PROFILE_FLOOR": INFO, # version inferred from PROFILE's defining version (medium)
    "VERSION_INFERRED_BARE_FLOOR": WARNING, # no signal; floored to 3.0 (low conf, low-trust snapshot)
```

- [ ] **Step 4: Write the ladder**

In `src/x3d_cpp_gen/conformance/version_resolve.py`, add imports and the resolver. Add to the existing imports at top:

```python
import re
from dataclasses import dataclass
```

Then append (after `detect_document_version`):

```python
# Lowest X3D version defining each profile (profile-floor rung, §1 step 3).
# Most corpus profiles are 3.0-era; newer profiles floor higher. Unknown → 3.0.
_PROFILE_FLOOR = {
    "Core": "3.0", "Interchange": "3.0", "Interactive": "3.0",
    "MPEG-4": "3.0", "Immersive": "3.0", "Full": "3.0",
    "CADInterchange": "3.1", "MedicalInterchange": "3.2",
}
_XSI = "{http://www.w3.org/2001/XMLSchema-instance}noNamespaceSchemaLocation"
_CITE_RE = re.compile(r"x3d-(\d\.\d)\.(?:xsd|dtd)")


@dataclass
class VersionResolution:
    """Resolved validation version + the confidence stamp that justifies it."""
    version: str          # always set; falls through to the 3.0 floor
    stamp: str            # VERSION_DECLARED or one of the VERSION_INFERRED_* codes
    declared: bool        # True only when the file carried an explicit version attr


def _cited_version(root) -> str | None:
    """x3d-N.xsd via xsi:noNamespaceSchemaLocation, or x3d-N.dtd via DOCTYPE SYSTEM."""
    loc = root.get(_XSI)
    if loc:
        m = _CITE_RE.search(loc)
        if m:
            return m.group(1)
    docinfo = root.getroottree().docinfo
    sysurl = docinfo.system_url if docinfo is not None else None
    if sysurl:
        m = _CITE_RE.search(sysurl)
        if m:
            return m.group(1)
    return None


def resolve_version(xml) -> VersionResolution:
    """Bind a document to a validation version via the VP-2 ladder (§1):
    declared → schema/DTD citation → node-floor → profile-floor → bare 3.0."""
    # Imported here to avoid a module-load cycle (node_floor imports load_manifest).
    from x3d_cpp_gen.conformance.node_floor import (
        FLOOR_VERSION, X3D_VERSIONS, node_floor_map, version_key,
    )
    root = etree.fromstring(xml.encode("utf-8") if isinstance(xml, str) else xml)

    # Rung 0 — declared version wins outright (forward-compat: never clamped).
    declared = root.get("version")
    if declared:
        return VersionResolution(declared, "VERSION_DECLARED", True)

    # Rung 1 — schema/DTD citation (strongest file-local signal).
    cited = _cited_version(root)
    if cited in X3D_VERSIONS:
        return VersionResolution(cited, "VERSION_INFERRED_XSD_CITED", False)

    # Rung 2 — node-floor: the highest earliest-version among nodes actually used.
    floor = node_floor_map()
    used = []
    for el in root.iter():
        if not isinstance(el.tag, str):
            continue
        qn = etree.QName(el)
        if qn.namespace:
            continue
        v = floor.get(qn.localname)
        if v:
            used.append(v)
    if used:
        return VersionResolution(max(used, key=version_key),
                                 "VERSION_INFERRED_NODE_FORCED", False)

    # Rung 3 — profile-floor.
    prof = root.get("profile")
    if prof:
        return VersionResolution(_PROFILE_FLOOR.get(prof, FLOOR_VERSION),
                                 "VERSION_INFERRED_PROFILE_FLOOR", False)

    # Rung 4 — bare floor (low-trust).
    return VersionResolution(FLOOR_VERSION, "VERSION_INFERRED_BARE_FLOOR", False)
```

- [ ] **Step 5: Run tests to verify they pass**

Run: `uv run pytest tests/conformance/test_version_inference.py -v`
Expected: PASS (9 tests total)

- [ ] **Step 6: Commit**

```bash
git add src/x3d_cpp_gen/conformance/codes.py src/x3d_cpp_gen/conformance/version_resolve.py tests/conformance/test_version_inference.py
git commit -m "conformance: VP-2 resolve_version inference ladder + tiered stamps

§1 ladder: declared > schema/DTD citation > node-floor > profile-floor > 3.0.
Adds VERSION_INFERRED_PROFILE_FLOOR to complete the spec's 4-rung ladder (the
prose enumerated only 3 stamps; the profile-floor rung had none). Forward-compat:
a version >= 3.0 (incl. future) is never floored."
---

## Task 3: Wire the ladder into `sweep.py`

`sweep` is the fast structural counter. Replace the `detect_document_version` + skip path with `resolve_version` (the floor always binds a manifest, so `skipped_no_manifest` should drop to ~0), and break out per-stamp counts.

**Files:**
- Modify: `src/x3d_cpp_gen/conformance/sweep.py`
- Test: `tests/conformance/test_version_inference.py`

- [ ] **Step 1: Write the failing test**

Append to `tests/conformance/test_version_inference.py`:

```python
def test_sweep_infers_unversioned(tmp_path):
    from x3d_cpp_gen.conformance.sweep import sweep
    # One declared 4.0 file + one unversioned file with a bare floor.
    (tmp_path / "a.x3d").write_text('<X3D version="4.0"><Scene><Box/></Scene></X3D>')
    (tmp_path / "b.x3d").write_text('<X3D><Scene><Box/></Scene></X3D>')
    counts = sweep(str(tmp_path))
    assert counts["validated"] == 2          # both bound to an oracle now
    assert counts["skipped_no_manifest"] == 0
    assert counts["inferred"] == 1           # only b.x3d was inferred
```

- [ ] **Step 2: Run test to verify it fails**

Run: `uv run pytest tests/conformance/test_version_inference.py::test_sweep_infers_unversioned -v`
Expected: FAIL — `KeyError: 'inferred'` (or AssertionError on `skipped_no_manifest`).

- [ ] **Step 3: Rewrite the sweep loop**

Replace the body of `sweep()` in `src/x3d_cpp_gen/conformance/sweep.py` (the function, keeping the `__main__` block) with:

```python
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
```

Update the module docstring's parenthetical to read: `binding each to its declared OR inferred version's manifest (VP-2 ladder; floor 3.0 always binds)`.

- [ ] **Step 4: Run test to verify it passes**

Run: `uv run pytest tests/conformance/test_version_inference.py::test_sweep_infers_unversioned -v`
Expected: PASS

- [ ] **Step 5: Run the existing sweep regression test (no break)**

Run: `uv run pytest tests/conformance/test_version_and_sweep.py -v`
Expected: PASS — if a test asserted the old `skipped_no_manifest` count on a fixture dir, update its expectation to the new inferred behavior (inferred files now validate). Adjust the test, not the code.

- [ ] **Step 6: Commit**

```bash
git add src/x3d_cpp_gen/conformance/sweep.py tests/conformance/test_version_inference.py tests/conformance/test_version_and_sweep.py
git commit -m "conformance: VP-2 sweep binds unversioned files via the inference ladder"
```

---

## Task 4: Wire the ladder into `report.py` (the citable artifact)

The L2 conformance report is the moat capstone. Bind via `resolve_version`, record inferred files under their inferred version, emit the resolution stamp as a `Finding`, and surface a per-stamp tally + a low-trust (BARE_FLOOR) count so snapshots are honestly weighted (§1, §3c).

**Files:**
- Modify: `src/x3d_cpp_gen/conformance/report.py:49-144`
- Test: `tests/conformance/test_version_inference.py`

- [ ] **Step 1: Write the failing test**

Append to `tests/conformance/test_version_inference.py`:

```python
def test_report_records_inference(tmp_path):
    from x3d_cpp_gen.conformance.report import build_report
    (tmp_path / "declared.x3d").write_text('<X3D version="4.0"><Scene><Box/></Scene></X3D>')
    (tmp_path / "bare.x3d").write_text('<X3D><Scene><Box/></Scene></X3D>')
    rpt = build_report(str(tmp_path))
    assert rpt["totals"]["validated"] == 2
    assert rpt["totals"].get("skippedNoManifest", 0) == 0
    inf = rpt["versionResolution"]
    assert inf["declared"] == 1
    assert inf["inferred"] == 1
    assert inf["byStamp"]["VERSION_INFERRED_BARE_FLOOR"] == 1
    assert inf["bareFloorLowTrust"] == 1
    # The bare file is bound to the 3.0 oracle, not skipped.
    assert "3.0" in rpt["perVersion"]
```

- [ ] **Step 2: Run test to verify it fails**

Run: `uv run pytest tests/conformance/test_version_inference.py::test_report_records_inference -v`
Expected: FAIL — `KeyError: 'versionResolution'`.

- [ ] **Step 3: Update the report build loop**

In `src/x3d_cpp_gen/conformance/report.py`:

(a) Change the import line `from x3d_cpp_gen.conformance.version_resolve import detect_document_version, load_manifest` to:

```python
from x3d_cpp_gen.conformance.version_resolve import resolve_version, load_manifest
```

(b) After `versions_seen = set()` (line ~59), add:

```python
    stamp_tally = collections.Counter()
    declared_n = inferred_n = 0
```

(c) Replace the per-file head of the `for p in files:` loop — from the `try:`/`vd = detect_document_version(xml)` block through the `if m is None:` skip — with:

```python
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
```

(The rest of the loop — `versions_seen.add(vd)`, `totals["validated"] += 1`, the clean/findings accounting — is unchanged and follows directly.)

(d) In the returned dict (after the `"perVersion": per_version_out,` line), add:

```python
        "versionResolution": {
            "declared": declared_n,
            "inferred": inferred_n,
            "byStamp": dict(sorted(stamp_tally.items())),
            "bareFloorLowTrust": stamp_tally["VERSION_INFERRED_BARE_FLOOR"],
        },
```

- [ ] **Step 4: Run test to verify it passes**

Run: `uv run pytest tests/conformance/test_version_inference.py::test_report_records_inference -v`
Expected: PASS

- [ ] **Step 5: Surface it in markdown + keep the report test green**

In `format_markdown()` (after the totals block it prints), add a line summarizing resolution. Locate the totals-printing lines and append:

```python
    vr = report.get("versionResolution", {})
    if vr:
        lines.append(
            f"- Version resolution: {vr['declared']} declared, {vr['inferred']} inferred "
            f"({vr['bareFloorLowTrust']} bare-floor low-trust)")
```

Run: `uv run pytest tests/conformance/test_report.py -v`
Expected: PASS — if `test_report.py` asserts exact top-level keys, add `"versionResolution"` to its expected set. Adjust the test.

- [ ] **Step 6: Commit**

```bash
git add src/x3d_cpp_gen/conformance/report.py tests/conformance/test_version_inference.py tests/conformance/test_report.py
git commit -m "conformance: VP-2 report binds + stamps inferred versions; bare-floor low-trust tally"
```

---

## Task 5: VRML97→3.0 floor + sub-3.0 clamp at read time (C++)

The §8 root fix. `parseHeaderLine` runs before the dialect hook and currently sets `doc.version` verbatim from `parts[1]`, so `#VRML V2.0` leaves `doc.version="2.0"`. Floor `#VRML*` headers and any sub-3.0 version to "3.0"; leave ≥3.0 (incl. future) untouched. Also set the floor in `Vrml97Reader::onHeaderLine`'s missing/garbled-header branches (where `parseHeaderLine` saw no version token and left the "4.0" default).

**Files:**
- Modify: `runtime/parse/ClassicVrmlReader.hpp:161-170`
- Modify: `runtime/parse/Vrml97Reader.hpp:106-145`
- Create: `runtime/parse/tests/version_floor_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing test**

Create `runtime/parse/tests/version_floor_test.cpp`:

```cpp
// version_floor_test.cpp
// VP-2 §8: a VRML97 (#VRML V2.0) source — and any sub-3.0 version token — must
// floor to a valid X3D version (3.0) at read time, so no writer ever emits the
// invalid `#X3D V2.0` header. A version >= 3.0 (incl. future) is left untouched.
#include "X3DParse.hpp"
#include "X3DRuntime.hpp"
#include "VrmlWriter.hpp"

#include <iostream>
#include <string>

static int failures = 0;
static void check(bool c, const std::string &what) {
  if (!c) { std::cerr << "FAIL: " << what << "\n"; ++failures; }
  else std::cout << "ok: " << what << "\n";
}

int main() {
  using namespace x3d;

  // VRML97 .wrl header -> floored to 3.0.
  {
    auto doc = codec::parseDocument("#VRML V2.0 utf8\nGroup {}\n", codec::Encoding::Vrml97);
    check(doc.version == "3.0", "VRML97 #VRML V2.0 floors doc.version to 3.0");
    codec::VrmlWriter w;
    std::string out = w.writeDocument(doc);
    check(out.find("#X3D V2.0") == std::string::npos, "writer never emits #X3D V2.0");
    check(out.find("#X3D V3.0 utf8") != std::string::npos, "writer emits #X3D V3.0 utf8");
  }

  // ClassicVRML .x3dv carrying a stray sub-3.0 token -> clamped to 3.0.
  {
    auto doc = codec::parseDocument("#X3D V2.0 utf8\nGroup {}\n", codec::Encoding::ClassicVrml);
    check(doc.version == "3.0", "ClassicVRML sub-3.0 version clamps to 3.0");
  }

  // A real X3D version is preserved (no over-clamping).
  {
    auto doc = codec::parseDocument("#X3D V4.0 utf8\nGroup {}\n", codec::Encoding::ClassicVrml);
    check(doc.version == "4.0", "X3D V4.0 preserved");
  }

  // Forward-compat: a future >=3.0 version is NOT floored.
  {
    auto doc = codec::parseDocument("#X3D V4.2 utf8\nGroup {}\n", codec::Encoding::ClassicVrml);
    check(doc.version == "4.2", "future X3D V4.2 not clamped");
  }

  std::cout << (failures ? "FAILED\n" : "PASSED\n");
  return failures ? 1 : 0;
}
```

> If `codec::parseDocument`'s exact signature/`Encoding` enumerators differ, mirror the call style already used at the bottom of `runtime/parse/tests/vrml97_reader_test.cpp` (front-door dispatch section). Do not invent an API.

- [ ] **Step 2: Register the test in CMake**

In `CMakeLists.txt`, after the `x3d_codec_roundtrip` test block (~line 324), add:

```cmake
    add_executable(x3d_version_floor
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/parse/tests/version_floor_test.cpp")
    target_link_libraries(x3d_version_floor PRIVATE x3d_cpp::x3d_cpp)
    add_test(NAME x3d_version_floor COMMAND x3d_version_floor)
```

- [ ] **Step 3: Run the test to verify it fails**

Run: `cmake --build build -j4 --target x3d_version_floor && ctest --test-dir build -R x3d_version_floor --output-on-failure`
Expected: FAIL — the `#VRML V2.0` and `#X3D V2.0` cases report `doc.version == "2.0"`.

- [ ] **Step 4: Implement the clamp in `parseHeaderLine`**

In `runtime/parse/ClassicVrmlReader.hpp`, replace the `if (parts.size() >= 2)` block in `parseHeaderLine` (lines ~161-169) with:

```cpp
    // A `#VRML`/`#VRML97` payload is VRML97 — floor to the X3D 3.0 baseline
    // (VP-2 §8). X3D has no V2.0/V1.0, so never let such a token reach a writer.
    if (!parts.empty() && (parts[0] == "#VRML" || parts[0] == "#VRML97")) {
      doc.version = "3.0";
      return;
    }
    if (parts.size() >= 2) {
      // parts[1] is like "V4.0" or "V2.0"; strip a leading 'V'/'v'.
      std::string ver = parts[1];
      if (!ver.empty() && (ver.front() == 'V' || ver.front() == 'v'))
        ver.erase(ver.begin());
      if (!ver.empty()) {
        // Clamp a sub-3.0 (legacy/VRML) major up to the 3.0 floor; leave >= 3.0
        // (incl. future versions we have no manifest for) untouched.
        int major = 0;
        try { major = std::stoi(ver.substr(0, ver.find('.'))); } catch (...) { major = 3; }
        doc.version = (major < 3) ? std::string("3.0") : ver;
      }
    }
```

(Add `#include <stdexcept>` near the other includes if not already present; `<string>` is already included.)

- [ ] **Step 5: Floor the headerless VRML97 branches**

In `runtime/parse/Vrml97Reader.hpp`, `onHeaderLine`, set the floor in the branches where `parseHeaderLine` left the default (no usable version token). Add `doc.version = "3.0";` alongside each `doc.profile = runtime::Profile::Immersive;` in:
- the missing-header branch (line ~108),
- the unrecognized-magic branch (line ~127),
- the trailing `#VRML` success path (line ~144).

(The `magic == "#X3D"` branch at line ~116 must NOT set a floor — it returns, preserving the version `parseHeaderLine` already clamped.)

- [ ] **Step 6: Run the test to verify it passes**

Run: `cmake --build build -j4 --target x3d_version_floor && ctest --test-dir build -R x3d_version_floor --output-on-failure`
Expected: PASS (all checks; "PASSED")

- [ ] **Step 7: Commit**

```bash
git add runtime/parse/ClassicVrmlReader.hpp runtime/parse/Vrml97Reader.hpp runtime/parse/tests/version_floor_test.cpp CMakeLists.txt
git commit -m "codec: VP-2 floor VRML97/sub-3.0 headers to X3D 3.0 at read (fixes #X3D V2.0)"
```

---

## Task 6: Defensive version clamp in `VrmlWriter` (C++)

Belt-and-suspenders: a document built programmatically (not via a reader) could still carry `version="2.0"`. The writer must never serialize a sub-3.0 X3D header.

**Files:**
- Modify: `runtime/codecs/VrmlWriter.hpp:45`
- Test: `runtime/parse/tests/version_floor_test.cpp`

- [ ] **Step 1: Add the failing test case**

In `runtime/parse/tests/version_floor_test.cpp`, before the final summary, add:

```cpp
  // Defensive: a hand-built doc with a sub-3.0 version still writes a valid header.
  {
    runtime::X3DDocument doc;
    doc.version = "2.0";
    codec::VrmlWriter w;
    std::string out = w.writeDocument(doc);
    check(out.find("#X3D V2.0") == std::string::npos, "writer clamps hand-built V2.0");
    check(out.find("#X3D V3.0 utf8") != std::string::npos, "writer floors hand-built doc to 3.0");
  }
```

- [ ] **Step 2: Run to verify it fails**

Run: `cmake --build build -j4 --target x3d_version_floor && ctest --test-dir build -R x3d_version_floor --output-on-failure`
Expected: FAIL — header line reads `#X3D V2.0 utf8`.

- [ ] **Step 3: Implement the writer clamp**

In `runtime/codecs/VrmlWriter.hpp`, replace line 45 (`os << "#X3D V" << doc.version << " utf8\n";`) with:

```cpp
    os << "#X3D V" << headerVersion(doc.version) << " utf8\n";
```

And add a private static helper to the `VrmlWriter` class (near the other helpers):

```cpp
  /// X3D has no version below 3.0; floor a sub-3.0/legacy token to "3.0" so the
  /// emitted header is always valid (VP-2 §8). >= 3.0 (incl. future) passes through.
  static std::string headerVersion(const std::string &v) {
    int major = 0;
    try { major = std::stoi(v.substr(0, v.find('.'))); } catch (...) { return "3.0"; }
    return (major < 3) ? std::string("3.0") : v;
  }
```

(`<string>` and `<stdexcept>` are already included by the writer's existing headers; add `#include <stdexcept>` if the build complains.)

- [ ] **Step 4: Run to verify it passes**

Run: `cmake --build build -j4 --target x3d_version_floor && ctest --test-dir build -R x3d_version_floor --output-on-failure`
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add runtime/codecs/VrmlWriter.hpp runtime/parse/tests/version_floor_test.cpp
git commit -m "codec: VP-2 VrmlWriter defensively floors sub-3.0 version in header"
```

---

## Task 7: Full verification (golden gate + pytest + ctest + corpus)

VP-2 is codegen-free: the generated bindings are untouched, so the golden hash MUST stay byte-identical. Prove it, then prove the conformance gain on the real corpus (133 unversioned files now bind to an oracle instead of skipping).

**Files:** none (verification only).

- [ ] **Step 1: Golden gate — byte-identical**

Run: `mise run golden`
Expected: PASS, no drift. (No `.hpp`/`.cpp` under `generated_cpp_bindings/` changed — VP-2 touched only `runtime/` and `src/x3d_cpp_gen/conformance/`.)

- [ ] **Step 2: Python suite green**

Run: `mise run test`
Expected: PASS — prior count + the new `test_version_inference.py` cases (8 unit + sweep + report).

- [ ] **Step 3: C++ suite green**

Run: `cmake --build build -j4 && ctest --test-dir build --output-on-failure`
Expected: PASS — all prior tests + `x3d_version_floor`. (`-j4`, NOT unbounded `-j` — the all-headers TU OOMs.)

- [ ] **Step 4: Corpus sweep — unversioned files now bind**

Run:
```bash
uv run python -c "from x3d_cpp_gen.conformance.sweep import sweep; import json; print(json.dumps(sweep('<x3d-render-workspace>/testdata'), indent=2))"
```
Expected: `skipped_no_manifest` drops from ~133 toward ~0; `inferred` ≈ 133; `validated` rises by ~133. Record the before/after numbers for the commit/memory note.

- [ ] **Step 5: Regenerate the citable report and eyeball the resolution block**

Run:
```bash
uv run python -m x3d_cpp_gen.conformance.report <x3d-render-workspace>/testdata --json 2>/dev/null | python3 -c "import sys,json; r=json.load(sys.stdin); print(json.dumps(r['versionResolution'], indent=2)); print('skippedNoManifest', r['totals'].get('skippedNoManifest'))"
```
Expected: `declared` + `inferred` ≈ total validated; `byStamp` shows the tier breakdown (XSD_CITED ~66, NODE_FORCED for the rest, BARE_FLOOR ~5); `skippedNoManifest` near 0.

- [ ] **Step 6: Final commit (verification record)**

```bash
git add -A
git commit -m "conformance: VP-2 verified — golden byte-identical, 133 unversioned files now bound

pytest <N>, ctest <M>/<M>, golden unchanged. Corpus skipped_no_manifest <before>-><after>;
inferred ~133 (XSD_CITED <x>, NODE_FORCED <y>, PROFILE_FLOOR <z>, BARE_FLOOR <w>)." || echo "nothing to commit"
```

---

## Self-Review notes (completed by plan author)

- **Spec coverage:** §1 ladder → Tasks 1-4; tiered stamps → Task 2 (+PROFILE_FLOOR gap closed); §8 VRML97→3.0 floor + read-time remap → Task 5; `#X3D V2.0` writer bug → Tasks 5 (root) + 6 (defense). §3b XSD on-admission audit and §3c full L3 abstention machinery are explicitly OUT of this increment (BARE_FLOOR low-trust flagging in Task 4 is the lightweight stand-in); they remain DESIGNED-UNBUILT fast-follows.
- **Type consistency:** `VersionResolution(version, stamp, declared)` used identically in Tasks 2/3/4; `node_floor_map()`, `version_key()`, `X3D_VERSIONS`, `FLOOR_VERSION` consistent; C++ `headerVersion()` clamp logic mirrors `parseHeaderLine`'s major<3 rule.
- **Codegen-free:** no template/parser-of-UOM change → golden hash invariant (gated in Task 7).
