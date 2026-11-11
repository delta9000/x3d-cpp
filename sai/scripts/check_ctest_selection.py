#!/usr/bin/env python3
"""Reject doctest-filter CTests that silently select no cases.

This checks selection only. CTest must still execute the selected cases. Never
interpret a nonzero count as a pass or as complete normative evidence.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import subprocess
import sys

FILTER = re.compile(r"^(?:--(?:test-case|test-suite)(?:-exclude)?=|-t[cs]e?=)")
COUNT = re.compile(r"unskipped test cases passing the current filters:\s*(\d+)")


def selection_errors(tests, run=subprocess.run):
    errors = []
    selected = 0
    for test in tests:
        command = test.get("command", [])
        if not any(FILTER.match(arg) for arg in command[1:]):
            continue
        selected += 1
        name = test.get("name", "unnamed CTest")
        try:
            result = run(command + ["--count", "--no-colors"], capture_output=True,
                         text=True, timeout=30, check=False)
        except (OSError, subprocess.TimeoutExpired) as exc:
            errors.append(f"{name}: cannot query selection: {exc}")
            continue
        match = COUNT.search(result.stdout)
        if result.returncode or not match:
            errors.append(f"{name}: doctest count failed or was not reported")
        elif int(match.group(1)) == 0:
            errors.append(f"{name}: doctest filter selects zero cases")
    return selected, errors


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build_dir", type=Path)
    parser.add_argument("--ctest", default="ctest")
    args = parser.parse_args(argv)
    try:
        result = subprocess.run(
            [args.ctest, "--test-dir", str(args.build_dir), "--show-only=json-v1"],
            check=True, capture_output=True, text=True, timeout=30,
        )
        tests = json.loads(result.stdout)["tests"]
        selected, errors = selection_errors(tests)
    except (OSError, subprocess.SubprocessError, ValueError, KeyError) as exc:
        print(f"CTest selection check failed: {exc}", file=sys.stderr)
        return 1
    if not selected:
        errors.append("No filtered doctest CTests found; verify the configured build")
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"CTest selection OK: {selected} filtered tests select nonzero cases")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
