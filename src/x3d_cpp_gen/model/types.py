"""The X3D type model: a single enum of every X3D field type plus a registry
mapping each enum member to its C++ type.

This is the spine of the IR refactor: every per-field decision (default literal,
constraint emission, move-overload eligibility, ...) keys on :class:`X3DType`
rather than on scattered raw type-name string comparisons.

The parser stores a field's type as a *string* — the raw X3D type name for the
SF*/MF* family (e.g. ``"SFBool"``) and the remapped C++ alias for ``xs:*`` types
(e.g. ``"xs_nmtoken"``). :func:`resolve_x3d_type` maps either spelling back to the
canonical :class:`X3DType` member so the rest of the pipeline never branches on
raw strings.
"""

from enum import Enum
from typing import Optional


class TypeKind(Enum):
    """Coarse structural category of an X3D type, used for shape decisions."""

    SCALAR = "scalar"        # bool / int / float / double / string / time
    STRUCT = "struct"        # SFVec*, SFColor*, SFRotation, SFMatrix*, SFImage
    NODE = "node"            # SFNode (shared_ptr)
    MULTI = "multi"          # any MF* type
    XS = "xs"                # xs:* aliased types


class X3DType(Enum):
    """One member per X3D field type (SF*/MF*) plus the supported ``xs:*`` types.

    Each value is the canonical raw X3D type name (the ``type`` attribute as it
    appears in the UOM XML). ``xs:*`` members keep their XML spelling as value.
    """

    # --- Single-value types ---
    SFBool = "SFBool"
    SFColor = "SFColor"
    SFColorRGBA = "SFColorRGBA"
    SFDouble = "SFDouble"
    SFFloat = "SFFloat"
    SFImage = "SFImage"
    SFInt32 = "SFInt32"
    SFMatrix3d = "SFMatrix3d"
    SFMatrix3f = "SFMatrix3f"
    SFMatrix4d = "SFMatrix4d"
    SFMatrix4f = "SFMatrix4f"
    SFNode = "SFNode"
    SFRotation = "SFRotation"
    SFString = "SFString"
    SFTime = "SFTime"
    SFVec2d = "SFVec2d"
    SFVec2f = "SFVec2f"
    SFVec3d = "SFVec3d"
    SFVec3f = "SFVec3f"
    SFVec4d = "SFVec4d"
    SFVec4f = "SFVec4f"
    # --- Multi-value types ---
    MFBool = "MFBool"
    MFColor = "MFColor"
    MFColorRGBA = "MFColorRGBA"
    MFDouble = "MFDouble"
    MFFloat = "MFFloat"
    MFImage = "MFImage"
    MFInt32 = "MFInt32"
    MFMatrix3d = "MFMatrix3d"
    MFMatrix3f = "MFMatrix3f"
    MFMatrix4d = "MFMatrix4d"
    MFMatrix4f = "MFMatrix4f"
    MFNode = "MFNode"
    MFRotation = "MFRotation"
    MFString = "MFString"
    MFTime = "MFTime"
    MFVec2d = "MFVec2d"
    MFVec2f = "MFVec2f"
    MFVec3d = "MFVec3d"
    MFVec3f = "MFVec3f"
    MFVec4d = "MFVec4d"
    MFVec4f = "MFVec4f"
    # --- XML-schema aliased types ---
    XS_NMTOKEN = "xs:NMTOKEN"


# Canonical C++ type for every X3D type. SFColor is deliberately its own struct,
# distinct from SFVec3f, even though both are three floats.
_CPP_TYPE = {
    X3DType.SFBool: "bool",
    X3DType.SFColor: "SFColor",
    X3DType.SFColorRGBA: "SFColorRGBA",
    X3DType.SFDouble: "double",
    X3DType.SFFloat: "float",
    X3DType.SFImage: "SFImage",
    X3DType.SFInt32: "int",
    X3DType.SFMatrix3d: "SFMatrix3d",
    X3DType.SFMatrix3f: "SFMatrix3f",
    X3DType.SFMatrix4d: "SFMatrix4d",
    X3DType.SFMatrix4f: "SFMatrix4f",
    X3DType.SFNode: "std::shared_ptr<X3DNode>",
    X3DType.SFRotation: "SFRotation",
    X3DType.SFString: "std::string",
    X3DType.SFTime: "double",
    X3DType.SFVec2d: "SFVec2d",
    X3DType.SFVec2f: "SFVec2f",
    X3DType.SFVec3d: "SFVec3d",
    X3DType.SFVec3f: "SFVec3f",
    X3DType.SFVec4d: "SFVec4d",
    X3DType.SFVec4f: "SFVec4f",
    X3DType.MFBool: "std::vector<bool>",
    X3DType.MFColor: "std::vector<SFColor>",
    X3DType.MFColorRGBA: "std::vector<SFColorRGBA>",
    X3DType.MFDouble: "std::vector<double>",
    X3DType.MFFloat: "std::vector<float>",
    X3DType.MFImage: "std::vector<SFImage>",
    X3DType.MFInt32: "std::vector<int>",
    X3DType.MFMatrix3d: "std::vector<SFMatrix3d>",
    X3DType.MFMatrix3f: "std::vector<SFMatrix3f>",
    X3DType.MFMatrix4d: "std::vector<SFMatrix4d>",
    X3DType.MFMatrix4f: "std::vector<SFMatrix4f>",
    X3DType.MFNode: "std::vector<std::shared_ptr<X3DNode>>",
    X3DType.MFRotation: "std::vector<SFRotation>",
    X3DType.MFString: "std::vector<std::string>",
    X3DType.MFTime: "std::vector<double>",
    X3DType.MFVec2d: "std::vector<SFVec2d>",
    X3DType.MFVec2f: "std::vector<SFVec2f>",
    X3DType.MFVec3d: "std::vector<SFVec3d>",
    X3DType.MFVec3f: "std::vector<SFVec3f>",
    X3DType.MFVec4d: "std::vector<SFVec4d>",
    X3DType.MFVec4f: "std::vector<SFVec4f>",
    X3DType.XS_NMTOKEN: "std::string",
}

