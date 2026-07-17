"""Default-literal generation, keyed on :class:`X3DType`.

Replaces the giant ``{% if field.type == 'SFVec3f' %} ...`` Jinja default ladder.
The single :func:`default_expr_for` entry point computes the C++ initializer
expression for a field's spec default (or ``None`` when there is none, in which
case the member value-initializes with ``{}``).
"""

import math
from typing import List, Optional

from x3d_cpp_gen.model.types import X3DType, TypeRegistry, resolve_x3d_type
from x3d_cpp_gen.emit.naming import cpp_str as cpp_string_literal  # noqa: F401


def tokenize_mfstring(default: str) -> List[str]:
    """Tokenize an MFString default into its component strings.

    X3D MFString defaults quote each element, e.g. '"GD" "WE"'. Quoted
    multi-word elements (e.g. '"CENTER" "CENTER"') must each become one element.
    Unquoted defaults fall back to whitespace splitting.
    """
    if not default:
        return []
    tokens = []
    i = 0
    n = len(default)
    saw_quote = False
    while i < n:
        ch = default[i]
        if ch.isspace():
            i += 1
            continue
        if ch == '"':
            saw_quote = True
            i += 1
            buf = []
            while i < n and default[i] != '"':
                if default[i] == '\\' and i + 1 < n:
                    buf.append(default[i])
                    buf.append(default[i + 1])
                    i += 2
                    continue
                buf.append(default[i])
                i += 1
            i += 1  # skip closing quote
            tokens.append(''.join(buf))
        else:
            buf = []
            while i < n and not default[i].isspace():
                buf.append(default[i])
                i += 1
            tokens.append(''.join(buf))
    if not saw_quote and len(tokens) == 1 and ' ' in default:
        # purely unquoted multi-word fallback
        return default.split()
    return tokens


def _scalar_list(default: str):
    return [tok for tok in default.replace(",", " ").split() if tok]


# Struct SF* types -> (struct name, component count, is-float, row size).
# Drives the brace initializer for fixed-arity vector/colour/rotation/matrix
# defaults. The plain vector/colour/rotation structs are flat (e.g. ``float
# x, y, z;``), so row_size=0 and _struct_literal emits a single flat brace
# list: ``SFVec3f{x, y, z}``.
#
# For matrix types, row_size (derived via math.isqrt(count)) is nonzero: their
# struct wraps a genuine 2D array member (e.g. ``float matrix[4][4]``). This
# is why Clang's -Wmissing-braces (promoted to -Werror) rejects the flat
# elided-brace form even though GCC accepts it -- empirically verified that
# Clang requires each row of the 2D array individually braced. row_size is
# consumed by _struct_literal via _chunk_braced to chunk the flat value list
# into row_size-sized rows, each explicitly braced, wrapped in one more brace
# for the array member: ``Struct{ {row0}, {row1}, ... }``.
#
# row_size is DERIVED (math.isqrt(count)) rather than hand-typed to prevent
# the class of bug that prompted this table's creation: a stale/wrong value
# that silently disagrees with count. All supported matrix types are square,
# so isqrt is exact for them; the assert in _matrix_row_size below fails
# loudly if a future non-square matrix type is added (it would need a
# different chunking scheme entirely).
def _matrix_row_size(count: int) -> int:
    row_size = math.isqrt(count)
    assert row_size * row_size == count, (
        f"_STRUCT_ARITY matrix entry with count={count} is not a perfect "
        f"square -- row-chunking assumes a square matrix; a non-square "
        f"matrix type needs a different scheme, not this helper."
    )
    return row_size


_STRUCT_ARITY = {
    X3DType.SFVec2f: ("SFVec2f", 2, True, 0),
    X3DType.SFVec3f: ("SFVec3f", 3, True, 0),
    X3DType.SFVec4f: ("SFVec4f", 4, True, 0),
    X3DType.SFColor: ("SFColor", 3, True, 0),
    X3DType.SFColorRGBA: ("SFColorRGBA", 4, True, 0),
    X3DType.SFVec2d: ("SFVec2d", 2, False, 0),
    X3DType.SFVec3d: ("SFVec3d", 3, False, 0),
    X3DType.SFVec4d: ("SFVec4d", 4, False, 0),
    X3DType.SFRotation: ("SFRotation", 4, True, 0),
    X3DType.SFMatrix3f: ("SFMatrix3f", 9, True, _matrix_row_size(9)),
    X3DType.SFMatrix4f: ("SFMatrix4f", 16, True, _matrix_row_size(16)),
    X3DType.SFMatrix3d: ("SFMatrix3d", 9, False, _matrix_row_size(9)),
    X3DType.SFMatrix4d: ("SFMatrix4d", 16, False, _matrix_row_size(16)),
}


