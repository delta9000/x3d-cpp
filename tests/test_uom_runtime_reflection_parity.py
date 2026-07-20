"""Runtime reflection parity: shipped C++ FieldInfo tables vs. a fresh UOM parse.

Historically this equivalence was checked at RUNTIME in the (now-split-out)
SAI kernel's test suite: it called `X3DNodeFactory::create(name)->fields()`
and compared the live-reflected FieldTable against a generated
`X3DSemanticMetadataRegistry` catalog, field-by-field (x3dName / type /
access, in declaration order). That registry has moved to the sister repo
and is not coming back to x3d-cpp.

This test re-establishes the check from what x3d-cpp still owns:

  - It parses the UOM XML directly with `x3d_cpp_gen.parser.parse_x3d_model`
    (the raw XML -> dataclass step the generator pipeline starts from) and
    goes NO further downstream: it does not import or call
    `emit.descriptors.build_reflection_descriptors` (or anything else under
    `emit/`), which is the code that actually decides field order/typing for
    the generated header. It maps X3D wire type/access spellings to the C++
    runtime enumerator spellings with its own inline dicts here, not by
    reusing `FieldDescriptor.runtime_field_type` / `.runtime_access`.
  - It extracts the actual FieldInfo entries straight out of the CHECKED-IN
    `generated_cpp_bindings/x3d/nodes/<Node>.cpp` text via regex (reading
    committed source, not re-invoking the generator to produce a fresh copy
    to diff against itself).

Caveat vs. the removed block: this compares the UOM against the generated
C++ *source text*, not against a live `X3DNodeFactory::create(name)->fields()`
call, so it cannot catch a divergence introduced between that source and
what actually gets compiled/linked (e.g. a hand-edit to a .hpp that the
.cpp's table doesn't reflect, or an ODR issue). It DOES independently catch
UOM/codegen drift: a field silently dropped, reordered, retyped, or given
the wrong access category relative to the spec.
"""

import re
from importlib.resources import files
from pathlib import Path

import pytest

from x3d_cpp_gen.parser import parse_x3d_model, get_own_fields, parse_enum_definitions
from x3d_cpp_gen.generator import FIELD_TYPE_MAPPING, XS_TYPES

SPEC = files("x3d_cpp_gen").joinpath("data", "X3dUnifiedObjectModel-4.0.xml")
REPO_ROOT = Path(__file__).resolve().parent.parent
NODES_DIR = REPO_ROOT / "generated_cpp_bindings" / "x3d" / "nodes"

# UOM accessType spelling -> the AccessType C++ enumerator in X3DReflection.hpp.
# Deliberately NOT imported from emit.descriptors.FieldDescriptor.runtime_access,
# so this test doesn't share logic with the code path it is checking.
_ACCESS_MAP = {
    "inputOutput": "InputOutput",
    "initializeOnly": "InitializeOnly",
    "outputOnly": "OutputOnly",
    "inputOnly": "InputOnly",
}

# Matches `FieldInfo{"name", X3DFieldType::Type, AccessType::Access` across the
# clang-format line-wrap variations actually present in the committed sources
# (single-line and multi-line pushes both start this way).
_FIELD_INFO_RE = re.compile(
    r'FieldInfo\{\s*"([^"]*)",\s*X3DFieldType::(\w+),\s*AccessType::(\w+)'
)


@pytest.fixture(scope="module")
def uom_nodes():
    nodes, _skipped = parse_x3d_model(str(SPEC), FIELD_TYPE_MAPPING, XS_TYPES)
    assert nodes, "UOM parse produced no nodes"
    return nodes


@pytest.fixture(scope="module")
def enum_defs():
    return parse_enum_definitions(str(SPEC))


