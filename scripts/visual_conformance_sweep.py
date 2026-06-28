#!/usr/bin/env python3
"""Visual conformance sweep: line up the PoC renderer against the Web3D/NIST
conformance reference renders, scored by a local vision model.

For every conformance scene that ships a `*-front.jpg` reference view (declared via
`<meta name='Image'>`), this:
  1. renders the scene through the OpenGL PoC with `--front` (canonical straight-on
     bounding-sphere-fit camera, matching how NIST generates its front view) under
     xvfb + mesa software GL,
  2. fetches+caches the NIST front reference next to the scene,
  3. asks a local OpenAI-compatible vision model (default gemma) whether OUR render
     depicts the SAME 3D subject as the reference — tolerant of camera nuance,
     lighting, background and resolution, which legitimately differ.

Verdicts bucket into match / minor / mismatch (+ render_fail / no_ref). Results are
written to a JSONL that the sweep RESUMES from, so it can be ground out in the
background across many invocations. Always exits 0 — a diagnostic, not a gate.

The reference images are open-source (Web3D Consortium BSD-style license); we fetch
on demand and cache under the gitignored corpus tree, never redistribute.

Usage:
  scripts/visual_conformance_sweep.py [CORPUS_ROOT] [--bin PATH] [--section NAME]
      [--limit N] [--workers N] [--model NAME] [--endpoint URL] [--out FILE]
      [--width PX] [--timeout S] [--redo]
"""
from __future__ import annotations
import argparse, base64, concurrent.futures as cf, io, json, os, re, subprocess, sys, tempfile, urllib.request
from collections import Counter, defaultdict
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
DEFAULT_ROOT = REPO / ".x3d-corpus/x3d-code/www.web3d.org/x3d/content/examples/ConformanceNist"
DEFAULT_BIN = REPO / "build-poc/examples/poc_renderer/x3d_poc_renderer"
URL_BASE = "https://www.web3d.org/x3d/content/examples/ConformanceNist"
DEFAULT_OUT = REPO / ".x3d-corpus/visual_sweep.jsonl"
RENDER_ENV = dict(LIBGL_ALWAYS_SOFTWARE="1", GALLIUM_DRIVER="llvmpipe",
                  __GLX_VENDOR_LIBRARY_NAME="mesa")
IMAGE_META = re.compile(r"name=['\"]Image['\"]\s+content=['\"]([^'\"]+)['\"]"
                        r"|content=['\"]([^'\"]+)['\"]\s+name=['\"]Image['\"]")
# url='...' or url="..."; the value is an MFString that itself contains quoted
# tokens (relative path + absolute URL), so capture the whole value then tokenize.
URL_ATTR = re.compile(r"\burl=(?:'([^']*)'|\"([^\"]*)\")")
TEX_EXT = (".png", ".jpg", ".jpeg", ".gif", ".bmp")

PROMPT = (
    "You are checking a 3D renderer for visual conformance. "
    "IMAGE 1 is the REFERENCE render of an X3D conformance scene. "
    "IMAGE 2 is OUR renderer's output of the SAME scene from the same front camera. "
    "They legitimately differ in lighting, shading, background color, anti-aliasing and "
    "resolution — IGNORE all of those. Judge only whether IMAGE 2 depicts the SAME 3D "
    "subject: same kind and shape of geometry, roughly the same number of distinct objects, "
    "same overall color/material family, same rough layout, and present (not blank when the "
    "reference shows something). "
    "Reply ONLY with compact JSON and nothing else: "
    '{"verdict":"match|minor|mismatch","ref":"<=8 word desc of image1",'
    '"ours":"<=8 word desc of image2","why":"<=15 words"}. '
    "verdict=match: same subject, only the ignorable differences. "
    "minor: same subject but a notable difference (extra/missing minor detail, wrong shade, "
    "off position). mismatch: different geometry, wrong object count, wrong colors, or ours "
    "is blank/missing geometry that the reference clearly shows."
)


def front_ref(scene: Path) -> str | None:
    """Return the `*-front.*` reference image filename declared by the scene, if any."""
    try:
        txt = scene.read_text(errors="replace")
    except OSError:
        return None
    imgs = [m.group(1) or m.group(2) for m in IMAGE_META.finditer(txt)]
    for img in imgs:
        if "front" in img.lower():
            return img
    return None


def fetch_ref(scene: Path, root: Path, img: str, timeout: float) -> Path | None:
    """Fetch+cache the reference image next to the scene in the corpus tree."""
    rel = scene.parent.relative_to(root)
    dest = root / rel / img
    if dest.exists() and dest.stat().st_size > 0:
        return dest
    url = f"{URL_BASE}/{rel.as_posix()}/{img}"
    try:
        with urllib.request.urlopen(url, timeout=timeout) as r:
            data = r.read()
        if not data:
            return None
        dest.write_bytes(data)
        return dest
    except Exception:
        return None