def struct_arity_names() -> set:
    """Return the set of struct names in _STRUCT_ARITY (public accessor).

    Used by generator.py to verify that every SPECIAL_STRUCTS entry has a
    corresponding _STRUCT_ARITY row for default literal generation.
    """
    return {arity[0] for arity in _STRUCT_ARITY.values()}


# MF struct types -> (element struct name, component count, is-float). A single
# element is built from the default.
_MF_STRUCT_ELEM = {
    X3DType.MFVec2f: ("SFVec2f", 2, True),
    X3DType.MFVec3f: ("SFVec3f", 3, True),
    X3DType.MFVec4f: ("SFVec4f", 4, True),
    X3DType.MFVec2d: ("SFVec2d", 2, False),
    X3DType.MFVec3d: ("SFVec3d", 3, False),
    X3DType.MFVec4d: ("SFVec4d", 4, False),
    X3DType.MFColor: ("SFColor", 3, True),
    X3DType.MFColorRGBA: ("SFColorRGBA", 4, True),
    X3DType.MFRotation: ("SFRotation", 4, True),
}


def _chunk_braced(vals: List[str], size: int, *, pad_short: bool,
                  floaty: bool = True) -> List[str]:
    """Slice ``vals`` into ``size``-sized groups, brace-wrapping each.

    ``pad_short=True`` zero-pads a short final group to exactly ``size``
    (matrix rows: the caller has already zero-padded the WHOLE list to a
    multiple of ``size`` via the arity guard, so this only matters if a
    caller passes an un-padded list directly). ``pad_short=False`` drops a
    ragged trailing remainder instead (MF-struct elements: a multi-element
    default with a trailing partial element is intentionally truncated, not
    padded with synthetic zeros the spec never declared).
    """
    zero = "0.0f" if floaty else "0.0"
    if pad_short:
        vals = list(vals)
        while len(vals) % size != 0:
            vals.append(zero)
        stop = len(vals)
    else:
        stop = len(vals) - len(vals) % size
    return [
        "{" + ", ".join(vals[i:i + size]) + "}"
        for i in range(0, stop, size)
    ]


def _struct_literal(struct: str, count: int, floaty: bool, row_size: int, d: str) -> str:
    vals = _scalar_list(d)
    zero = "0.0f" if floaty else "0.0"
    while len(vals) < count:  # arity guard: never read OOB
        vals.append(zero)
    vals = vals[:count]
    if row_size:
        rows = _chunk_braced(vals, row_size, pad_short=False, floaty=floaty)
        return struct + "{{" + ", ".join(rows) + "}}"
    return struct + "{" + ", ".join(vals) + "}"


def default_expr_for(x3d_type: X3DType, default: Optional[str]) -> Optional[str]:
    """C++ default initializer expression for ``(x3d_type, default)``.

    Returns ``None`` when there is no spec default (member value-initializes).
    """
    if default is None or x3d_type is None:
        return None
    d = default

    # Fixed-arity struct types (vectors / colours / rotation / matrices).
    if x3d_type in _STRUCT_ARITY:
        return _struct_literal(*_STRUCT_ARITY[x3d_type], d=d)

    if x3d_type is X3DType.SFImage:
        vals = _scalar_list(d)
        while len(vals) < 3:
            vals.append("0")
        return ("SFImage{" + ", ".join(vals[:3]) +
                ", std::vector<unsigned char>{}}")

    if x3d_type is X3DType.SFBool:
        return "true" if str(d).strip().lower() == "true" else "false"
    if x3d_type is X3DType.SFInt32:
        return str(d).strip() or "0"
    if x3d_type is X3DType.SFFloat:
        return str(d).strip() or "0"
    if x3d_type in (X3DType.SFDouble, X3DType.SFTime):
        return str(d).strip() or "0"
    if x3d_type in (X3DType.SFString, X3DType.XS_NMTOKEN):
        return f'"{cpp_string_literal(d)}"'
    if x3d_type is X3DType.SFNode:
        return "nullptr"

    # Multi-field (MF*) types.
    if x3d_type is X3DType.MFString:
        toks = tokenize_mfstring(d)
        if not toks:
            return "std::vector<std::string>{}"
        body = ", ".join(f'"{cpp_string_literal(tok)}"' for tok in toks)
        return "std::vector<std::string>{" + body + "}"
    if x3d_type is X3DType.MFInt32:
        return "std::vector<int>{" + ", ".join(_scalar_list(d)) + "}"
    if x3d_type is X3DType.MFFloat:
        return "std::vector<float>{" + ", ".join(_scalar_list(d)) + "}"
    if x3d_type in (X3DType.MFDouble, X3DType.MFTime):
        return "std::vector<double>{" + ", ".join(_scalar_list(d)) + "}"
    if x3d_type is X3DType.MFBool:
        vals = ["true" if v.strip().lower() == "true" else "false"
                for v in _scalar_list(d)]
        return "std::vector<bool>{" + ", ".join(vals) + "}"
    if x3d_type in _MF_STRUCT_ELEM:
        struct, count, floaty = _MF_STRUCT_ELEM[x3d_type]
        vals = _scalar_list(d)
        if len(vals) < count:
            return f"std::vector<{struct}>{{}}"
        # Chunk the flat scalar list into element-sized groups so a MULTI-element
        # default (e.g. Extrusion.crossSection = the 5-point square, spine = the
        # 2-point segment) emits every element, not just the first. Trailing
        # scalars that do not fill a whole element are dropped.
        chunks = _chunk_braced(vals, count, pad_short=False, floaty=floaty)
        elems = [struct + chunk for chunk in chunks]
        return f"std::vector<{struct}>{{" + ", ".join(elems) + "}"
    if TypeRegistry.is_multi(x3d_type):
        # Remaining MF types (e.g. MFNode/MFImage/MFMatrix*) value-initialize.
        return None

    # Fallback: numeric/raw default.
    return str(d).strip()


