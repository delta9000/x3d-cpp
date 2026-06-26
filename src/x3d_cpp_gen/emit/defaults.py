"""Default-literal generation, keyed on :class:`X3DType`.

Replaces the giant ``{% if field.type == 'SFVec3f' %} ...`` Jinja default ladder.
The single :func:`default_expr_for` entry point computes the C++ initializer
expression for a field's spec default (or ``None`` when there is none, in which
case the member value-initializes with ``{}``).
"""

from typing import List, Optional

from x3d_cpp_gen.model.types import X3DType, TypeRegistry, resolve_x3d_type


def cpp_string_literal(s: str) -> str:
    """Escape a Python string into the body of a C++ double-quoted literal."""
    out = []
    for ch in s:
        if ch == '\\':
            out.append('\\\\')
        elif ch == '"':
            out.append('\\"')
        elif ch == '\n':
            out.append('\\n')
        elif ch == '\t':
            out.append('\\t')
        elif ch == '\r':
            out.append('\\r')
        else:
            out.append(ch)
    return ''.join(out)


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


# Struct SF* types -> (struct name, component count, is-float). Drives the brace
# initializer for fixed-arity vector/colour/rotation defaults.
_STRUCT_ARITY = {
    X3DType.SFVec2f: ("SFVec2f", 2, True),
    X3DType.SFVec3f: ("SFVec3f", 3, True),
    X3DType.SFVec4f: ("SFVec4f", 4, True),
    X3DType.SFColor: ("SFColor", 3, True),
    X3DType.SFColorRGBA: ("SFColorRGBA", 4, True),
    X3DType.SFVec2d: ("SFVec2d", 2, False),
    X3DType.SFVec3d: ("SFVec3d", 3, False),
    X3DType.SFVec4d: ("SFVec4d", 4, False),
    X3DType.SFRotation: ("SFRotation", 4, True),
    X3DType.SFMatrix3f: ("SFMatrix3f", 9, True),
    X3DType.SFMatrix4f: ("SFMatrix4f", 16, True),
    X3DType.SFMatrix3d: ("SFMatrix3d", 9, False),
    X3DType.SFMatrix4d: ("SFMatrix4d", 16, False),
}

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


def _struct_literal(struct: str, count: int, floaty: bool, d: str) -> str:
    vals = _scalar_list(d)
    zero = "0.0f" if floaty else "0.0"
    while len(vals) < count:  # arity guard: never read OOB
        vals.append(zero)
    return f"{struct}{{" + ", ".join(vals[:count]) + "}"


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
        if not vals:
            return f"std::vector<{struct}>{{}}"
        elem = _struct_literal(struct, count, floaty, d)
        return f"std::vector<{struct}>{{{elem}}}"
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

    Unrecognised default tokens fall back to the first member (SF) or are
    skipped (MF) so the emitted initializer is always well-formed.
    """
    cpp = enum_def.cpp_name
    if not enum_def.is_multi:
        if default is None:
            return None
        member = enum_def.member_for_value(default)
        if member is None:
            member = enum_def.members[0] if enum_def.members else None
        if member is None:
            return None
        return f"{cpp}::{member.cpp_name}"

    # Multi-valued enum field: vector of enum members.
    toks = tokenize_mfstring(default) if default else []
    members = [enum_def.member_for_value(t) for t in toks]
    members = [m for m in members if m is not None]
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
