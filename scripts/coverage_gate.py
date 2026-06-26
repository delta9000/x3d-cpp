#!/usr/bin/env python3
"""ADR coverage gate.

`docs/wiki/coverage.md` is a hand-maintained manifest whose "Decisions (ADRs)"
table is meant to enumerate every ADR. It silently drifted (ADRs 0028..0036
existed on disk but were never added; one row was duplicated) — this gate stops
that recurring.

It cross-checks the ADR files on disk (`docs/wiki/decisions/NNNN-*.md`) against
the rows of the Decisions table in `coverage.md`, and fails (exit 1) on any:
  - ADR file with no matching row (the manifest undercounts), or
  - row pointing at a missing file / duplicated slug, or
  - non-contiguous ADR numbering (a gap means an ADR was deleted or skipped).

Stdlib only; no build needed. Wired as `mise run coverage-gate` and into `ci`.
"""
from __future__ import annotations

import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
DECISIONS = REPO / "docs" / "wiki" / "decisions"
COVERAGE = REPO / "docs" / "wiki" / "coverage.md"

ADR_FILE_RE = re.compile(r"^(\d{4})-[a-z0-9-]+\.md$")
# A Decisions-table row references the page as `decisions/NNNN-...md`.
ROW_RE = re.compile(r"`decisions/((\d{4})-[a-z0-9-]+\.md)`")


def adr_files() -> dict[str, str]:
    """slug-number -> filename for every ADR file on disk."""
    out: dict[str, str] = {}
    for p in sorted(DECISIONS.glob("*.md")):
        m = ADR_FILE_RE.match(p.name)
        if m:
            out[m.group(1)] = p.name
    return out


def coverage_rows() -> list[tuple[str, str]]:
    """(number, filename) for every `decisions/...md` cited in a table ROW.

    Only Markdown table rows (lines starting with `|`) count — prose mentions of
    an ADR slug elsewhere on the page (e.g. the residuals section) are not rows.
    """
    out: list[tuple[str, str]] = []
    for line in COVERAGE.read_text().splitlines():
        if not line.lstrip().startswith("|"):
            continue
        for m in ROW_RE.finditer(line):
            out.append((m.group(2), m.group(1)))
    return out


def main() -> int:
    files = adr_files()
    rows = coverage_rows()
    errs: list[str] = []

    # Duplicate rows.
    seen: set[str] = set()
    for num, fname in rows:
        if num in seen:
            errs.append(f"coverage.md lists ADR-{num} more than once")
        seen.add(num)

    listed = {num for num, _ in rows}
    row_fname = {num: fname for num, fname in rows}

    # File without a row.
    for num, fname in files.items():
        if num not in listed:
            errs.append(f"ADR file {fname} has no row in coverage.md")

    # Row without a file, or row slug != file slug.
    for num in listed:
        if num not in files:
            errs.append(f"coverage.md row ADR-{num} points at a non-existent file")
        elif row_fname[num] != files[num]:
            errs.append(
                f"coverage.md row for ADR-{num} cites {row_fname[num]!r} "
                f"but the file is {files[num]!r}"
            )

    # Contiguous numbering (a gap == a deleted/skipped ADR).
    nums = sorted(int(n) for n in files)
    if nums:
        expected = set(range(nums[0], nums[-1] + 1))
        missing = sorted(expected - set(nums))
        if missing:
            errs.append(
                "ADR numbering is not contiguous; missing: "
                + ", ".join(f"{m:04d}" for m in missing)
            )

    if errs:
        print("coverage-gate: FAIL", file=sys.stderr)
        for e in errs:
            print(f"  - {e}", file=sys.stderr)
        return 1

    print(f"coverage-gate: OK — {len(files)} ADRs, all rowed and contiguous")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
