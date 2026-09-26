#!/usr/bin/env python3
"""SAI service register validator.

`docs/conformance/sai-services.yaml` is x3d-cpp's side of the SAI service
register split out in ADR-0047: the services implemented by x3d-cpp
(`x3d::runtime::SaiContext`, `x3d::codec`), each citing the CTest / doctest
case that exercises it. This gate loads the register, checks its schema
(required keys, allowed `kind`/`support` values, unique ids, the `defaults:`
anchors exactly matching the merge keys rows use), and checks that every
`tests:` name it cites actually exists in the C++ test sources — an
`add_test(NAME …)` registration in CMake or a `TEST_CASE("…")` in a .cpp.

Wired as `mise run sai-services` and exercised by `tests/test_sai_services.py`.
Stdlib + pyyaml only; no build needed.
"""
from __future__ import annotations

import argparse
import pathlib
import re
import sys

import yaml

REPO = pathlib.Path(__file__).resolve().parent.parent
REGISTER = REPO / "docs" / "conformance" / "sai-services.yaml"

SCHEMA_VERSION = 1
REQUIRED_KEYS = ("kind", "id", "clause", "public_symbol", "support", "tests", "preserves")
KINDS = ("browser_query", "query", "command", "async", "console")
SUPPORTS = ("none", "partial", "full")

ANCHOR_RE = re.compile(r"^\s*(?:-\s+)?<<:\s*\*([A-Za-z0-9_]+)", re.MULTILINE)
ADD_TEST_RE = re.compile(r'add_test\(NAME\s+"?([A-Za-z0-9_.\-]+)"?')
TEST_CASE_RE = re.compile(r'TEST_CASE\(\s*"((?:[^"\\]|\\.)*)"')

# Source trees scanned for evidence. generated_cpp_bindings is skipped: its
# TEST_CASEs (if any) are generated, not hand-authored evidence.
_SCAN_SKIP = {".git", "build", "generated_cpp_bindings", "node_modules", ".venv"}


def _walk(repo: pathlib.Path, suffixes: tuple[str, ...]) -> list[pathlib.Path]:
    out = []
    for p in repo.rglob("*"):
        if any(part in _SCAN_SKIP for part in p.parts):
            continue
        if p.suffix in suffixes and p.is_file():
            out.append(p)
    return out


def cmake_files(repo: pathlib.Path) -> list[pathlib.Path]:
    root = repo / "CMakeLists.txt"
    files = [root] if root.exists() else []
    return files + _walk(repo, (".cmake", ".txt"))


def collect_evidence(repo: pathlib.Path) -> set[str]:
    """Every CTest name (add_test) and doctest case (TEST_CASE) in the repo."""
    names: set[str] = set()
    for p in cmake_files(repo):
        names.update(ADD_TEST_RE.findall(p.read_text(errors="ignore")))
    for p in _walk(repo, (".cpp", ".hpp")):
        names.update(TEST_CASE_RE.findall(p.read_text(errors="ignore")))
    return names


def referenced_anchors(text: str) -> set[str]:
    return set(ANCHOR_RE.findall(text))


def load_register(path: pathlib.Path) -> dict:
    return yaml.safe_load(path.read_text())


def check_schema(register: object, text: str) -> list[str]:
    """Schema violations as human-readable strings ([] means clean)."""
    errs: list[str] = []
    if not isinstance(register, dict):
        return [f"top level must be a mapping, got {type(register).__name__}"]

    if register.get("schema_version") != SCHEMA_VERSION:
        errs.append(f"schema_version must be {SCHEMA_VERSION}, "
                    f"got {register.get('schema_version')!r}")

    defaults = register.get("defaults")
    if not isinstance(defaults, dict):
        errs.append("missing/non-mapping `defaults:` block")
        defaults = {}
    refs = referenced_anchors(text)
    undefined = refs - set(defaults)
    if undefined:
        errs.append(f"rows reference undefined anchors: {sorted(undefined)}")
    unused = set(defaults) - refs
    if unused:
        errs.append(f"defaults define unused anchors: {sorted(unused)}")

    services = register.get("services")
    if not isinstance(services, list) or not services:
        errs.append("missing/empty `services:` list")
        return errs

    seen: set[str] = set()
    for i, row in enumerate(services):
        where = row.get("id", f"#{i}") if isinstance(row, dict) else f"#{i}"
        if not isinstance(row, dict):
            errs.append(f"{where}: row must be a mapping")
            continue
        for key in REQUIRED_KEYS:
            if key not in row:
                errs.append(f"{where}: missing required key `{key}`")
        if row.get("kind") not in KINDS:
            errs.append(f"{where}: kind must be one of {list(KINDS)}, "
                        f"got {row.get('kind')!r}")
        if row.get("support") not in SUPPORTS:
            errs.append(f"{where}: support must be one of {list(SUPPORTS)}, "
                        f"got {row.get('support')!r}")
        tests = row.get("tests")
        if not isinstance(tests, list) or not tests:
            errs.append(f"{where}: `tests` must be a non-empty list")
        rid = row.get("id")
        if isinstance(rid, str):
            if rid in seen:
                errs.append(f"{where}: duplicate id")
            seen.add(rid)
    return errs


def check_evidence(register: dict, evidence: set[str]) -> list[str]:
    errs: list[str] = []
    for row in register.get("services", []):
        if not isinstance(row, dict):
            continue
        for name in row.get("tests", []) or []:
            if name not in evidence:
                errs.append(f"{row.get('id')}: unknown test evidence `{name}`")
    return errs


def run(repo: pathlib.Path = REPO) -> list[str]:
    register_path = repo / "docs" / "conformance" / "sai-services.yaml"
    text = register_path.read_text()
    register = load_register(register_path)
    errs = check_schema(register, text)
    if errs:
        return errs
    return check_evidence(register, collect_evidence(repo))


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--repo", default=None, help="repo root (default: auto)")
    args = ap.parse_args(argv)
    repo = pathlib.Path(args.repo) if args.repo else REPO
    errs = run(repo)
    if errs:
        print("SAI service register: FAIL", file=sys.stderr)
        for e in errs:
            print(f"  {e}", file=sys.stderr)
        return 1
    register_path = repo / "docs" / "conformance" / "sai-services.yaml"
    n = len(yaml.safe_load(register_path.read_text())["services"])
    print(f"SAI service register OK: {n} services, schema + evidence valid.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
