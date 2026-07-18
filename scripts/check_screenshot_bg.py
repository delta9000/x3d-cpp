#!/usr/bin/env python3
"""Background-dominance check for a P6 PPM screenshot.

Reports the fraction of pixels within `--tol` (per 0-255 channel) of a reference
background colour, and asserts it against `--min` / `--max`. Used by the RND-1
GL alpha-cutout gate in scripts/validate-examples.sh: a MASK panel whose alpha is
below the cutoff must be fully discarded (frame ~= 100% background), while the
opaque control panel must render (background clearly below 100%). Robust to
software-Mesa/llvmpipe shading noise because the signal is "did a whole panel of
pixels appear or not", not exact per-pixel colour.

No third-party deps (mirrors the stdlib-only tooling used elsewhere in scripts/).
"""
import argparse
import sys


def read_ppm_p6(path):
    with open(path, "rb") as f:
        data = f.read()
    if not data.startswith(b"P6"):
        raise ValueError(f"{path}: not a binary P6 PPM")
    # Parse header: magic, width, height, maxval — whitespace-separated, then one
    # whitespace byte before the pixel block. Skip '#' comment lines defensively.
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


def bg_fraction(px, bg, tol):
    br, bgc, bb = bg
    n = len(px) // 3
    hit = 0
    for k in range(0, n * 3, 3):
        if (abs(px[k] - br) <= tol and abs(px[k + 1] - bgc) <= tol
                and abs(px[k + 2] - bb) <= tol):
            hit += 1
    return hit / n if n else 0.0


def parse_color(s):
    parts = [int(x) for x in s.split(",")]
    if len(parts) != 3:
        raise argparse.ArgumentTypeError("colour must be r,g,b (0-255)")
    return tuple(parts)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("ppm")
    ap.add_argument("--bg", type=parse_color, required=True, help="r,g,b 0-255")
    ap.add_argument("--tol", type=int, default=24, help="per-channel tolerance")
    ap.add_argument("--min", type=float, default=None, help="fail if frac < MIN")
    ap.add_argument("--max", type=float, default=None, help="fail if frac > MAX")
    ap.add_argument("--label", default="")
    args = ap.parse_args()

    w, h, px = read_ppm_p6(args.ppm)
    frac = bg_fraction(px, args.bg, args.tol)
    tag = f"[{args.label}] " if args.label else ""
    print(f"{tag}{args.ppm}: {w}x{h}, background-fraction={frac:.4f} "
          f"(bg={args.bg} tol={args.tol})")

    ok = True
    if args.min is not None and frac < args.min:
        print(f"{tag}FAIL: background-fraction {frac:.4f} < min {args.min}",
              file=sys.stderr)
        ok = False
    if args.max is not None and frac > args.max:
        print(f"{tag}FAIL: background-fraction {frac:.4f} > max {args.max}",
              file=sys.stderr)
        ok = False
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
