"""Generated experimental SAI typed-binding contract."""

import copy
from importlib.resources import files

import pytest

from x3d_sai_gen.fieldtypes import FIELD_TYPE_MAPPING, XS_TYPES
from x3d_sai_gen.model.types import X3DType
from x3d_sai_gen.parser import (
    build_dependency_graph,
    parse_enum_definitions,
    parse_x3d_model,
)
from x3d_sai_gen.emit.sai_bindings import (
    SAI_VALUE_TYPE_MAPPING,
    gen_sai_binding_index,
    gen_sai_node_binding,
    sai_value_type,
)


SPEC = files("x3d_sai_gen").joinpath(
    "data", "X3dUnifiedObjectModel-4.0.xml"
)


@pytest.fixture(scope="module")
def model():
    nodes, skipped = parse_x3d_model(str(SPEC), FIELD_TYPE_MAPPING, XS_TYPES)
    assert not skipped
    return nodes, build_dependency_graph(nodes), parse_enum_definitions(str(SPEC))


def emit(name, model):
    nodes, graph, enum_defs = model
    return gen_sai_node_binding(nodes[name], nodes, graph)


def test_transform_binding_uses_owner_typed_keys(model):
    generated = emit("Transform", model)
    assert "struct Transform" in generated
    assert 'static constexpr std::string_view x3d_name = "Transform";' in generated
    assert "X3DSAIBindings.hpp" in generated
    assert "schema_fingerprint = model_fingerprint" in generated
    assert "field_key<Transform, ::x3d::sai::experimental::vec3f>" in generated
    assert "translation{" in generated
    assert 'translation{"translation",' in generated
    assert "access_type::input_output" in generated
    assert "field_keys" in generated
    assert "translation.name(), translation.kind, translation.access()" in generated


def test_binding_index_covers_every_generated_tag_in_order(model):
    nodes, _graph, _enum_defs = model
    generated = gen_sai_binding_index(nodes)
    assert generated.index('bindings/AcousticProperties.hpp') < generated.index(
        'bindings/WorldInfo.hpp'
    )
    assert "std::array<node_key_descriptor" in generated
    assert "Transform::field_keys" in generated
    assert "Transform::schema_fingerprint" in generated


def test_node_and_image_field_mappings_are_exact(model):
    group = emit("Group", model)
    texture = emit("PixelTexture", model)
    assert "field_key<Group, ::x3d::sai::experimental::node_list>" in group
    assert "children{" in group
    assert "field_key<PixelTexture, ::x3d::sai::experimental::image>" in texture
    assert "image{" in texture


def test_bounded_strings_use_normative_owning_values(model):
    fog = emit("X3DFogObject", model)
    font = emit("FontStyle", model)
    assert "field_key<X3DFogObject, std::string>" in fog
    assert "fogType{" in fog
    assert "field_key<FontStyle, ::x3d::sai::experimental::string_list>" in font
    assert "justify{" in font


def test_bounded_string_metadata_and_keys_preserve_declared_field_types(model):
    # A vocabulary restricts values; it does not introduce another ISO field
    # type. Check the parsed declaration independently of the legacy runtime
    # reflection tag, which uses synthetic SFEnum/MFEnum storage tags.
    from x3d_sai_gen.emit.semantic_metadata import gen_semantic_metadata_source

    nodes, graph, enum_defs = model
    source = gen_semantic_metadata_source(nodes, graph, "4.0")
    assert "X3DFieldType::SFEnum" not in source
    assert "X3DFieldType::MFEnum" not in source
    seen = set()
    for name, node in nodes.items():
        generated = emit(name, model)
        node_source = source.split(f'            "{name}",', 1)[1].split(
            "        SemanticNodeDescriptor{", 1
        )[0]
        for field in node.fields:
            if field.simple_type not in enum_defs:
                continue
            # Ignore encoding attributes excluded from the semantic catalog.
            wire = field.x3d_name or field.name
            if f'SemanticFieldDescriptor{{"{wire}",' not in node_source:
                continue
            assert field.type in {"SFString", "MFString", "xs_nmtoken"}
            normative_type = "MFString" if field.type == "MFString" else "SFString"
            assert (f'SemanticFieldDescriptor{{"{wire}", '
                    f'X3DFieldType::{normative_type},') in node_source
            cpp_type = ("::x3d::sai::experimental::string_list"
                        if field.type == "MFString" else "std::string")
            assert (f"field_key<{name}, {cpp_type}>\n"
                    f'      {field.name}{{"{wire}",') in generated
            seen.add(normative_type)
    assert seen == {"SFString", "MFString"}


def test_every_sf_mf_kind_has_one_mapping():
    expected = set(X3DType)
    assert set(SAI_VALUE_TYPE_MAPPING) == expected
    assert len(SAI_VALUE_TYPE_MAPPING) == len(expected)
    assert sai_value_type(X3DType.XS_NMTOKEN) == "std::string"
    with pytest.raises(ValueError, match="unsupported SAI field type"):
        sai_value_type(object())


def test_duplicate_sanitized_field_identifier_fails_closed(model):
    nodes, graph, enum_defs = model
    duplicate = copy.deepcopy(nodes["Transform"])
    translation = next(
        field for field in duplicate.fields if field.x3d_name == "translation"
    )
    duplicate.fields.append(copy.deepcopy(translation))
    with pytest.raises(ValueError, match="duplicate SAI field identifier"):
        gen_sai_node_binding(duplicate, nodes, graph)


