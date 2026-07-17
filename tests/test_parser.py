"""Parser invariants: real spec parses, no None field leakage, unsupported types skipped."""

from importlib.resources import files

import pytest

from x3d_cpp_gen.parser import (
    X3DField,
    parse_x3d_model,
    parse_node,
    _parse_fields,
    sanitize_field_name,
)
from x3d_cpp_gen.generator import FIELD_TYPE_MAPPING, XS_TYPES


SPEC = files("x3d_cpp_gen").joinpath("data", "X3dUnifiedObjectModel-4.0.xml")


@pytest.fixture(scope="module")
def nodes():
    nodes, skipped = parse_x3d_model(str(SPEC), FIELD_TYPE_MAPPING, XS_TYPES)
    assert skipped == [], f"real spec unexpectedly skipped fields: {skipped}"
    return nodes


def test_spec_parses_nonempty(nodes):
    assert nodes, "expected the packaged UOM spec to parse into nodes"
    assert "Box" in nodes


def test_no_none_leakage_anywhere(nodes):
    """Every field in every parsed node must be a real X3DField, never None."""
    for name, node in nodes.items():
        assert node.fields is not None
        for f in node.fields:
            assert f is not None, f"None field leaked into node {name!r}"
            assert isinstance(f, X3DField), f"non-X3DField in node {name!r}: {f!r}"
            assert f.name is not None
            assert f.type is not None
            # _parse_fields keeps the raw X3D type name for FIELD_TYPE_MAPPING
            # types (e.g. 'SFBool') and remaps xs:* types to their cpp name.
            assert (
                f.type in FIELD_TYPE_MAPPING
                or f.type in {v[0] for v in XS_TYPES.values()}
            ), f"unexpected unmapped type {f.type!r} on {name}.{f.name}"


def test_box_has_expected_own_fields(nodes):
    box = nodes["Box"]
    names = {f.name for f in box.fields}
    assert "size" in names
    assert "solid" in names


def _make_node_element(xml_fragment: str):
    from lxml import etree

    return etree.fromstring(xml_fragment)


def test_unsupported_type_is_skipped_not_noned(capsys):
    """An unsupported field type is dropped (with a warning), never yielded as None."""
    xml = (
        "<ConcreteNode name='Frob'>"
        "  <field name='good' type='SFBool' accessType='inputOutput'/>"
        "  <field name='bad' type='SFTotallyMadeUp' accessType='inputOutput'/>"
        "</ConcreteNode>"
    )
    el = _make_node_element(xml)
    parsed, skipped = _parse_fields(el, FIELD_TYPE_MAPPING, XS_TYPES, "Frob")

    assert all(f is not None for f in parsed)
    assert [f.name for f in parsed] == ["good"]
    assert all(isinstance(f, X3DField) for f in parsed)
    assert skipped == [("Frob", "bad", "SFTotallyMadeUp")]

    captured = capsys.readouterr()
    assert "skipping field 'bad'" in captured.out
    assert "SFTotallyMadeUp" in captured.out


def test_parse_fields_returns_skipped_unsupported_types(capsys):
    xml = (
        "<AbstractNodeType name='TestNode'>"
        "  <field name='ok' type='SFBool' accessType='inputOutput'/>"
        "  <field name='bad' type='SFTotallyMadeUpType' accessType='inputOutput'/>"
        "</AbstractNodeType>"
    )
    el = _make_node_element(xml)
    field_type_mapping = {"SFBool": "bool"}
    xs_types = {}

    fields, skipped = _parse_fields(el, field_type_mapping, xs_types, "TestNode")

    assert len(fields) == 1
    assert fields[0].name == "ok"
    assert skipped == [("TestNode", "bad", "SFTotallyMadeUpType")]


def test_xs_nmtoken_type_is_mapped(capsys):
    """xs:NMTOKEN is a supported XS type and should be parsed (mapped to its cpp name)."""
    xml = (
        "<ConcreteNode name='Frob'>"
        "  <field name='tok' type='xs:NMTOKEN' accessType='inputOutput'/>"
        "</ConcreteNode>"
    )
    el = _make_node_element(xml)
    parsed, skipped = _parse_fields(el, FIELD_TYPE_MAPPING, XS_TYPES, "Frob")
    assert len(parsed) == 1
    assert parsed[0].type == XS_TYPES["xs:NMTOKEN"][0]
    assert skipped == []


