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

# Types whose scalar element is float32 in C++ (SFFloat is `typedef float`,
# vec/color/rotation/matrix -f structs hold float members). Range comparisons
# for these must happen in float32: comparing the stored float against a wider
# double literal widens the value first, so a value at exactly a decimal bound
# that float can't represent (e.g. 1.570796) lands epsilon above/below it.
_FLOAT32_ELEMENT = {
    X3DType.SFFloat, X3DType.MFFloat,
    X3DType.SFColor, X3DType.MFColor,
    X3DType.SFColorRGBA, X3DType.MFColorRGBA,
    X3DType.SFVec2f, X3DType.MFVec2f,
    X3DType.SFVec3f, X3DType.MFVec3f,
    X3DType.SFVec4f, X3DType.MFVec4f,
    X3DType.SFRotation, X3DType.MFRotation,
    X3DType.SFMatrix3f, X3DType.MFMatrix3f,
    X3DType.SFMatrix4f, X3DType.MFMatrix4f,
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
        code: add the member to :class:`X3DType` and an entry to
        ``FIELD_TYPE_MAPPING`` (in both generator.py and the parser). The error
        message says so explicitly so the failure is actionable rather than a
        bare ``None``.
        """
        resolved = cls.resolve(type_string)
        if resolved is None:
            known = ", ".join(sorted(m.value for m in X3DType))
            raise KeyError(
                f"Unknown X3D field type {type_string!r}. If a newer spec "
                f"version added this type, register it as DATA: add an X3DType "
                f"member and a FIELD_TYPE_MAPPING entry in generator.py "
                f"(and the parser's FIELD_TYPE_MAPPING). Known types: {known}."
            )
        return resolved

    @classmethod
    def components(cls, t: "X3DType"):
        """Component accessor names for struct types, else None."""
        return _COMPONENTS.get(t)

    @classmethod
    def is_multi(cls, t: "X3DType") -> bool:
        return t.value.startswith("MF")

    @classmethod
    def is_float32_element(cls, t: "X3DType") -> bool:
        """True when the type's scalar element is C++ ``float``."""
        return t in _FLOAT32_ELEMENT

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