def test_emission_is_byte_deterministic(model):
    assert emit("Transform", model) == emit("Transform", model)


def test_transform_fields_match_the_abstract_node_interface(model):
    # ISO/IEC 19775-1:2023, 10.4.4; independently listed rather than derived
    # from another emitter consuming the same UOM fields.
    from x3d_sai_gen.emit.semantic_fields import resolved_node_fields

    nodes, graph, enum_defs = model
    fields = resolved_node_fields(nodes["Transform"], nodes, graph, enum_defs)
    assert {field.x3d_name for field in fields} == {
        "addChildren", "removeChildren", "center", "children", "bboxDisplay",
        "metadata", "rotation", "scale", "scaleOrientation", "translation",
        "visible", "bboxCenter", "bboxSize",
    }


def test_encoding_attributes_are_not_sai_fields_on_any_node(model):
    from x3d_sai_gen.emit.semantic_fields import resolved_node_fields

    nodes, graph, enum_defs = model
    for node in nodes.values():
        fields = resolved_node_fields(node, nodes, graph, enum_defs)
        assert not {field.x3d_name for field in fields} & {
            "DEF", "USE", "IS", "class", "id",
        }, node.name
        assert ("style" in {field.x3d_name for field in fields}) == (
            node.name in {"FontStyle", "ScreenFontStyle"}
        ), node.name


def test_font_styles_keep_the_complete_normative_interface(model):
    # ISO/IEC 19775-1:2023, 15.4.1 and 36.4.4. Font style is a semantic
    # field even though X3DNode's HTML style attribute has the same name.
    from x3d_sai_gen.emit.semantic_fields import resolved_node_fields

    nodes, graph, enum_defs = model
    common = {"metadata", "family", "horizontal", "justify", "language",
              "leftToRight", "spacing", "style", "topToBottom"}
    for name, size in (("FontStyle", "size"), ("ScreenFontStyle", "pointSize")):
        fields = resolved_node_fields(nodes[name], nodes, graph, enum_defs)
        assert {field.x3d_name for field in fields} == common | {size}


@pytest.mark.parametrize("difference", ["content", "missing", "extra"])
def test_generated_check_detects_drift_without_rewriting(tmp_path, difference):
    from x3d_sai_gen.cli import check_generated

    expected = tmp_path / "expected"
    actual = tmp_path / "actual"
    expected.mkdir()
    actual.mkdir()
    (expected / "binding.hpp").write_text("expected")
    (actual / "binding.hpp").write_text("expected")
    assert check_generated(expected, actual) == 0
    if difference == "content":
        (actual / "binding.hpp").write_text("changed")
    elif difference == "missing":
        (actual / "binding.hpp").unlink()
    else:
        (actual / "stale.hpp").write_text("stale")
    before = {path.name: path.read_bytes() for path in actual.iterdir()}
    assert check_generated(expected, actual) == 1
    assert {path.name: path.read_bytes() for path in actual.iterdir()} == before


def test_semantic_metadata_preserves_uom_numeric_bounds(model):
    from x3d_sai_gen.emit.semantic_metadata import gen_semantic_metadata_source

    nodes, graph, enum_defs = model
    radius = next(f for f in nodes["Sphere"].fields if f.name == "radius")
    angle = next(f for f in nodes["Arc2D"].fields if f.name == "endAngle")
    assert radius.min_exclusive == "0"
    assert angle.min_exclusive == "-6.2832"
    assert angle.max_exclusive == "6.2832"
    source = gen_semantic_metadata_source(nodes, graph, "4.0")
    radius_line = next(line for line in source.splitlines()
                       if 'SemanticFieldDescriptor{"radius"' in line)
    assert radius_line.endswith('SemanticNumericRange{0.0, std::nullopt, false, true}},')


@pytest.mark.parametrize("attribute", [
    "min_inclusive", "max_inclusive", "min_exclusive", "max_exclusive",
])
def test_semantic_fingerprint_covers_each_numeric_bound(model, attribute):
    import re
    from x3d_sai_gen.emit.semantic_metadata import gen_semantic_metadata_source

    nodes, graph, enum_defs = model
    baseline = gen_semantic_metadata_source(nodes, graph, "4.0")
    changed = copy.deepcopy(nodes)
    translation = next(f for f in changed["Transform"].fields
                       if f.name == "translation")
    setattr(translation, attribute, "2")
    emitted = gen_semantic_metadata_source(changed, graph, "4.0")
    fingerprint = r'return "([a-f0-9]{64})";'
    assert re.search(fingerprint, baseline).group(1) != re.search(
        fingerprint, emitted).group(1)


@pytest.mark.parametrize("bounds", [
    {"min_inclusive": "0", "min_exclusive": "0"},
    {"max_inclusive": "1", "max_exclusive": "1"},
    {"min_inclusive": "NaN"},
    {"max_inclusive": "inf"},
])
def test_numeric_bound_emission_rejects_ambiguous_or_nonfinite_limits(model, bounds):
    from x3d_sai_gen.emit.semantic_metadata import gen_semantic_metadata_source

    nodes, graph, enum_defs = model
    changed = copy.deepcopy(nodes)
    translation = next(f for f in changed["Transform"].fields
                       if f.name == "translation")
    for key, value in bounds.items():
        setattr(translation, key, value)
    with pytest.raises(ValueError):
        gen_semantic_metadata_source(changed, graph, "4.0")