def test_missing_spec_returns_empty_dict(tmp_path):
    nodes, skipped = parse_x3d_model(str(tmp_path / "nope.xml"), FIELD_TYPE_MAPPING, XS_TYPES)
    assert nodes == {}
    assert skipped == []


# The three node categories now share one parse_node() path.

def test_parse_node_mixin_has_no_base():
    xml = (
        "<AbstractObjectType name='X3DMixin'>"
        "  <InterfaceDefinition appinfo='a mixin' specificationUrl='u'>"
        "    <field name='metadata' type='SFNode' accessType='inputOutput'/>"
        "  </InterfaceDefinition>"
        "</AbstractObjectType>"
    )
    node, skipped = parse_node(_make_node_element(xml), FIELD_TYPE_MAPPING, XS_TYPES,
                      is_abstract=True, is_mixin=True)
    assert node.base_type is None
    assert node.is_abstract is True
    assert node.class_description == "a mixin"
    assert node.specification_url == "u"
    assert [f.name for f in node.fields] == ["metadata"]
    assert skipped == []


def test_parse_node_concrete_reads_inheritance_and_container():
    xml = (
        "<ConcreteNode name='Frob'>"
        "  <InterfaceDefinition appinfo='desc'>"
        "    <field name='size' type='SFVec3f' accessType='inputOutput'/>"
        "  </InterfaceDefinition>"
        "  <Inheritance baseType='X3DBase'/>"
        "  <AdditionalInheritance baseType='X3DBoundedObject'/>"
        "  <containerField default='children' type='SFNode'/>"
        "</ConcreteNode>"
    )
    node, skipped = parse_node(_make_node_element(xml), FIELD_TYPE_MAPPING, XS_TYPES,
                      is_abstract=False)
    assert node.base_type == "X3DBase"
    assert node.additional_base_types == ["X3DBoundedObject"]
    assert node.is_abstract is False
    assert node.container_field is not None
    assert node.container_field.default == "children"
    assert [f.name for f in node.fields] == ["size"]
    assert skipped == []


def test_parse_node_reads_component_info():
    xml = (
        "<ConcreteNode name='Frob'>"
        "  <InterfaceDefinition appinfo='desc'>"
        "    <componentInfo name='Shape' level='2'/>"
        "    <field name='size' type='SFVec3f' accessType='inputOutput'/>"
        "  </InterfaceDefinition>"
        "  <Inheritance baseType='X3DBase'/>"
        "</ConcreteNode>"
    )
    node, skipped = parse_node(_make_node_element(xml), FIELD_TYPE_MAPPING, XS_TYPES,
                      is_abstract=False)
    assert node.component is not None
    assert node.component.name == "Shape"
    assert node.component.level == 2
    assert skipped == []


def test_parse_node_captures_simple_type_and_acceptable_node_types():
    xml = (
        "<ConcreteNode name='Frob'>"
        "  <InterfaceDefinition appinfo='desc'>"
        "    <field name='alphaMode' type='SFString' accessType='inputOutput'"
        "           simpleType='alphaModeChoices' default='AUTO'/>"
        "    <field name='material' type='SFNode' accessType='inputOutput'"
        "           acceptableNodeTypes='X3DMaterialNode|TwoSidedMaterial'/>"
        "  </InterfaceDefinition>"
        "</ConcreteNode>"
    )
    node, skipped = parse_node(_make_node_element(xml), FIELD_TYPE_MAPPING, XS_TYPES,
                      is_abstract=False)
    by_name = {f.name: f for f in node.fields}
    assert by_name["alphaMode"].simple_type == "alphaModeChoices"
    assert by_name["material"].acceptable_node_types == [
        "X3DMaterialNode", "TwoSidedMaterial"]
    assert skipped == []


def test_real_spec_material_metadata(nodes):
    """Material (real spec) carries containerField 'material' + Shape component."""
    mat = nodes["Material"]
    assert mat.container_field is not None
    assert mat.container_field.default == "material"
    assert mat.component is not None
    assert mat.component.name == "Shape"
    assert mat.component.level == 1


def test_every_concrete_node_has_container_field(nodes):
    """Every concrete (instantiable) node parses an explicit containerField."""
    for name, node in nodes.items():
        if node.is_abstract:
            continue
        assert node.container_field is not None, (
            f"concrete node {name!r} missing containerField")
        assert node.container_field.default, (
            f"concrete node {name!r} has empty containerField default")
