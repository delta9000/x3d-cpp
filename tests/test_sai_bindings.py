"""Generated experimental SAI typed-binding contract."""

import copy
from importlib.resources import files

import pytest

from x3d_cpp_gen.generator import FIELD_TYPE_MAPPING, XS_TYPES
from x3d_cpp_gen.model.types import X3DType
from x3d_cpp_gen.parser import (
    build_dependency_graph,
    parse_enum_definitions,
    parse_x3d_model,
)
from x3d_cpp_gen.emit.sai_bindings import (
    SAI_VALUE_TYPE_MAPPING,
    gen_sai_binding_index,
    gen_sai_node_binding,
    sai_value_type,
)


SPEC = files("x3d_cpp_gen").joinpath(
    "data", "X3dUnifiedObjectModel-4.0.xml"
)


@pytest.fixture(scope="module")
def model():
    nodes, skipped = parse_x3d_model(str(SPEC), FIELD_TYPE_MAPPING, XS_TYPES)
    assert not skipped
    return nodes, build_dependency_graph(nodes), parse_enum_definitions(str(SPEC))


def emit(name, model):
    nodes, graph, enum_defs = model
    return gen_sai_node_binding(nodes[name], nodes, graph, enum_defs)


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


def test_node_image_and_reserved_field_mappings_are_exact(model):
    group = emit("Group", model)
    texture = emit("PixelTexture", model)
    assert "field_key<Group, ::x3d::sai::experimental::node_list>" in group
    assert "children{" in group
    assert "field_key<PixelTexture, ::x3d::sai::experimental::image>" in texture
    assert "image{" in texture
    assert "field_key<PixelTexture, std::string>" in texture
    assert 'class_{"class",' in texture


def test_bounded_enums_use_experimental_owning_values(model):
    fog = emit("X3DFogObject", model)
    font = emit("FontStyle", model)
    assert "field_key<X3DFogObject, ::x3d::sai::experimental::enum_value>" in fog
    assert "fogType{" in fog
    assert "field_key<FontStyle, ::x3d::sai::experimental::enum_list>" in font
    assert "justify{" in font


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
        gen_sai_node_binding(duplicate, nodes, graph, enum_defs)


def test_emission_is_byte_deterministic(model):
    assert emit("Transform", model) == emit("Transform", model)
