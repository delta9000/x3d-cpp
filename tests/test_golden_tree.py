"""Full-tree golden-drift test.

Regenerates the ENTIRE generated C++ source tree into a temp dir and asserts
every generated source file (*.hpp and *.cpp) matches the committed
generated_cpp_bindings/ byte-for-byte (in both directions: no missing, no extra,
no drifted files). This is the pytest twin of scripts/check_golden.sh, so
`uv run pytest` alone catches codegen drift.

Codegen changes are intentional: change a template/emitter, regenerate with
`uv run x3d-cpp-gen --out generated_cpp_bindings`, and commit the new sources.
"""

import os
import subprocess
from importlib.resources import files
from pathlib import Path

import pytest

SPEC = files("x3d_cpp_gen").joinpath("data", "X3dUnifiedObjectModel-4.0.xml")
REPO_ROOT = Path(__file__).resolve().parent.parent
GOLDEN_DIR = REPO_ROOT / "generated_cpp_bindings"

# Byte-for-byte equality only holds with the pinned formatter. Every test here
# takes the pinned_clang_format fixture (conftest.py), which skips locally /
# fails under CI without it, and hands its path to the generator and
# check_golden.sh through CLANG_FORMAT.


def _env_with(clang_format: str) -> dict:
    return {**os.environ, "CLANG_FORMAT": clang_format}


def test_golden_tree_matches(generated_tree, pinned_clang_format):
    # generated_tree (conftest.py) used the pinned formatter, which the
    # pinned_clang_format fixture guarantees is available.
    assert GOLDEN_DIR.is_dir(), f"golden dir missing: {GOLDEN_DIR}"
    out = generated_tree

    def _tree(root):
        # test.cpp is the gitignored smoke-test artifact, not golden — exclude it
        # so a leftover from `mise run gen` / the README's first command (both of
        # which emit test.cpp) doesn't trip the drift gate.
        return {p.relative_to(root)
                for ext in ("*.hpp", "*.cpp")
                for p in root.rglob(ext)
                if p.name != "test.cpp"}
    golden = _tree(GOLDEN_DIR)
    produced = _tree(out)

    assert golden, "no golden sources found"

    missing = sorted(str(p) for p in golden - produced)
    extra = sorted(str(p) for p in produced - golden)
    assert not missing, f"generated sources in golden but not regenerated: {missing}"
    assert not extra, f"generated sources regenerated but not committed to golden: {extra}"

    drifted = []
    for rel in sorted(golden, key=str):
        if (GOLDEN_DIR / rel).read_bytes() != (out / rel).read_bytes():
            drifted.append(str(rel))
    assert not drifted, (
        "regenerated sources differ from committed golden "
        f"(regenerate + commit): {drifted}"
    )


def test_check_golden_script_passes(pinned_clang_format):
    """The shell drift gate exits 0 on a clean tree (covers the CI script too)."""
    script = REPO_ROOT / "scripts" / "check_golden.sh"
    assert script.exists(), f"missing {script}"
    result = subprocess.run(
        ["bash", str(script)], cwd=str(REPO_ROOT),
        env=_env_with(pinned_clang_format), capture_output=True, text=True,
    )
    assert result.returncode == 0, (
        f"check_golden.sh reported drift on a clean tree:\n"
        f"{result.stdout}\n{result.stderr}"
    )
