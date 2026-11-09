"""Full-tree golden-drift test.

Regenerates the ENTIRE generated C++ source tree into a temp dir and asserts
every generated source file (*.hpp and *.cpp) matches the committed
generated_cpp_bindings/ byte-for-byte (in both directions: no missing, no extra,
no drifted files). Both `uv run pytest` and scripts/check_golden.sh use this comparator.

Codegen changes are intentional: change a template/emitter, regenerate with
`uv run x3d-cpp-gen --out generated_cpp_bindings`, and commit the new sources.
"""

from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
GOLDEN_DIR = REPO_ROOT / "generated_cpp_bindings"

def test_golden_tree_matches(pinned_clang_format, generated_tree):
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
