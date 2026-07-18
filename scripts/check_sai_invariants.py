#!/usr/bin/env python3
"""Entry point for C++ SAI invariant schema and coverage validation."""

from sai_conformance import main as conformance_main


if __name__ == "__main__":
    raise SystemExit(conformance_main(["check"]))
