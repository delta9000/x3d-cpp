"""build_descriptor: per-field decisions computed in Python (the IR layer)."""

from dataclasses import dataclass
from typing import List, Optional

import pytest

from x3d_cpp_gen.emit.descriptors import build_descriptor, FieldDescriptor, _render_range_collect
from x3d_cpp_gen.model.types import X3DType


@dataclass
class FakeField:
    """Stand-in matching the attributes build_descriptor reads off X3DField."""

    name: str
    type: str
    accessType: str = "inputOutput"
    default: Optional[str] = None
    description: Optional[str] = ""
    min_inclusive: Optional[str] = None
    max_inclusive: Optional[str] = None
    acceptable_node_types: Optional[List[str]] = None


def desc(**kw) -> FieldDescriptor:
    return build_descriptor(FakeField(**kw))


def test_sfvec3f_with_default():
    d = desc(name="bboxCenter", type="SFVec3f", default="1 2 3")
    assert d.x3d_type is X3DType.SFVec3f
    assert d.cpp_type == "SFVec3f"             # X3D spelling, aliased in X3Dtypes
    assert d.cpp_ident == "_bboxCenter"
    assert d.getter_name == "getBboxCenter"
    assert d.setter_name == "setBboxCenter"
    assert d.default_init_expr == "SFVec3f{1, 2, 3}"
    assert d.value_init is False
    assert d.has_default is True
    assert d.is_readable and d.is_settable and d.has_member
    assert d.needs_move_overload is True       # struct type -> move overload
    assert d.constraint_checks is None


def test_input_only_event_field():
    d = desc(name="set_fraction", type="SFFloat", accessType="inputOnly")
    assert d.is_event is True
    assert d.is_readable is False              # no getter
    assert d.is_settable is False             # no setter
    assert d.has_member is False              # no persisted member
    assert d.handler_name == "onSet_fraction"
    assert d.needs_move_overload is False     # not settable


def test_output_only_readonly_field():
    d = desc(name="isActive", type="SFBool", accessType="outputOnly")
    assert d.is_readonly is True
    assert d.is_readable is True              # has a getter
    assert d.is_settable is False            # but no setter
    assert d.is_event is False
    assert d.has_member is True


def test_min_max_constrained_numeric():
    d = desc(name="intensity", type="SFFloat", min_inclusive="0", max_inclusive="1")
    assert d.has_constraints is True
    assert d.validator_name == "validateIntensity"
    body = d.constraint_checks
    assert 'if (value < 0) throw std::out_of_range("intensity below minimum of 0");' in body
    assert 'if (value > 1) throw std::out_of_range("intensity above maximum of 1");' in body
    assert "for (const auto& v : value)" not in body


def test_sfcolor_is_distinct_and_clamped():
    d = desc(name="diffuseColor", type="SFColor", default="0.8 0.8 0.8")
    assert d.x3d_type is X3DType.SFColor
    assert d.cpp_type == "SFColor"            # distinct from SFVec3f
    # Colors always clamp to [0,1] even without a declared min/max.
    assert d.has_constraints is True
    body = d.constraint_checks
    assert 'if (value.r < 0) throw std::out_of_range("diffuseColor.r below minimum of 0");' in body
    assert 'if (value.b > 1) throw std::out_of_range("diffuseColor.b above maximum of 1");' in body
    assert d.default_init_expr == "SFColor{0.8, 0.8, 0.8}"


def test_mf_constraint_uses_element_loop():
    d = desc(name="color", type="MFColor")
    assert d.has_constraints is True
    body = d.constraint_checks
    assert body.startswith("for (const auto& v : value) {")
    assert 'if (v.r < 0) throw std::out_of_range("color.r below minimum of 0");' in body


def test_initialize_only_has_getter_no_setter():
    d = desc(name="solid", type="SFBool", accessType="initializeOnly", default="true")
    assert d.is_readable is True
    assert d.is_settable is False
    assert d.default_init_expr == "true"


def test_unconstrained_struct_has_no_checks():
    d = desc(name="translation", type="SFVec3f")
    assert d.constraint_checks is None
    assert d.has_constraints is False


def test_initialize_only_has_data_setter_routed_to_unchecked():
    d = desc(name="radius", type="SFFloat", accessType="initializeOnly", default="1")
    # No public typed setter (unchanged contract):
    assert d.is_settable is False
    # But writable at the data layer, via the unchecked setter:
    assert d.has_data_setter is True
    assert d.setter_unchecked_name == "setRadiusUnchecked"
    assert d.reader_setter_call == "setRadiusUnchecked"


def test_input_output_reader_setter_call_unchanged():
    # Unconstrained inputOutput still routes to the plain setter.
    d = desc(name="point", type="MFVec3f", accessType="inputOutput")
    assert d.has_data_setter is True
    assert d.reader_setter_call == "setPoint"
    # Constrained inputOutput still routes to the unchecked setter.
    c = desc(name="intensity", type="SFFloat", accessType="inputOutput",
             min_inclusive="0", max_inclusive="1")
    assert c.reader_setter_call == "setIntensityUnchecked"


def test_event_fields_have_no_data_setter():
    assert desc(name="set_fraction", type="SFFloat", accessType="inputOnly").has_data_setter is False
    assert desc(name="isActive", type="SFBool", accessType="outputOnly").has_data_setter is False


