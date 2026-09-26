#!/usr/bin/env python3
"""Non-blank check for a P6 PPM render of a gallery scene.

A first-party showcase scene can extract to non-empty geometry yet still render
to a flat frame — a single background colour, or a clean gradient sky and
nothing else — if the camera, lighting, or material binding rots. This asserts
the frame actually contains a visible subject. The background of each image row
is taken to be that row's first and last pixel (a vertical sky/ground gradient
is constant along a row); the check fails when fewer than `--min` of the pixels
differ from their row's background, i.e. the image is a flat fill or a bare
gradient.

Tolerance is per 0-255 channel, matching scripts/check_screenshot_bg.py, because
software shading is not exactly reproducible. No third-party deps (stdlib only).
"""
import argparse
import sys


def read_ppm_p6(path):
    with open(path, "rb") as f:
        data = f.read()
    if not data.startswith(b"P6"):
        raise ValueError(f"{path}: not a binary P6 PPM")
    tokens = []
    i = 2
    while len(tokens) < 3:
        while i < len(data) and data[i:i + 1].isspace():
            i += 1
        if data[i:i + 1] == b"#":
            while i < len(data) and data[i:i + 1] not in (b"\n", b"\r"):
                i += 1
            continue
        start = i
        while i < len(data) and not data[i:i + 1].isspace():
            i += 1
        tokens.append(data[start:i])
    w, h, maxval = (int(t) for t in tokens)
    if maxval != 255:
        raise ValueError(f"{path}: only 8-bit PPM supported (maxval={maxval})")
    i += 1  # single whitespace separator after maxval
    px = data[i:i + w * h * 3]
    if len(px) < w * h * 3:
        raise ValueError(f"{path}: pixel data truncated")
    return w, h, px


def _close(p, q, tol):
    return (abs(p[0] - q[0]) <= tol and abs(p[1] - q[1]) <= tol
            and abs(p[2] - q[2]) <= tol)


def nonblank_fraction(w, h, px, tol):
    """Fraction of pixels that differ (by > tol per channel) from their row's
    background.

    A tall sky/ground gradient (a Background node) is constant along a row, so
    the row's first and last pixel are that row's background; a flat clear fill
    is the same for every row. Treating the background per row — instead of as
    one global most-common colour — keeps a gradient sky from being mistaken for
    a subject. ~0.0 means blank / background-only; higher means a subject
    rendered."""
    n = w * h
    if n == 0:
        return 0.0
    diff = 0
    for y in range(h):
        base = y * w * 3
        first = px[base:base + 3]
        last = px[base + (w - 1) * 3:base + w * 3]
        for x in range(w):
            k = base + x * 3
            p = px[k:k + 3]
            if not _close(p, first, tol) and not _close(p, last, tol):
                diff += 1
    return diff / n


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("ppm")
    ap.add_argument("--tol", type=int, default=8, help="per-channel tolerance")
    ap.add_argument("--min", type=float, default=0.01,
                    help="fail if non-background fraction < MIN")
    ap.add_argument("--label", default="")
    args = ap.parse_args()

    w, h, px = read_ppm_p6(args.ppm)
    frac = nonblank_fraction(w, h, px, args.tol)
    tag = f"[{args.label}] " if args.label else ""
    print(f"{tag}{args.ppm}: {w}x{h}, non-background-fraction={frac:.4f} "
          f"(tol={args.tol})")
    if frac < args.min:
        print(f"{tag}FAIL: frame is blank/background-only "
              f"(non-background-fraction {frac:.4f} < min {args.min})",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
