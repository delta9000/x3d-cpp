"""Semantic oracles independent of emitter/adapter parity.

The pinned UOM also describes XML/DOM syntax. These expectations deliberately
name the SAI surface rather than deriving it from the projection under test.
"""

from importlib.resources import files

import pytest

from x3d_sai_gen.emit.sai_bindings import gen_sai_node_binding
from x3d_sai_gen.emit.semantic_fields import resolved_node_fields
from x3d_sai_gen.emit.semantic_metadata import gen_semantic_metadata_source
from x3d_sai_gen.fieldtypes import FIELD_TYPE_MAPPING, XS_TYPES
from x3d_sai_gen.parser import (
    build_dependency_graph,
    parse_enum_definitions,
    parse_x3d_model,
)


@pytest.fixture(scope="module")
def model():
    spec = files("x3d_sai_gen").joinpath("data", "X3dUnifiedObjectModel-4.0.xml")
    nodes, skipped = parse_x3d_model(str(spec), FIELD_TYPE_MAPPING, XS_TYPES)
    assert not skipped
    return nodes, build_dependency_graph(nodes), parse_enum_definitions(str(spec))


def test_transform_runtime_fields_match_semantic_contract(model):
    nodes, graph, enums = model
    actual = resolved_node_fields(nodes["Transform"], nodes, graph, enums)
    assert [field.x3d_name for field in actual] == [
        "addChildren", "bboxCenter", "bboxDisplay", "bboxSize", "center",
        "children", "metadata", "removeChildren", "rotation", "scale",
        "scaleOrientation", "translation", "visible",
    ]


def test_encoding_bindings_and_host_attributes_are_not_runtime_keys(model):
    nodes, graph, enums = model
    for node in nodes.values():
        fields = resolved_node_fields(node, nodes, graph, enums)
        names = {field.x3d_name for field in fields}
        assert names.isdisjoint({"DEF", "USE", "IS", "class", "id"}), node.name
        if node.name not in {"FontStyle", "ScreenFontStyle"}:
            assert "style" not in names, node.name
        generated = gen_sai_node_binding(node, nodes, graph, enums)
        for name in ("DEF", "USE", "IS", "class", "id"):
            assert f'"{name}"' not in generated, node.name


def test_semantic_catalog_excludes_document_attributes(model):
    nodes, graph, enums = model
    generated = gen_semantic_metadata_source(nodes, graph, enums, "4.0")
    for name in ("DEF", "USE", "IS", "class", "id"):
        assert f'SemanticFieldDescriptor{{"{name}",' not in generated
    assert generated.count('SemanticFieldDescriptor{"style",') == 2


@pytest.mark.parametrize("name", ["FontStyle", "ScreenFontStyle"])
def test_text_style_is_preserved_as_a_real_field(model, name):
    nodes, graph, enums = model
    fields = resolved_node_fields(nodes[name], nodes, graph, enums)
    style = next(field for field in fields if field.x3d_name == "style")
    assert style.runtime_field_type == "SFEnum"
    assert style.access_type == "inputOutput"
    generated = gen_sai_node_binding(nodes[name], nodes, graph, enums)
    assert 'style{"style",' in generated


def test_raw_uom_retains_encoding_and_host_facts(model):
    nodes, _, _ = model
    raw = {field.x3d_name for field in nodes["Transform"].fields}
    assert {"DEF", "USE", "IS", "class", "id", "style"} <= raw
