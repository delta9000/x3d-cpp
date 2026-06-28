#!/usr/bin/env python3
"""Sweep the Web3D X3D Conformance (NIST) example set through the OpenGL PoC renderer.

Two passes per scene:
  * --headless  (default): parse + extract probe, NO GL — fast, runs anywhere.
                Buckets: OK / OK_EMPTY (0 render items: silent drop) / PARSE_FAIL /
                CRASH (signal) / TIMEOUT / ERROR.
  * --gl (opt-in): the real OpenGL pipeline via `poc --screenshot` under xvfb-run +
                mesa software GL (llvmpipe). Buckets: GL_OK / GL_FAIL / GL_CRASH /
                GL_TIMEOUT. Slower; honors --limit / --section to keep it bounded.

Outcomes are grouped by section (Appearance, Geometry, Lights, ...) and failures
collapse by a normalized signature (paths/numbers elided) so the long tail ranks
into distinct gaps. Always exits 0 — a diagnostic, not a gate.

Corpus: fetch with  scripts/fetch_conformance_nist.sh  (or unzip the Web3D
X3dExamplesConformanceNist.zip under .x3d-corpus/x3d-code/).

Usage:
  scripts/poc_conformance_sweep.py [CORPUS_ROOT] [--bin PATH] [--gl] [--timeout S]
                                   [--section NAME] [--limit N] [--jsonl FILE]
"""
from __future__ import annotations
import argparse, json, os, re, subprocess, sys, tempfile
from collections import Counter, defaultdict
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
DEFAULT_ROOT = REPO / ".x3d-corpus/x3d-code/www.web3d.org/x3d/content/examples/ConformanceNist"
DEFAULT_BIN = REPO / "build-poc/examples/poc_renderer/x3d_poc_renderer"
RENDER_ITEMS = re.compile(r"render_items=(\d+)")

def normalize(msg: str) -> str:
    """Collapse a failure message to a signature: drop paths, numbers, quotes."""
    msg = re.sub(r"/\S+", "<path>", msg)
    msg = re.sub(r"\b\d+\b", "N", msg)
    msg = re.sub(r"['\"][^'\"]*['\"]", "<q>", msg)
    return msg.strip()[:160]

def headless(binp: str, scene: str, timeout: float) -> dict:
    try:
        p = subprocess.run([binp, "--headless", scene], capture_output=True,
                           text=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return {"bucket": "TIMEOUT", "items": None, "sig": "timeout"}
    out = (p.stdout or "") + (p.stderr or "")
    m = RENDER_ITEMS.search(out)
    items = int(m.group(1)) if m else None
    if p.returncode < 0:
        return {"bucket": "CRASH", "items": items, "sig": f"signal {-p.returncode}"}
    if p.returncode != 0:
        pf = re.search(r"parse failed: (.*)", out)
        if pf:
            return {"bucket": "PARSE_FAIL", "items": items, "sig": normalize(pf.group(1))}
        last = next((l for l in reversed(out.splitlines()) if l.strip()), "exit nonzero")
        return {"bucket": "ERROR", "items": items, "sig": normalize(last)}
    if items == 0:
        return {"bucket": "OK_EMPTY", "items": 0, "sig": "0 render items"}
    return {"bucket": "OK", "items": items, "sig": ""}

def gl(binp: str, scene: str, timeout: float) -> dict:
    shot = Path(tempfile.mkdtemp()) / "shot.ppm"
    env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE="1", GALLIUM_DRIVER="llvmpipe",
               __GLX_VENDOR_LIBRARY_NAME="mesa")
    try:
        p = subprocess.run(["xvfb-run", "-a", binp, "--screenshot", str(shot), scene],
                           capture_output=True, text=True, timeout=timeout, env=env)
    except subprocess.TimeoutExpired:
        return {"bucket": "GL_TIMEOUT", "sig": "timeout"}
    if p.returncode < 0:
        return {"bucket": "GL_CRASH", "sig": f"signal {-p.returncode}"}
    if p.returncode != 0 or not shot.exists() or shot.stat().st_size == 0:
        last = next((l for l in reversed((p.stderr or "").splitlines()) if l.strip()), "no frame")
        return {"bucket": "GL_FAIL", "sig": normalize(last)}
    return {"bucket": "GL_OK", "sig": ""}

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("root", nargs="?", default=str(DEFAULT_ROOT))
    ap.add_argument("--bin", default=str(DEFAULT_BIN))
    ap.add_argument("--gl", action="store_true", help="also run the real GL pipeline under xvfb")
    ap.add_argument("--timeout", type=float, default=20.0)
    ap.add_argument("--section", help="restrict to one section dir (e.g. Geometry)")
    ap.add_argument("--limit", type=int, default=0, help="sweep at most N scenes")
    ap.add_argument("--jsonl", help="write per-scene results here")
    a = ap.parse_args()

    root = Path(a.root)
    if not Path(a.bin).exists():
        print(f"poc binary not found: {a.bin}\n  build it:  mise run poc", file=sys.stderr)
        return 0
    if not root.exists():
        print(f"corpus not found: {root}\n  fetch it:  scripts/fetch_conformance_nist.sh", file=sys.stderr)
        return 0

    base = root / a.section if a.section else root
    scenes = sorted(str(p) for p in base.rglob("*.x3d"))
    if a.limit:
        scenes = scenes[:a.limit]
    print(f"sweeping {len(scenes)} scene(s) under {base}  ({'GL+headless' if a.gl else 'headless'})\n")

    by_bucket = Counter()
    by_section = defaultdict(Counter)
    sigs = defaultdict(Counter)
    jl = open(a.jsonl, "w") if a.jsonl else None
    for i, s in enumerate(scenes, 1):
        section = Path(s).relative_to(root).parts[0] if Path(s).relative_to(root).parts else "(root)"
        r = headless(a.bin, s, a.timeout)
        if a.gl and r["bucket"] in ("OK", "OK_EMPTY"):
            r.update(gl(a.bin, s, a.timeout))  # GL bucket overrides when reached
        by_bucket[r["bucket"]] += 1
        by_section[section][r["bucket"]] += 1
        if r.get("sig"):
            sigs[r["bucket"]][r["sig"]] += 1
        if jl:
            jl.write(json.dumps({"scene": os.path.relpath(s, root), "section": section, **r}) + "\n")
        if i % 100 == 0:
            print(f"  ...{i}/{len(scenes)}", file=sys.stderr)
    if jl:
        jl.close()

    print("=== outcomes ===")
    for b, n in by_bucket.most_common():
        print(f"  {b:12} {n:4}  ({100*n/len(scenes):.1f}%)")
    print("\n=== per-section (OK / total) ===")
    for sec in sorted(by_section):
        c = by_section[sec]
        ok = c["OK"] + c.get("GL_OK", 0)
        print(f"  {sec:20} {ok:3}/{sum(c.values()):<3}  " +
              " ".join(f"{b}:{n}" for b, n in c.most_common() if b not in ("OK", "GL_OK")))
    print("\n=== top failure signatures ===")
    for bucket in ("PARSE_FAIL", "ERROR", "CRASH", "GL_FAIL", "GL_CRASH"):
        if sigs[bucket]:
            print(f"\n[{bucket}]")
            for sig, n in sigs[bucket].most_common(12):
                print(f"  {n:3}x  {sig}")
    return 0

if __name__ == "__main__":
    sys.exit(main())
