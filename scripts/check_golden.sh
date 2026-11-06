#!/usr/bin/env bash
# Reuse pytest's full-tree comparator; standalone verification must not skip.
set -euo pipefail
cd "$(dirname "$0")/.."
CI=1 uv run pytest -q tests/test_golden_tree.py