def enum_default_expr(enum_def, default: Optional[str]) -> Optional[str]:
    """C++ default initializer for an enum-typed field.

    For a single-valued (SF) enum field this is ``EnumClass::MEMBER``. For a
    multi-valued (MF) enum field it is ``std::vector<EnumClass>{MEMBER, ...}``
    built from the (quoted, whitespace-separated) default tokens.

    An SF default token that doesn't match any member is a spec/generator
    mismatch (a renamed token, a typo, a newer spec revision) -- raising here
    turns a silently-wrong generated default into a loud generation-time
    failure, rather than emitting a default that happens to compile but does
    not match what the X3D spec actually says. An MF default with SOME
    unmatched tokens among otherwise-valid ones is not escalated the same
    way (a single bad token in a longer list is much lower-blast-radius than
    an entirely wrong SF default) but is printed loudly so it's visible.
    """
    cpp = enum_def.cpp_name
    if not enum_def.is_multi:
        if default is None:
            return None
        member = enum_def.member_for_value(default)
        if member is None:
            raise ValueError(
                f"Enum default token {default!r} does not match any member "
                f"of SimpleType {enum_def.name!r} (known: "
                f"{[m.value for m in enum_def.members]}). This is a spec/"
                f"generator mismatch -- check for a renamed token or a UOM "
                f"version drift, and update model/enums.py's parsing or the "
                f"UOM source, not this generator."
            )
        return f"{cpp}::{member.cpp_name}"

    # Multi-valued enum field: vector of enum members. Unmatched tokens are
    # dropped (not fatal -- see docstring) but printed so they're visible.
    toks = tokenize_mfstring(default) if default else []
    members = []
    for t in toks:
        m = enum_def.member_for_value(t)
        if m is None:
            print(f"WARNING: enum default token {t!r} on SimpleType "
                  f"{enum_def.name!r} does not match any member; dropping "
                  f"it from the generated default.")
            continue
        members.append(m)
    if not members:
        return f"std::vector<{cpp}>{{}}"
    body = ", ".join(f"{cpp}::{m.cpp_name}" for m in members)
    return f"std::vector<{cpp}>{{{body}}}"


def compute_default_expr(field) -> Optional[str]:
    """Backwards-compatible field-keyed entry point.

    Resolves ``field.type`` (a stored type string) to its :class:`X3DType` and
    delegates to :func:`default_expr_for`. Kept so existing call sites and tests
    that pass a field with a ``.type`` string continue to work unchanged.
    """
    if field.default is None:
        return None
    x3d_type = resolve_x3d_type(field.type)
    if x3d_type is not None:
        return default_expr_for(x3d_type, field.default)
    # Field type was given as a raw C++ type string rather than an X3D name.
    # The real generator never does this (fields store X3D type names), but a
    # few historical call sites/tests pass cpp spellings; honour them here.
    t = field.type
    d = field.default
    if t == "SFNode" or t.startswith("std::shared_ptr"):
        return "nullptr"
    if t in ("int", "float", "double", "std::string"):
        if t == "std::string":
            return f'"{cpp_string_literal(d)}"'
        return str(d).strip() or "0"
    if t == "bool":
        return "true" if str(d).strip().lower() == "true" else "false"
    # Unknown type string: preserve the old raw-default fallback.
    return str(d).strip()
