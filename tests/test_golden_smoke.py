"""Golden smoke test: regenerating into a tmp dir reproduces a committed header byte-for-byte."""

import shutil
from importlib.resources import files
from pathlib import Path

import pytest

from x3d_cpp_gen.parser import parse_x3d_model, build_dependency_graph
from x3d_cpp_gen.generator import (
    FIELD_TYPE_MAPPING,
    XS_TYPES,
    write_types_header,
    generate_cpp_bindings,
)

SPEC = files("x3d_cpp_gen").joinpath("data", "X3dUnifiedObjectModel-4.0.xml")
REPO_ROOT = Path(__file__).resolve().parent.parent
GOLDEN_DIR = REPO_ROOT / "generated_cpp_bindings"

# clang-format produced the golden formatting; the byte-for-byte comparison only
# holds when it is available. Skip (rather than fail) if it is missing.
HAVE_CLANG_FORMAT = shutil.which("clang-format") is not None

# Headers under x3d/core/ (everything else is under x3d/nodes/).
_CORE_HEADERS = {"X3Dtypes.hpp", "X3Denums.hpp", "X3DReflection.hpp"}


@pytest.mark.skipif(not HAVE_CLANG_FORMAT, reason="clang-format not installed")
@pytest.mark.parametrize("header", ["Box.hpp", "X3Dtypes.hpp"])
def test_generated_header_matches_golden(tmp_path, header):
    subdir = Path("x3d") / ("core" if header in _CORE_HEADERS else "nodes")
    golden = GOLDEN_DIR / subdir / header
    assert golden.exists(), f"golden header missing: {golden}"

    nodes = parse_x3d_model(str(SPEC), FIELD_TYPE_MAPPING, XS_TYPES)
    assert nodes
    graph = build_dependency_graph(nodes)

    out = tmp_path / "out"
    out.mkdir()
    core_dir = out / "x3d" / "core"
    core_dir.mkdir(parents=True, exist_ok=True)
    write_types_header(str(core_dir))
    generate_cpp_bindings(nodes, graph, str(out), clang_format="clang-format",
                          namespace="x3d::nodes")

    produced = out / subdir / header
    assert produced.exists(), f"generator did not emit {header}"
    assert produced.read_bytes() == golden.read_bytes(), (
        f"{header} differs from committed golden output"
    )
