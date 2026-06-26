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
import hashlib
import json
import os
import pathlib
import re
from typing import Optional

HERE = pathlib.Path(__file__).parent
MANIFEST_DIR = HERE / "manifests"
ANCHORS_PATH = HERE / "prose_anchors.json"
# Local mirror of the X3D normative-prose markdown (external, not bundled). Set
# X3D_SPEC_PROSE_DIR to point at it; when unset, fall back to a non-existent
# sentinel so prose-dependent code/tests skip cleanly (rather than "" -> cwd).
PROSE_DIR = pathlib.Path(
    os.environ.get("X3D_SPEC_PROSE_DIR") or (HERE / "_x3d_spec_prose_unset")
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
            # tiebreak: exact-title first, then largest/most-specific section number,
            # shallowest heading, then file order — all deterministic over sorted inputs.
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


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="prose_anchors")
    sub = ap.add_subparsers(dest="cmd", required=True)
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


if __name__ == "__main__":
    raise SystemExit(main())
