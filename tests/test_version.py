"""Phase 5 multi-version tests.

Covers:
- version AUTO-DETECTION from the UOM XML (4.0 detected as "4.0"),
- the SpecVersion model helpers (slug / namespace / detect),
- unknown-but-well-formed type tolerance (data-driven, actionable error),
- a generation-from-a-SECOND-version run against the synthetic 4.1 fixture,
  asserting the brand-new node / enum member appear and 4.0 assumptions don't
  break.

The synthetic fixture is self-contained, so these tests never require network
access. A separate test opportunistically uses the real 4.1 UOM if present.
"""

from importlib.resources import files
from pathlib import Path

import pytest

from x3d_cpp_gen.model.version import (
    SpecVersion,
    detect_version_from_file,
)
from x3d_cpp_gen.model.types import TypeRegistry, X3DType, resolve_x3d_type
from x3d_cpp_gen.parser import (
    parse_x3d_model,
    parse_enum_definitions,
    build_dependency_graph,
)
from x3d_cpp_gen.generator import (
    FIELD_TYPE_MAPPING,
    XS_TYPES,
    generate_cpp_bindings,
    write_types_header,
    write_enums_header,
)


SPEC_40 = files("x3d_cpp_gen").joinpath("data", "X3dUnifiedObjectModel-4.0.xml")
FIXTURES = Path(__file__).parent / "fixtures"
SYNTHETIC_41 = FIXTURES / "X3dUnifiedObjectModel-4.1-synthetic.xml"
REAL_41 = FIXTURES / "X3dUnifiedObjectModel-4.1.xml"


# --- Auto-detection -------------------------------------------------------

def test_autodetect_40_from_packaged_spec():
    assert detect_version_from_file(str(SPEC_40)) == "4.0"


def test_specversion_detect_40():
    sv = SpecVersion.detect(str(SPEC_40))
    assert sv.version == "4.0"
    assert sv.major == 4 and sv.minor == 0
    assert sv.spec_path == Path(str(SPEC_40))


def test_autodetect_41_from_synthetic_fixture():
    assert detect_version_from_file(str(SYNTHETIC_41)) == "4.1"


def test_specversion_helpers():
    sv = SpecVersion(version="4.1")
    assert sv.slug == "4_1"
    assert sv.cpp_namespace == "v4_1"
    assert sv.minor == 1


def test_detect_raises_on_unversioned(tmp_path):
    bad = tmp_path / "noversion.xml"
    bad.write_text('<X3dUnifiedObjectModel></X3dUnifiedObjectModel>')
    with pytest.raises(ValueError, match="auto-detect"):
        SpecVersion.detect(str(bad))


# --- Type registry tolerance (data-driven, actionable) --------------------

def test_unknown_type_resolves_to_none_not_crash():
    # A well-formed but unregistered type from a hypothetical newer spec must
    # not crash: resolve() returns None (parser then skips-with-warning).
    assert resolve_x3d_type("SFQuaternion") is None


def test_resolve_or_raise_is_actionable():
    with pytest.raises(KeyError, match="register it as DATA"):
        TypeRegistry.resolve_or_raise("SFQuaternion")
    # Known types still resolve via the same path.
    assert TypeRegistry.resolve_or_raise("SFBool") is X3DType.SFBool


# --- Generation from a SECOND version (synthetic 4.1) ---------------------

def test_synthetic_41_parses_new_node_and_enum():
    nodes = parse_x3d_model(str(SYNTHETIC_41), FIELD_TYPE_MAPPING, XS_TYPES)
    # Brand-new node not present in 4.0.
    assert "QuantumWidget" in nodes
    assert "Box" not in nodes  # synthetic subset, distinct from 4.0
    qw = nodes["QuantumWidget"]
    assert qw.base_type == "X3DChildNode"
    field_names = {f.name for f in qw.fields}
    assert {"entangled", "spin", "quality"} <= field_names

    enum_defs = parse_enum_definitions(str(SYNTHETIC_41))
    assert "textureQualityChoices" in enum_defs
    members = {m.value for m in enum_defs["textureQualityChoices"].members}
    # The brand-new 4.1-only enum member flows through the data-driven path.
    assert "superSharp" in members


def test_synthetic_41_generates_and_emits_new_content(tmp_path):
    nodes = parse_x3d_model(str(SYNTHETIC_41), FIELD_TYPE_MAPPING, XS_TYPES)
    graph = build_dependency_graph(nodes)
    enum_defs = parse_enum_definitions(str(SYNTHETIC_41))
    out = tmp_path / "v41syn"
    out.mkdir()
    core_dir = out / "x3d" / "core"
    core_dir.mkdir(parents=True, exist_ok=True)
    write_types_header(str(core_dir))
    write_enums_header(str(core_dir), enum_defs)
    generate_cpp_bindings(nodes, graph, str(out),
                          clang_format="", enum_defs=enum_defs)

    qw_hpp = out / "x3d" / "nodes" / "QuantumWidget.hpp"
    assert qw_hpp.exists()
    text = qw_hpp.read_text()
    assert "class QuantumWidget" in text
    assert "public virtual X3DChildNode" in text

    enums_text = (out / "x3d" / "core" / "X3Denums.hpp").read_text()
    assert "SUPERSHARP" in enums_text


def test_synthetic_41_namespace_isolation(tmp_path):
    # A namespaced emission wraps each class; the default 4.0 path (namespace="")
    # is exercised by the golden tests and stays unnamespaced.
    nodes = parse_x3d_model(str(SYNTHETIC_41), FIELD_TYPE_MAPPING, XS_TYPES)
    graph = build_dependency_graph(nodes)
    out = tmp_path / "v41ns"
    out.mkdir()
    core_dir = out / "x3d" / "core"
    core_dir.mkdir(parents=True, exist_ok=True)
    write_types_header(str(core_dir))
    write_enums_header(str(core_dir), {})
    generate_cpp_bindings(nodes, graph, str(out), clang_format="",
                          enum_defs={}, namespace="x3d::v4_1")
    text = (out / "x3d" / "nodes" / "QuantumWidget.hpp").read_text()
    assert "namespace x3d::v4_1 {" in text
    assert "} // namespace x3d::v4_1" in text


# --- Real 4.1 UOM (only if the large fixture is present) ------------------

@pytest.mark.skipif(not REAL_41.exists(),
                    reason="real X3D 4.1 UOM fixture not present")
def test_real_41_autodetect_and_new_nodes():
    assert detect_version_from_file(str(REAL_41)) == "4.1"
    nodes = parse_x3d_model(str(REAL_41), FIELD_TYPE_MAPPING, XS_TYPES)
    # Nodes introduced in X3D 4.1 (absent from 4.0).
    nodes_40 = parse_x3d_model(str(SPEC_40), FIELD_TYPE_MAPPING, XS_TYPES)
    assert "EnvironmentLight" in nodes
    assert "EnvironmentLight" not in nodes_40
    assert len(nodes) > len(nodes_40)