def test_render_range_collect_scalar_and_color():
    body = _render_range_collect("beamWidth", X3DType.SFFloat, None, "1.5707")
    assert "out.push_back(RangeDiagnostic{" in body
    assert "beamWidth above maximum of 1.5707" in body
    assert "throw" not in body
    cbody = _render_range_collect("specularColor", X3DType.SFColor, "0", "1")
    assert "specularColor.r above maximum of 1" in cbody
    assert "specularColor.r below minimum of 0" in cbody
    assert "throw" not in cbody
    mfbody = _render_range_collect("color", X3DType.MFColor, "0", "1")
    assert mfbody.startswith("for (const auto& v : value) {")
    assert "out.push_back(RangeDiagnostic{" in mfbody


def test_initializeonly_constrained_field_gets_range_diagnostics_not_throwing_validation():
    from x3d_cpp_gen.parser import X3DField
    from x3d_cpp_gen.emit.descriptors import build_descriptor

    field = X3DField(
        name="order", type="SFInt32", accessType="initializeOnly",
        x3d_name="order", default="4", min_inclusive="0", max_inclusive="5",
    )
    d = build_descriptor(field)

    # initializeOnly must NOT get the throwing validate<Name>() path -- it has
    # no public typed setter to protect (data-layer writes always go through
    # set<Name>Unchecked by design).
    assert d.constraint_checks is None
    assert not d.has_constraints

    # But it MUST get the non-throwing diagnostic-collection path, so an
    # out-of-range authored value is at least surfaced via validateRanges()/
    # collectRangeWarnings() instead of vanishing silently.
    assert d.range_collect_body is not None
    assert d.has_range_diagnostics


def test_inputoutput_constrained_field_still_gets_both_paths():
    from x3d_cpp_gen.parser import X3DField
    from x3d_cpp_gen.emit.descriptors import build_descriptor

    field = X3DField(
        name="transparency", type="SFFloat", accessType="inputOutput",
        x3d_name="transparency", default="0", min_inclusive="0", max_inclusive="1",
    )
    d = build_descriptor(field)

    assert d.constraint_checks is not None
    assert d.has_constraints
    assert d.range_collect_body is not None
    assert d.has_range_diagnostics


def test_unconstrained_field_gets_neither():
    from x3d_cpp_gen.parser import X3DField
    from x3d_cpp_gen.emit.descriptors import build_descriptor

    field = X3DField(
        name="name", type="SFString", accessType="inputOutput",
        x3d_name="name", default="",
    )
    d = build_descriptor(field)

    assert d.constraint_checks is None
    assert not d.has_constraints
    assert d.range_collect_body is None
    assert not d.has_range_diagnostics


def test_build_reflection_descriptors_drops_phantom_fields():
    from x3d_cpp_gen.parser import X3DField, X3DNode
    from x3d_cpp_gen.emit.descriptors import build_reflection_descriptors

    # "ghost" claims inheritedFrom="NeverDeclaresIt" but no node in the
    # hierarchy actually owns a field named "ghost" -- this is the UOM
    # phantom-field pattern (9 such fields exist in the real 4.0 spec).
    child = X3DNode(
        name="Child", base_type="Parent",
        fields=[
            X3DField(name="ghost", type="SFBool", accessType="inputOutput",
                     x3d_name="ghost", inherited_from="NeverDeclaresIt"),
            X3DField(name="real", type="SFBool", accessType="inputOutput",
                     x3d_name="real"),
        ],
    )
    descriptors = build_reflection_descriptors(
        child,
        own_field_names={"Child": {"real"}, "Parent": set()},
        ancestors=["Parent"],
    )

    names = {d.x3d_name for d in descriptors}
    assert "real" in names
    assert "ghost" not in names, "phantom field (declared nowhere) must be dropped"


def test_build_reflection_descriptors_qualifies_inherited_fields_by_true_declarer():
    from x3d_cpp_gen.parser import X3DField, X3DNode
    from x3d_cpp_gen.emit.descriptors import build_reflection_descriptors

    # "color" is claimed inheritedFrom="WrongClass" in the UOM data, but is
    # actually declared by "RealBase" per own_field_names -- the resolver
    # must trust own_field_names (derived from the actual C++ hierarchy),
    # not the UOM's raw inheritedFrom attribute.
    child = X3DNode(
        name="Child", base_type="RealBase",
        fields=[
            X3DField(name="color", type="SFColor", accessType="inputOutput",
                     x3d_name="color", inherited_from="WrongClass"),
        ],
    )
    descriptors = build_reflection_descriptors(
        child,
        own_field_names={"Child": set(), "RealBase": {"color"}},
        ancestors=["RealBase"],
    )

    assert len(descriptors) == 1
    assert descriptors[0].inherited_from == "RealBase"


def test_build_reflection_descriptors_leaves_own_fields_unqualified():
    from x3d_cpp_gen.parser import X3DField, X3DNode
    from x3d_cpp_gen.emit.descriptors import build_reflection_descriptors

    node = X3DNode(
        name="Leaf", fields=[
            X3DField(name="size", type="SFVec3f", accessType="inputOutput",
                     x3d_name="size"),
        ],
    )
    descriptors = build_reflection_descriptors(
        node,
        own_field_names={"Leaf": {"size"}},
        ancestors=[],
    )
    assert descriptors[0].inherited_from is None
