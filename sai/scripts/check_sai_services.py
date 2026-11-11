#!/usr/bin/env python3
"""Compatibility entry point for the cross-linked C++ SAI service gate."""

import argparse

from sai_conformance import main as conformance_main


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--strict", action="store_true")
    args = parser.parse_args()
    return conformance_main(["strict" if args.strict else "check"])


if __name__ == "__main__":
    raise SystemExit(main())