def fetch_textures(scene: Path, root: Path, timeout: float) -> None:
    """Fetch+cache image textures the scene references, where poc expects them.

    Each X3D `url=` MFString lists a relative path then (usually) an absolute
    https source. poc resolves the relative path against the scene dir, so we
    save the bytes there. Movie/script textures (.mpg/.js) are skipped — poc
    can't render them and they'd only add noise. Best-effort; failures are mute.
    """
    try:
        txt = scene.read_text(errors="replace")
    except OSError:
        return
    for g1, g2 in URL_ATTR.findall(txt):
        toks = re.findall(r"[^\s\"']+", g1 or g2)
        if not toks:
            continue
        rel = toks[0]
        if not rel.lower().endswith(TEX_EXT):
            continue
        dest = (scene.parent / rel).resolve()
        try:
            dest.relative_to(root.resolve())
        except ValueError:
            continue  # don't write outside the corpus tree
        if dest.exists() and dest.stat().st_size > 0:
            continue
        src = next((t for t in toks if t.startswith("http")), None)
        if not src:
            relposix = dest.relative_to(root.resolve()).as_posix()
            src = f"{URL_BASE}/{relposix}"
        try:
            with urllib.request.urlopen(src, timeout=timeout) as r:
                data = r.read()
            if data:
                dest.parent.mkdir(parents=True, exist_ok=True)
                dest.write_bytes(data)
        except Exception:
            pass


def render(binp: str, scene: Path, width: int, timeout: float,
           front: bool = True) -> bytes | None:
    """Render the scene; return downscaled PNG bytes, or None on failure.

    front=True forces the canonical NIST front camera (right for geometry scenes
    whose `*-front.jpg` reference is a fixed front view). Set front=False for
    bindable-Viewpoint tests, where the NIST reference is the scene's BOUND
    viewpoint and forcing front would falsely mismatch."""
    from PIL import Image
    with tempfile.TemporaryDirectory() as td:
        ppm = Path(td) / "shot.ppm"
        env = dict(os.environ, **RENDER_ENV)
        cmd = ["xvfb-run", "-a", binp] + (["--front"] if front else [])
        cmd += ["--screenshot", str(ppm), str(scene)]
        try:
            p = subprocess.run(cmd, capture_output=True, timeout=timeout, env=env)
        except subprocess.TimeoutExpired:
            return None
        if p.returncode != 0 or not ppm.exists() or ppm.stat().st_size == 0:
            return None
        im = Image.open(ppm).convert("RGB")
        if im.width > width:
            im = im.resize((width, round(im.height * width / im.width)))
        buf = io.BytesIO()
        im.save(buf, format="PNG")
        return buf.getvalue()


def data_uri(path_or_bytes, mime: str) -> str:
    raw = path_or_bytes if isinstance(path_or_bytes, bytes) else Path(path_or_bytes).read_bytes()
    return f"data:{mime};base64,{base64.b64encode(raw).decode()}"