def _expected_runtime_type(field, enum_defs) -> str:
    """The X3DFieldType enumerator this UOM field should reflect as.

    `field.type` (from parse_x3d_model) is already the resolved wire-type
    spelling, which for ordinary fields IS the X3DFieldType enumerator name
    (FIELD_TYPE_MAPPING's keys are spelled identically to the enumerators).
    Two exceptions:

    - A field whose raw XML type is "xs:NMTOKEN" is resolved by the parser
      (via XS_TYPES) to the C++ typedef name "xs_nmtoken", not an
      X3DFieldType spelling. XS_TYPES declares its underlying storage as
      plain std::string (`"xs:NMTOKEN": ("xs_nmtoken", "std::string")`), so
      at runtime it reflects as an ordinary SFString (there is no
      "SFNmtoken" enumerator in X3DFieldType).
    - A field whose simpleType names a BOUNDED enumeration (per
      parse_enum_definitions) reflects as the opaque SFEnum/MFEnum tag
      instead of its raw SFString/MFString scalar type.
    """
    if field.type == "xs_nmtoken":
        return "SFString"
    if field.simple_type and field.simple_type in enum_defs:
        return "MFEnum" if field.type.startswith("MF") else "SFEnum"
    return field.type


def _extract_generated_fields(node_name: str):
    """(x3dName, type, access) triples straight out of the committed
    <Node>.cpp text, in file order. None if there is no concrete fields()
    table for this node (abstract nodes have no .cpp)."""
    path = NODES_DIR / f"{node_name}.cpp"
    if not path.exists():
        return None
    text = path.read_text()
    if "::fields() const" not in text:
        return None
    return _FIELD_INFO_RE.findall(text)


def _node_names_with_bindings():
    return sorted(p.stem for p in NODES_DIR.glob("*.cpp"))


@pytest.mark.parametrize("node_name", _node_names_with_bindings())
def test_generated_field_table_matches_uom(node_name, uom_nodes, enum_defs):
    if node_name not in uom_nodes:
        pytest.skip(f"{node_name} has a .cpp but no UOM entry")
    uom_node = uom_nodes[node_name]

    generated = _extract_generated_fields(node_name)
    if generated is None:
        pytest.skip(f"{node_name}.cpp has no fields() reflection table")
    generated_names = [g[0] for g in generated]

    # 1. Coverage: every field this node declares itself (not inherited) must
    #    appear in the generated table. "Own" fields need no inheritance
    #    resolution to trust -- inheritedFrom is unset for them by definition
    #    -- so this direction is a clean, unambiguous check.
    for f in get_own_fields(uom_node):
        assert f.x3d_name in generated_names, (
            f"{node_name}: UOM-declared own field '{f.x3d_name}' is missing "
            f"from the generated reflection table"
        )

    # 2. Order: the UOM's full flattened field list (own + inherited, UOM
    #    declaration order), filtered down to just the names that made it
    #    into the generated table, must reproduce the generated order
    #    exactly. The generator only ever drops fields (undeclared "phantom"
    #    inherited references); it never reorders or duplicates them.
    uom_by_name = {}
    for f in uom_node.fields:
        uom_by_name.setdefault(f.x3d_name, f)
    uom_order_filtered = [n for n in uom_by_name if n in generated_names]
    assert uom_order_filtered == generated_names, (
        f"{node_name}: generated field order diverges from UOM declaration "
        f"order.\nUOM (filtered): {uom_order_filtered}\n"
        f"generated:      {generated_names}"
    )

    # 3. Per-field type/access agreement.
    for x3d_name, gen_type, gen_access in generated:
        assert x3d_name in uom_by_name, (
            f"{node_name}: generated field '{x3d_name}' has no UOM source"
        )
        f = uom_by_name[x3d_name]
        expected_type = _expected_runtime_type(f, enum_defs)
        assert gen_type == expected_type, (
            f"{node_name}.{x3d_name}: generated type {gen_type} != "
            f"UOM-derived type {expected_type}"
        )
        expected_access = _ACCESS_MAP[f.accessType]
        assert gen_access == expected_access, (
            f"{node_name}.{x3d_name}: generated access {gen_access} != "
            f"UOM-derived access {expected_access} "
            f"(UOM accessType={f.accessType!r})"
        )


def test_uom_and_bindings_both_nonempty(uom_nodes):
    # Guards against both halves of this test silently comparing nothing
    # (e.g. a bad UOM path or an empty generated_cpp_bindings/x3d/nodes dir
    # would otherwise make every parametrized case vacuously pass/skip).
    assert len(uom_nodes) > 300
    assert len(_node_names_with_bindings()) > 300
