"""SimpleType enumeration parsing + enum-typed field descriptors."""

from dataclasses import dataclass
from typing import List, Optional

from lxml import etree

from x3d_cpp_gen.model.enums import parse_enum_defs, _sanitize_enum_member
from x3d_cpp_gen.emit.descriptors import build_descriptor, build_descriptors
from x3d_cpp_gen.emit.defaults import enum_default_expr
from x3d_cpp_gen.generator import gen_enums_header


def _root(xml: str):
    return etree.fromstring(xml)


def test_parse_bounded_simpletype_becomes_enum():
    xml = """
    <SimpleTypes>
      <SimpleType name="alphaModeChoices" baseType="SFString"
                  appinfo="Permitted values. This list is bounded, no additional values are allowed.">
        <enumeration value="AUTO"/>
        <enumeration value="OPAQUE"/>
        <enumeration value="MASK"/>
        <enumeration value="BLEND"/>
      </SimpleType>
    </SimpleTypes>
    """
    defs = parse_enum_defs(_root(xml))
    assert "alphaModeChoices" in defs
    ed = defs["alphaModeChoices"]
    assert ed.cpp_name == "AlphaModeChoices"
    assert ed.base_type == "SFString"
    assert ed.is_multi is False
    assert [m.cpp_name for m in ed.members] == ["AUTO", "OPAQUE", "MASK", "BLEND"]


def test_open_vocabulary_is_not_an_enum():
    xml = """
    <SimpleTypes>
      <SimpleType name="lineTypeValues" baseType="SFInt32"
                  appinfo="linetype selects a hatch pattern, registered values may be extended.">
        <enumeration value="1"/>
        <enumeration value="2"/>
      </SimpleType>
    </SimpleTypes>
    """
    defs = parse_enum_defs(_root(xml))
    assert defs == {}


def test_sanitize_enum_members():
    assert _sanitize_enum_member('"BOUNCE"') == "BOUNCE"
    assert _sanitize_enum_member("UTF-8") == "UTF_8"
    assert _sanitize_enum_member("SPEED-1") == "SPEED_1"
    assert _sanitize_enum_member("3DFOO") == "_3DFOO"
    assert _sanitize_enum_member("") == "VALUE"


def test_duplicate_member_idents_decollide():
    xml = """
    <SimpleTypes>
      <SimpleType name="weirdChoices" baseType="SFString"
                  appinfo="This list is bounded.">
        <enumeration value="A-B"/>
        <enumeration value="A_B"/>
      </SimpleType>
    </SimpleTypes>
    """
    ed = parse_enum_defs(_root(xml))["weirdChoices"]
    names = [m.cpp_name for m in ed.members]
    assert len(names) == len(set(names))  # all unique


def test_mf_enum_is_multi():
    xml = """
    <SimpleTypes>
      <SimpleType name="justifyChoices" baseType="MFString"
                  appinfo="This list is bounded.">
        <enumeration value="BEGIN"/>
        <enumeration value="MIDDLE"/>
        <enumeration value="END"/>
      </SimpleType>
    </SimpleTypes>
    """
    ed = parse_enum_defs(_root(xml))["justifyChoices"]
    assert ed.is_multi is True


@dataclass
class FakeField:
    name: str
    type: str
    accessType: str = "inputOutput"
    default: Optional[str] = None
    description: Optional[str] = ""
    min_inclusive: Optional[str] = None
    max_inclusive: Optional[str] = None
    acceptable_node_types: Optional[List[str]] = None
    simple_type: Optional[str] = None


def _alpha_defs():
    xml = """
    <SimpleTypes>
      <SimpleType name="alphaModeChoices" baseType="SFString"
                  appinfo="This list is bounded.">
        <enumeration value="AUTO"/>
        <enumeration value="OPAQUE"/>
      </SimpleType>
      <SimpleType name="justifyChoices" baseType="MFString"
                  appinfo="This list is bounded.">
        <enumeration value="BEGIN"/>
        <enumeration value="END"/>
      </SimpleType>
    </SimpleTypes>
    """
    return parse_enum_defs(_root(xml))


def test_sf_enum_field_descriptor_typed_as_enum():
    defs = _alpha_defs()
    f = FakeField(name="alphaMode", type="SFString", default="AUTO",
                  simple_type="alphaModeChoices")
    d = build_descriptor(f, defs)
    assert d.is_enum is True
    assert d.enum_cpp_name == "AlphaModeChoices"
    assert d.cpp_type == "AlphaModeChoices"
    assert d.default_init_expr == "AlphaModeChoices::AUTO"
    assert d.is_readable and d.is_settable
    assert d.constraint_checks is None
    assert d.needs_move_overload is False  # small enum scalar, no move


def test_mf_enum_field_descriptor_is_vector():
    defs = _alpha_defs()
    f = FakeField(name="justify", type="MFString", default='"BEGIN" "END"',
                  simple_type="justifyChoices")
    d = build_descriptor(f, defs)
    assert d.is_enum is True
    assert d.cpp_type == "std::vector<JustifyChoices>"
    assert d.default_init_expr == "std::vector<JustifyChoices>{JustifyChoices::BEGIN, JustifyChoices::END}"
    assert d.needs_move_overload is True  # vector -> move overload


def test_open_vocab_field_stays_string():
    # simple_type names something not in defs -> ordinary std::string field.
    f = FakeField(name="hatchStyle", type="SFString", default="foo",
                  simple_type="lineTypeValues")
    d = build_descriptor(f, {})
    assert d.is_enum is False
    assert d.cpp_type == "SFString"


def test_enum_default_value_fallback_to_first_member():
    defs = _alpha_defs()
    ed = defs["alphaModeChoices"]
    # An unknown default token falls back to the first member.
    assert enum_default_expr(ed, "NONEXISTENT") == "AlphaModeChoices::AUTO"


def test_gen_enums_header_emits_enum_class():
    defs = _alpha_defs()
    hdr = gen_enums_header(defs)
    assert "enum class AlphaModeChoices {" in hdr
    assert "AUTO," in hdr
    assert "enum class JustifyChoices {" in hdr
    assert "#ifndef X3D_ENUMS_HPP" in hdr
