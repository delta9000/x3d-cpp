#!/usr/bin/env python3
"""Doc ctest-command gate.

Subsystem/guide wiki pages document how to run tests with `ctest -R <regex>`.
The project registers AGGREGATE test executables (`add_test(NAME x3d_<x>_tests …)`)
and does NOT use doctest CMake discovery, so a `-R` that names an individual
doctest CASE (e.g. `x3d_mat4`) matches nothing and returns "No tests found".

This gate greps every `ctest … -R <arg>` in docs/wiki and fails (exit 1) if the
`-R` regex matches NONE of the real `add_test(NAME …)` targets in CMakeLists.txt
(gated targets included — we read the names, not a configured build). Keeps the
documented commands runnable. Wired as `mise run doc-ctest-gate` and into `ci`.

Stdlib only.
"""
from __future__ import annotations

import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
WIKI = REPO / "docs" / "wiki"
CMAKELISTS = REPO / "CMakeLists.txt"

# A documented invocation: `ctest … -R <arg>` where <arg> is a (quoted) regex.
CTEST_R_RE = re.compile(r"ctest\b[^\n`]*?-R\s+(\"[^\"]+\"|'[^']+'|\S+)")


def real_targets() -> set[str]:
    text = CMAKELISTS.read_text()
    return set(re.findall(r"add_test\(NAME\s+([A-Za-z0-9_]+)", text))


def doc_invocations() -> list[tuple[pathlib.Path, int, str]]:
    """(path, lineno, -R-arg) for every ctest -R in the wiki."""
    out: list[tuple[pathlib.Path, int, str]] = []
    for p in sorted(WIKI.rglob("*.md")):
        for i, line in enumerate(p.read_text().splitlines(), 1):
            for m in CTEST_R_RE.finditer(line):
                arg = m.group(1).strip("\"'")
                # Cut a trailing inline-code close + sentence punctuation so a real
                # target written as `… -R x3d_codecs_tests`. isn't mis-read.
                arg = arg.split("`")[0].rstrip(".,;")
                if arg:
                    out.append((p, i, arg))
    return out


def main() -> int:
    targets = real_targets()
    if not targets:
        print("doc-ctest-gate: FAIL — no add_test(NAME …) targets found", file=sys.stderr)
        return 1

    broken: list[tuple[pathlib.Path, int, str]] = []
    checked = 0
    for path, lineno, arg in doc_invocations():
        # Only police x3d_* test invocations (skip generic/illustrative -R args).
        if "x3d_" not in arg:
            continue
        checked += 1
        try:
            pat = re.compile(arg)
        except re.error:
            # Not a usable regex on its own — fall back to token containment.
            toks = re.findall(r"x3d_[a-z0-9_]+", arg)
            if all(any(t in tgt for tgt in targets) for t in toks):
                continue
            broken.append((path, lineno, arg))
            continue
        if not any(pat.search(t) for t in targets):
            broken.append((path, lineno, arg))

    if broken:
        print("doc-ctest-gate: FAIL — documented `ctest -R` targets that match no "
              "add_test(NAME …):", file=sys.stderr)
        for path, lineno, arg in broken:
            print(f"  - {path.relative_to(REPO)}:{lineno}  -R {arg}", file=sys.stderr)
        print(f"\n  {len(broken)} broken of {checked} checked. Real targets are the "
              "aggregate `x3d_*_tests` executables; cite those (doctest CASE names "
              "are not ctest targets).", file=sys.stderr)
        return 1

    print(f"doc-ctest-gate: OK — {checked} documented `ctest -R x3d_*` invocations "
          "all resolve")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
