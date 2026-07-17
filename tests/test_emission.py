"""End-to-end emission tests: render real-spec nodes and assert on the C++.

These render through the actual backend + Jinja template (clang-format disabled
so the test is deterministic regardless of clang-format availability) and assert
on the emitted text for the Phase 4 features.
"""

from importlib.resources import files

import pytest

from x3d_cpp_gen.parser import (
    parse_x3d_model, build_dependency_graph, parse_enum_definitions,
)
from x3d_cpp_gen.backends.cpp_header import CppHeaderBackend
from x3d_cpp_gen.generator import (
    FIELD_TYPE_MAPPING, XS_TYPES, write_types_header, write_enums_header,
)


SPEC = files("x3d_cpp_gen").joinpath("data", "X3dUnifiedObjectModel-4.0.xml")


@pytest.fixture(scope="module")
def rendered(tmp_path_factory):
    nodes, _skipped = parse_x3d_model(str(SPEC), FIELD_TYPE_MAPPING, XS_TYPES)
    graph = build_dependency_graph(nodes)
    enum_defs = parse_enum_definitions(str(SPEC))
    out = tmp_path_factory.mktemp("out")
    core_dir = out / "x3d" / "core"
    core_dir.mkdir(parents=True, exist_ok=True)
    write_types_header(str(core_dir))
    write_enums_header(str(core_dir), enum_defs)
    backend = CppHeaderBackend(clang_format="", enum_defs=enum_defs)
    backend.emit(nodes, graph, str(out))
    return out


# Headers that live under x3d/core/ (all others go to x3d/nodes/).
_CORE_HEADERS = {"X3Dtypes.hpp", "X3Denums.hpp", "X3DReflection.hpp"}


def _read(rendered, name):
    # Collapse runs of whitespace so assertions are robust to clang-format /
    # Jinja indentation differences.
    if name in _CORE_HEADERS:
        path = rendered / "x3d" / "core" / name
    else:
        path = rendered / "x3d" / "nodes" / name
    raw = path.read_text()
    return raw, " ".join(raw.split())


def test_appearance_alpha_mode_is_enum(rendered):
    raw, src = _read(rendered, "Appearance.hpp")
    assert "AlphaModeChoices getAlphaMode()" in src
    assert "AlphaModeChoices _alphaMode{ AlphaModeChoices::AUTO }" in src
    # Must NOT be a plain std::string member any more.
    assert "std::string _alphaMode" not in src


def test_appearance_acceptable_node_types(rendered):
    raw, src = _read(rendered, "Appearance.hpp")
    assert "acceptableMaterialNodeTypes()" in src
    assert '"X3DMaterialNode"' in src


def test_material_container_field_and_component(rendered):
    raw, src = _read(rendered, "Material.hpp")
    assert 'getDefaultContainerField() { return "material"; }' in src
    assert 'componentName() { return "Shape"; }' in src
    assert "componentLevel() { return 1; }" in src


def test_no_inaccessible_base_inheritance_all_virtual(rendered):
    # The diamond fix: every base is emitted 'public virtual'.
    raw, src = _read(rendered, "LayoutGroup.hpp")
    assert "class LayoutGroup : public virtual X3DNode, public virtual X3DGroupingNode" in src
    # No non-virtual 'public X3D...' base should remain on the class line.
    class_line = next(l for l in raw.splitlines() if l.startswith("class LayoutGroup"))
    assert "public X3D" not in class_line  # only 'public virtual X3D...'


def test_mfnode_getters_return_const_ref(rendered):
    # MFNode getters hand out a reference to the member instead of copying the
    # vector (and bumping every child's refcount) on each read.
    raw, src = _read(rendered, "X3DGroupingNode.hpp")
    assert "const MFNode& getChildren() const" in src
    raw, src = _read(rendered, "Appearance.hpp")
    assert "const MFNode& getShaders() const" in src


def test_non_mfnode_getters_stay_by_value(rendered):
    # Only MFNode getters change shape; value-typed getters keep the uniform
    # by-value contract.
    raw, src = _read(rendered, "Transform.hpp")
    assert "SFVec3f getTranslation() const" in src
    raw, src = _read(rendered, "Coordinate.hpp")
    assert "MFVec3f getPoint() const" in src


def test_enums_header_generated(rendered):
    raw, src = _read(rendered, "X3Denums.hpp")
    assert "enum class AlphaModeChoices {" in src
    assert "enum class FogTypeChoices {" in src


def test_reflection_header_defines_range_diagnostic():
    from x3d_cpp_gen.emit.reflection import gen_reflection_header
    src = gen_reflection_header()
    assert "struct RangeDiagnostic" in src
    for field in ("nodeType", "defName", "fieldName", "detail"):
        assert field in src
    assert "std::string message() const" in src


def test_node_emits_checkranges_and_validateranges(rendered):
    # Material has several constrained fields (e.g. specularColor clamped [0,1]).
    # C1 decl/def split: the header DECLARES checkRanges*/validateRanges; the
    # out-of-line bodies live in Material.cpp.
    raw, src = _read(rendered, "Material.hpp")
    raw_c, src_c = _read(rendered, "Material.cpp")
    assert "checkRangesSpecularColor" in src
    assert "out.push_back(RangeDiagnostic{" in raw_c
    assert "void validateRanges(std::vector<RangeDiagnostic>& out) const override" in src

    # Fix 2: validateRanges passes nodeTypeName() as the concrete type argument.
    assert 'nodeTypeName(), "", out' in src_c
    # Fix 2: the static no longer hard-codes nodeType; nodeType is now a parameter.
    assert 'const std::string nodeType = "Material"' not in src_c
    # Fix 2: the static signature has the extra nodeType parameter.
    assert "const std::string& nodeType" in src

    # X3DNode must emit the virtual base no-op.
    raw_root, src_root = _read(rendered, "X3DNode.hpp")
    assert "virtual void validateRanges(std::vector<RangeDiagnostic>&" in src_root


def test_validateranges_covers_inherited_mixin_fields(rendered):
    # Fog inherits constrained fields from the X3DFogObject mixin (color [0,1],
    # visibilityRange numeric min). validateRanges must check those via the
    # base-qualified checker static + the inherited getter, even though they are
    # not Fog's own fields.
    raw, src = _read(rendered, "Fog.hpp")
    raw_c, src_c = _read(rendered, "Fog.cpp")
    # C1 decl/def split: the override is DECLARED in the header; its body
    # (the base-qualified checker calls) lives in Fog.cpp.
    assert "void validateRanges(std::vector<RangeDiagnostic>& out) const override" in src
    assert ("X3DFogObject::checkRangesColor" in src_c
            or "X3DFogObject::checkRangesVisibilityRange" in src_c)
    # Fix 1: Fog itself has no own constrained fields, so no bare protected: on Fog.
    # The protected: lives on X3DFogObject (where the statics are defined).
    # Fix 2: Fog's validateRanges passes nodeTypeName() so diagnostics say "Fog".
    assert 'nodeTypeName(), "", out' in src_c


def test_unconstrained_node_has_no_bare_protected(rendered):
    # Fix 1: nodes with no own constrained fields must not emit a bare protected:.
    raw, src = _read(rendered, "Box.hpp")
    assert "protected:" not in raw
    raw2, src2 = _read(rendered, "Group.hpp")
    assert "protected:" not in raw2