def compare(endpoint: str, model: str, ref: Path, ours_png: bytes, timeout: float) -> dict:
    ref_mime = "image/png" if ref.suffix.lower() == ".png" else "image/jpeg"
    body = {
        "model": model, "temperature": 0, "max_tokens": 8192,
        # gemma-4 is a reasoning model; left on, thinking eats the whole token
        # budget and returns empty content (finish_reason=length). Disable it so
        # the answer JSON lands in `content` directly — and keep a generous 8k
        # ceiling so nothing truncates even if a build re-enables reasoning.
        "chat_template_kwargs": {"enable_thinking": False},
        "messages": [{"role": "user", "content": [
            {"type": "text", "text": PROMPT},
            {"type": "text", "text": "IMAGE 1 (reference):"},
            {"type": "image_url", "image_url": {"url": data_uri(ref, ref_mime)}},
            {"type": "text", "text": "IMAGE 2 (ours):"},
            {"type": "image_url", "image_url": {"url": data_uri(ours_png, "image/png")}},
        ]}],
    }
    req = urllib.request.Request(endpoint, data=json.dumps(body).encode(),
                                 headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        resp = json.load(r)
    content = resp["choices"][0]["message"]["content"]
    finish = resp["choices"][0].get("finish_reason")
    m = re.search(r"\{.*\}", content, re.S)
    if not m:
        return {"verdict": "parse_fail", "raw": content[:200], "finish": finish}
    try:
        out = json.loads(m.group(0))
    except json.JSONDecodeError:
        return {"verdict": "parse_fail", "raw": content[:200], "finish": finish}
    out["finish"] = finish
    return out


def process(scene: Path, root: Path, a) -> dict:
    rel = scene.relative_to(root).as_posix()
    rec = {"scene": rel, "section": scene.relative_to(root).parts[0]}
    img = front_ref(scene)
    if not img:
        rec["bucket"] = "no_ref"
        return rec
    ref = fetch_ref(scene, root, img, a.timeout)
    if not ref:
        rec["bucket"] = "no_ref"
        rec["why"] = f"reference {img} unavailable"
        return rec
    fetch_textures(scene, root, a.timeout)
    ours = render(a.bin, scene, a.width, a.timeout, front=not a.no_front)
    if ours is None:
        rec["bucket"] = "render_fail"
        return rec
    try:
        v = compare(a.endpoint, a.model, ref, ours, a.timeout)
    except Exception as e:
        rec["bucket"] = "compare_fail"
        rec["why"] = str(e)[:120]
        return rec
    rec["bucket"] = v.get("verdict", "parse_fail")
    rec.update({k: v[k] for k in ("ref", "ours", "why", "raw") if k in v})
    return rec


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("root", nargs="?", default=str(DEFAULT_ROOT))
    ap.add_argument("--bin", default=str(DEFAULT_BIN))
    ap.add_argument("--section", help="restrict to one section dir (e.g. Geometry)")
    ap.add_argument("--no-front", action="store_true",
                    help="render the scene's bound viewpoint, not the canonical front "
                         "camera (use for BindableNodes/Viewpoint tests)")
    ap.add_argument("--limit", type=int, default=0)
    ap.add_argument("--workers", type=int, default=4)
    ap.add_argument("--model", default="gemma-4-12b-it")
    ap.add_argument("--endpoint", default="http://localhost:8080/v1/chat/completions")
    ap.add_argument("--out", default=str(DEFAULT_OUT))
    ap.add_argument("--width", type=int, default=640, help="downscale our render to this width")
    ap.add_argument("--timeout", type=float, default=300.0)
    ap.add_argument("--redo", action="store_true", help="ignore prior results, sweep all")
    a = ap.parse_args()

    root = Path(a.root)
    if not Path(a.bin).exists():
        print(f"poc binary not found: {a.bin}\n  build it:  mise run poc", file=sys.stderr)
        return 0
    if not root.exists():
        print(f"corpus not found: {root}\n  fetch it:  scripts/fetch_conformance_nist.sh", file=sys.stderr)
        return 0

    base = root / a.section if a.section else root
    scenes = sorted(base.rglob("*.x3d"))

    done: dict[str, dict] = {}
    out_path = Path(a.out)
    if out_path.exists() and not a.redo:
        for line in out_path.read_text().splitlines():
            if line.strip():
                r = json.loads(line)
                done[r["scene"]] = r
    todo = [s for s in scenes if s.relative_to(root).as_posix() not in done]
    if a.limit:
        todo = todo[:a.limit]
    print(f"{len(scenes)} scene(s) under {base}; {len(done)} already scored; "
          f"sweeping {len(todo)} now ({a.workers} workers, model {a.model})\n", flush=True)

    results = list(done.values())
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_path, "a") as jl:
        with cf.ThreadPoolExecutor(max_workers=a.workers) as ex:
            futs = {ex.submit(process, s, root, a): s for s in todo}
            for i, fut in enumerate(cf.as_completed(futs), 1):
                rec = fut.result()
                jl.write(json.dumps(rec) + "\n"); jl.flush()
                results.append(rec)
                if rec["bucket"] in ("mismatch", "render_fail", "compare_fail"):
                    print(f"  [{rec['bucket']:11}] {rec['scene']}  "
                          f"{rec.get('why', rec.get('ours',''))}", flush=True)
                if i % 25 == 0:
                    print(f"  ...{i}/{len(todo)}", file=sys.stderr, flush=True)

    by_bucket = Counter(r["bucket"] for r in results)
    by_section = defaultdict(Counter)
    for r in results:
        by_section[r["section"]][r["bucket"]] += 1
    n = len(results) or 1
    print("\n=== outcomes ===")
    for b, c in by_bucket.most_common():
        print(f"  {b:12} {c:4}  ({100*c/n:.1f}%)")
    print("\n=== per-section (match / scored-with-ref) ===")
    for sec in sorted(by_section):
        c = by_section[sec]
        ref_total = sum(v for k, v in c.items() if k != "no_ref")
        bad = " ".join(f"{k}:{v}" for k, v in c.most_common() if k not in ("match", "no_ref"))
        print(f"  {sec:18} {c['match']:3}/{ref_total:<3}  {bad}")
    print("\n=== mismatches ===")
    for r in results:
        if r["bucket"] == "mismatch":
            print(f"  {r['scene']}\n      ref: {r.get('ref','')}  | ours: {r.get('ours','')}  | {r.get('why','')}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