# Component layout of struct-shaped types (used for per-component validation).
_COMPONENTS = {
    X3DType.SFColor: ("r", "g", "b"),
    X3DType.MFColor: ("r", "g", "b"),
    X3DType.SFColorRGBA: ("r", "g", "b", "a"),
    X3DType.MFColorRGBA: ("r", "g", "b", "a"),
    X3DType.SFVec2f: ("x", "y"),
    X3DType.SFVec2d: ("x", "y"),
    X3DType.MFVec2f: ("x", "y"),
    X3DType.MFVec2d: ("x", "y"),
    X3DType.SFVec3f: ("x", "y", "z"),
    X3DType.SFVec3d: ("x", "y", "z"),
    X3DType.MFVec3f: ("x", "y", "z"),
    X3DType.MFVec3d: ("x", "y", "z"),
    X3DType.SFVec4f: ("x", "y", "z", "w"),
    X3DType.SFVec4d: ("x", "y", "z", "w"),
    X3DType.MFVec4f: ("x", "y", "z", "w"),
    X3DType.MFVec4d: ("x", "y", "z", "w"),
    X3DType.SFRotation: ("x", "y", "z", "angle"),
    X3DType.MFRotation: ("x", "y", "z", "angle"),
}

# Scalar value-types whose copy setter does NOT also get a move overload (passing
# them by value/move is pointless; matches the original Jinja exclusion list).
_NO_MOVE_OVERLOAD = {
    X3DType.SFBool,
    X3DType.SFDouble,
    X3DType.SFFloat,
    X3DType.SFInt32,
}


class TypeRegistry:
    """Resolves type strings to :class:`X3DType` and exposes per-type facts."""

    #: Raw-string spellings the parser may store, mapped to their enum member.
    #: Includes both the X3D spelling and the remapped ``xs:*`` C++ alias.
    _BY_STRING = {}

    @classmethod
    def _ensure_index(cls):
        if cls._BY_STRING:
            return
        for member in X3DType:
            cls._BY_STRING[member.value] = member
        # The parser remaps xs:* types to their C++ alias before storing them.
        cls._BY_STRING["xs_nmtoken"] = X3DType.XS_NMTOKEN

    @classmethod
    def resolve(cls, type_string: str) -> Optional["X3DType"]:
        """Map a stored field-type string to its :class:`X3DType` (or None).

        Returns ``None`` for an unknown-but-well-formed type from a newer spec
        rather than crashing; callers decide whether to skip-with-warning (the
        parser does this) or escalate via :meth:`resolve_or_raise`.
        """
        cls._ensure_index()
        return cls._BY_STRING.get(type_string)

    @classmethod
    def resolve_or_raise(cls, type_string: str) -> "X3DType":
        """Like :meth:`resolve` but raise a clear, actionable error on miss.

        A newer X3D spec introducing a NEW field type is handled by DATA, not
        code: add the member to :class:`X3DType` and an entry to ``_CPP_TYPE``
        (and, for the parser, ``FIELD_TYPE_MAPPING``). The error message says so
        explicitly so the failure is actionable rather than a bare ``None``.
        """
        resolved = cls.resolve(type_string)
        if resolved is None:
            known = ", ".join(sorted(m.value for m in X3DType))
            raise KeyError(
                f"Unknown X3D field type {type_string!r}. If a newer spec "
                f"version added this type, register it as DATA: add an X3DType "
                f"member and a _CPP_TYPE entry in model/types.py (and a "
                f"FIELD_TYPE_MAPPING entry for the parser). Known types: {known}."
            )
        return resolved

    @classmethod
    def cpp_type(cls, t: "X3DType") -> str:
        return _CPP_TYPE[t]

    @classmethod
    def components(cls, t: "X3DType"):
        """Component accessor names for struct types, else None."""
        return _COMPONENTS.get(t)

    @classmethod
    def kind(cls, t: "X3DType") -> TypeKind:
        if t in (X3DType.SFNode, X3DType.MFNode):
            return TypeKind.NODE if t is X3DType.SFNode else TypeKind.MULTI
        if t is X3DType.XS_NMTOKEN:
            return TypeKind.XS
        if t.value.startswith("MF"):
            return TypeKind.MULTI
        if t in (
            X3DType.SFBool, X3DType.SFInt32, X3DType.SFFloat,
            X3DType.SFDouble, X3DType.SFTime, X3DType.SFString,
        ):
            return TypeKind.SCALAR
        return TypeKind.STRUCT

    @classmethod
    def is_multi(cls, t: "X3DType") -> bool:
        return t.value.startswith("MF")

    @classmethod
    def needs_move_overload(cls, t: "X3DType") -> bool:
        """True if the inputOutput setter should also get an rvalue overload."""
        return t not in _NO_MOVE_OVERLOAD

    @classmethod
    def runtime_tag(cls, t: "X3DType") -> str:
        """Name of the X3DFieldType enumerator a codec switches on for this type.

        The runtime tag mirrors the X3D type spelling for the SF*/MF* family so
        a hand-written codec can decide how to format/parse a value without any
        per-node code. ``xs:NMTOKEN`` is treated as a plain string at runtime.
        """
        if t is X3DType.XS_NMTOKEN:
            return "SFString"
        return t.name


def resolve_x3d_type(type_string: str) -> Optional[X3DType]:
    """Module-level convenience wrapper over :meth:`TypeRegistry.resolve`."""
    return TypeRegistry.resolve(type_string)
