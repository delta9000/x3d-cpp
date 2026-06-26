"""SimpleType enumeration model: parse the UOM ``<SimpleType>`` definitions into
named C++ ``enum class`` definitions.

The UOM XML declares ~87 ``<SimpleType>`` elements. The closed (BOUNDED)
vocabularies — those whose ``appinfo`` declares "This list is bounded" — become
strongly-typed C++ ``enum class`` definitions emitted in ``X3Denums.hpp``. Open
vocabularies (no bounded marker) stay as their underlying ``std::string`` base
type and are deliberately NOT turned into enums.

A field links to an enumeration via its ``simpleType`` attribute. Only bounded
enumerations participate; a field naming an open SimpleType keeps its base type.
"""

from dataclasses import dataclass, field as dataclass_field
from typing import Dict, List, Optional

from x3d_cpp_gen.emit.naming import pascal


@dataclass(frozen=True)
class EnumMember:
    """One enumeration value: the raw spec spelling and its C++ identifier."""

    value: str          # raw spec value (may contain quotes/spaces/dashes)
    cpp_name: str       # sanitized C++ enum member identifier


@dataclass
class EnumDef:
    """A bounded SimpleType rendered as a C++ ``enum class``."""

    name: str                       # raw SimpleType name (e.g. alphaModeChoices)
    cpp_name: str                   # PascalCase C++ enum class name
    base_type: str                  # underlying X3D base (SFString/MFString/...)
    members: List[EnumMember] = dataclass_field(default_factory=list)
    description: str = ""

    @property
    def is_multi(self) -> bool:
        """True when the field family is MF* (vector of enum values)."""
        return (self.base_type or "").startswith("MF")

    def member_for_value(self, raw: str) -> Optional[EnumMember]:
        """Resolve a (possibly quoted) default token to its enum member."""
        cleaned = raw.strip().strip('"')
        for m in self.members:
            if m.value.strip().strip('"') == cleaned:
                return m
        return None


def _sanitize_enum_member(value: str) -> str:
    """Turn a raw enumeration value into a valid, uppercase C++ identifier.

    Handles quotes, spaces, dashes/dots/slashes, leading digits and empty
    results. The transform is deterministic so generated output is stable.
    """
    s = (value or "").strip().strip('"')
    out = []
    for ch in s:
        if ch.isalnum():
            out.append(ch)
        else:
            out.append("_")
    ident = "".join(out).upper()
    # Collapse runs of underscores for readability, but keep it deterministic.
    while "__" in ident:
        ident = ident.replace("__", "_")
    ident = ident.strip("_")
    if not ident:
        ident = "VALUE"
    if ident[0].isdigit():
        ident = f"_{ident}"
    return ident


def _enum_cpp_name(simple_type_name: str) -> str:
    """C++ enum class name for a SimpleType name (PascalCase)."""
    return pascal(simple_type_name)


def _is_bounded(appinfo: str) -> bool:
    """A SimpleType is a closed vocabulary iff its appinfo says so.

    The UOM marks open vocabularies with the word "unbounded" — which itself
    contains the substring "bounded". An explicit "unbounded" declaration must
    therefore veto the match; otherwise an open vocabulary (e.g.
    ``hanimJointNameValues``) is wrongly emitted as a closed ``enum class`` that
    drops out-of-list values and elides its default on round-trip.
    """
    text = (appinfo or "").lower()
    if "unbounded" in text:
        return False
    return "bounded" in text


def parse_enum_defs(root) -> Dict[str, EnumDef]:
    """Parse every BOUNDED ``<SimpleType>`` with enumerations into an EnumDef.

    Returns a dict keyed by the raw SimpleType name. Open vocabularies and
    SimpleTypes without enumeration children are skipped (they stay scalar).
    Duplicate sanitized member identifiers within one enum are de-collided with
    a numeric suffix so the emitted ``enum class`` is always well-formed.
    """
    defs: Dict[str, EnumDef] = {}
    for st in root.findall('.//SimpleType'):
        appinfo = st.get('appinfo') or ""
        enum_elems = st.findall('enumeration')
        if not enum_elems or not _is_bounded(appinfo):
            continue
        name = st.get('name')
        base_type = st.get('baseType') or "SFString"
        members: List[EnumMember] = []
        seen: Dict[str, int] = {}
        for e in enum_elems:
            raw = e.get('value')
            if raw is None:
                continue
            ident = _sanitize_enum_member(raw)
            if ident in seen:
                seen[ident] += 1
                ident = f"{ident}_{seen[ident]}"
            else:
                seen[ident] = 0
            members.append(EnumMember(value=raw, cpp_name=ident))
        if not members:
            continue
        defs[name] = EnumDef(
            name=name,
            cpp_name=_enum_cpp_name(name),
            base_type=base_type,
            members=members,
            description=appinfo,
        )
    return defs
