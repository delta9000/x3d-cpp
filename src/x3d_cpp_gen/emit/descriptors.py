"""Per-field descriptors: every per-field decision computed in Python.

A :class:`FieldDescriptor` is the fully-resolved, render-ready view of one X3D
field. :func:`build_descriptor` computes ALL decisions — C++ type, accessor
names, default literal, access-type role flags, move-overload eligibility, and
the *rendered* constraint-check C++ — so the Jinja template can stay thin and
never branch on raw type strings.
"""

from dataclasses import dataclass, field as dataclass_field
from typing import Dict, List, Optional

from x3d_cpp_gen.emit.defaults import default_expr_for, enum_default_expr
from x3d_cpp_gen.emit.naming import pascal
from x3d_cpp_gen.model.types import X3DType, TypeRegistry, resolve_x3d_type

# SFColor / SFColorRGBA always validate against [0, 1] per project decision,
# regardless of any spec-declared min/max.
_COLOR_TYPES = (X3DType.SFColor, X3DType.SFColorRGBA,
                X3DType.MFColor, X3DType.MFColorRGBA)


@dataclass
class FieldDescriptor:
    """Render-ready description of a single field."""

    x3d_name: str
    cpp_ident: str
    # PascalCase of the C++-sanitized field name (used to build C++ identifiers
    # like getDefault<Name>() / acceptable<Name>NodeTypes()). Distinct from
    # x3d_name, which is the wire name and may differ for sanitized fields
    # (e.g. wire "class" -> cpp ident base "Class_").
    name_pascal: str
    x3d_type: Optional[X3DType]
    cpp_type: str
    access_type: str
    description: str
    default_init_expr: Optional[str]
    value_init: bool                      # member uses {} (no spec default)
    constraint_checks: Optional[str]      # rendered C++ validator body, or None
    range_collect_body: Optional[str]     # rendered C++ for checkRanges<Name>(), or None
    getter_name: str
    setter_name: str
    handler_name: str                     # on<Name>() for inputOnly events
    validator_name: str
    is_event: bool                        # inputOnly
    is_readonly: bool                     # outputOnly
    needs_move_overload: bool
    acceptable_node_types: Optional[List[str]] = dataclass_field(default=None)
    # Name of the bounded enum class this field is typed as (or None). When set,
    # cpp_type is already the enum class (SF) or std::vector<EnumClass> (MF).
    enum_cpp_name: Optional[str] = dataclass_field(default=None)
    # The ancestor class this field was inherited from (field/@inheritedFrom), or
    # None for a node's own field. Used by reflection thunks to base-qualify an
    # accessor call (Base::getX()) when the same field name is reachable through
    # more than one base subobject (diamond), which would otherwise be ambiguous.
    inherited_from: Optional[str] = dataclass_field(default=None)

    @property
    def getter_call(self) -> str:
        """The accessor name a reflection thunk calls, base-qualified if needed."""
        if self.inherited_from:
            return f"{self.inherited_from}::{self.getter_name}"
        return self.getter_name

    @property
    def setter_call(self) -> str:
        if self.inherited_from:
            return f"{self.inherited_from}::{self.setter_name}"
        return self.setter_name

    @property
    def handler_setter_name(self) -> str:
        """Registrar for an inputOnly event handler: setOn<Name>Handler()."""
        return f"setOn{self.name_pascal}Handler"

    @property
    def handler_member(self) -> str:
        """Member holding the registered inputOnly handler (a std::function)."""
        return f"_on{self.name_pascal}Handler"

    @property
    def handler_call(self) -> str:
        """The event handler a reflection thunk calls, base-qualified if needed."""
        if self.inherited_from:
            return f"{self.inherited_from}::{self.handler_name}"
        return self.handler_name

    @property
    def emitter_name(self) -> str:
        """Runtime write path for an outputOnly field: emit<Name>().

        outputOnly fields have no public setter (they are read-only to scene
        authors), but a node's behavior/runtime must be able to produce output
        events. emit<Name>() writes the field's value; the cascade then routes it.
        """
        return f"emit{self.name_pascal}"

    @property
    def emitter_call(self) -> str:
        """The emitter a reflection thunk calls, base-qualified if needed."""
        if self.inherited_from:
            return f"{self.inherited_from}::{self.emitter_name}"
        return self.emitter_name

    @property
    def setter_unchecked_name(self) -> str:
        """Non-validating writer for a constrained field: set<Name>Unchecked().

        Assigns without the range check the public set<Name>() enforces, so a
        permissive reader can keep an out-of-range authored value instead of
        rejecting the whole document. Only generated for constrained fields.
        """
        return f"{self.setter_name}Unchecked"

    @property
    def range_checker_name(self) -> str:
        """Non-throwing range collector for a constrained field: checkRanges<Name>().

        Base-qualified when the field is inherited (e.g. from an AbstractObjectType
        mixin), so a derived node's validateRanges() can reach the static defined on
        the base — mirroring getter_call / reader_setter_call. Only meaningful when
        range_collect_body is not None (constrained fields).
        """
        name_pascal = self.validator_name[len("validate"):]  # reuse the Pascal stem
        name = f"checkRanges{name_pascal}"
        if self.inherited_from:
            return f"{self.inherited_from}::{name}"
        return name

    @property
    def reader_setter_call(self) -> str:
        """The setter a node-agnostic reflection write should call.

        Constrained fields route through the non-validating set<Name>Unchecked
        (the data layer does raw writes; the typed set<Name> is the enforcement
        point for programmatic callers). Unconstrained fields use the normal
        setter, which performs no validation anyway. Base-qualified if inherited.
        """
        # initializeOnly fields have no public set<Name>(); the data-layer write
        # goes through set<Name>Unchecked. Constrained inputOutput fields also use
        # the unchecked path (the typed set<Name> stays the enforcement point).
        if self.access_type == "initializeOnly" or self.has_constraints:
            name = self.setter_unchecked_name
        else:
            name = self.setter_name
        if self.inherited_from:
            return f"{self.inherited_from}::{name}"
        return name

    @property
    def is_enum(self) -> bool:
        return self.enum_cpp_name is not None

    @property
    def runtime_field_type(self) -> str:
        """The X3DFieldType enumerator (in X3DReflection.hpp) for this field.

        Mirrors the X3D type spelling so a node-agnostic codec can switch on it.
        Bounded-enum fields report ``MFEnum``/``SFEnum`` (the underlying value is
        a generated ``enum class`` / vector thereof, serialized as a token).
        """
        if self.is_enum:
            return "MFEnum" if self.cpp_type.startswith("std::vector<") else "SFEnum"
        if self.x3d_type is not None:
            return TypeRegistry.runtime_tag(self.x3d_type)
        # Unknown/newer-spec type with no enum: fall back to an opaque string.
        return "SFString"

    @property
    def runtime_access(self) -> str:
        """The AccessType enumerator (in X3DReflection.hpp) for this field."""
        return {
            "inputOutput": "InputOutput",
            "initializeOnly": "InitializeOnly",
            "outputOnly": "OutputOnly",
            "inputOnly": "InputOnly",
        }.get(self.access_type, "InputOutput")

    @property
    def container_field_default(self) -> str:
        """containerField placement hint for SFNode/MFNode child fields.

        Empty for value (attribute) fields. For node fields this is the field's
        own X3D name, which is also the default containerField a child written
        into this slot carries (the codec can override per the child's spec).
        """
        if self.x3d_type in (X3DType.SFNode, X3DType.MFNode):
            return self.x3d_name
        return ""

    @property
    def has_acceptable_node_types(self) -> bool:
        return bool(self.acceptable_node_types)

    # --- role predicates the template iterates on ---
    @property
    def is_readable(self) -> bool:
        """Has a getter: inputOutput / initializeOnly / outputOnly."""
        return self.access_type in (
            "inputOutput", "initializeOnly", "outputOnly")

    @property
    def is_settable(self) -> bool:
        """Has a setter: inputOutput only."""
        return self.access_type == "inputOutput"

    @property
    def has_data_setter(self) -> bool:
        """Writable at the data/initialization layer (reflection set thunk emitted).

        inputOutput (runtime-settable) AND initializeOnly (author-settable at parse
        time, but not a runtime/event setter). inputOnly/outputOnly are excluded.
        """
        return self.access_type in ("inputOutput", "initializeOnly")

    @property
    def has_member(self) -> bool:
        """Has a persisted member: everything except inputOnly events."""
        return not self.is_event

    @property
    def has_default(self) -> bool:
        return self.default_init_expr is not None

    @property
    def has_constraints(self) -> bool:
        return self.constraint_checks is not None


def _render_constraints(name: str, x3d_type: X3DType,
                        lo: Optional[str], hi: Optional[str]) -> str:
    """Render the C++ body of validate<Name>() for a constrained field.

    Mirrors the original Jinja constraint ladder exactly (component-wise checks
    for struct types, per-element loops for MF types, scalar checks otherwise).
    Returns the inner statements (no enclosing braces / signature).
    """
    comps = TypeRegistry.components(x3d_type)

    def group(target: str, suffix: str, label: str) -> str:
        """The 1-2 bound checks for a single (component or scalar) target."""
        out = []
        if lo is not None:
            out.append(
                f'if ({target} < {lo}) throw std::out_of_range('
                f'"{label} below minimum of {lo}");')
        if hi is not None:
            out.append(
                f'if ({target} > {hi}) throw std::out_of_range('
                f'"{label} above maximum of {hi}");')
        return "\n".join(out)

    def body(prefix: str) -> str:
        """Per-component groups, blank-line separated (matches clang-format)."""
        if comps:
            groups = [group(f"{prefix}.{c}", c, f"{name}.{c}") for c in comps]
        else:
            groups = [group(prefix, "", name)]
        # A blank line separates consecutive component groups.
        return "\n\n".join(groups)

    if TypeRegistry.is_multi(x3d_type):
        # The for-loop opens with a blank line before its first check, mirroring
        # the original template's per-iteration whitespace.
        return ("for (const auto& v : value) {\n\n" + body("v") + "\n}")
    return body("value")


def _render_range_collect(name: str, x3d_type: X3DType,
                          lo: Optional[str], hi: Optional[str]) -> str:
    """Render the C++ body of checkRanges<Name>(): the same bound ladder as
    _render_constraints, but APPENDING a RangeDiagnostic per violation instead of
    throwing. `nodeType` and `defName` are symbols the generated static receives
    (the template/static signature supplies them). Returns inner statements only.
    """
    comps = TypeRegistry.components(x3d_type)

    def push(detail: str) -> str:
        return f'out.push_back(RangeDiagnostic{{nodeType, defName, "{name}", "{detail}"}});'

    def group(target: str, label: str) -> str:
        out = []
        if lo is not None:
            out.append(f'if ({target} < {lo}) {push(f"{label} below minimum of {lo}")}')
        if hi is not None:
            out.append(f'if ({target} > {hi}) {push(f"{label} above maximum of {hi}")}')
        return "\n".join(out)

    def body(prefix: str) -> str:
        if comps:
            groups = [group(f"{prefix}.{c}", f"{name}.{c}") for c in comps]
        else:
            groups = [group(prefix, name)]
        return "\n\n".join(groups)

    if TypeRegistry.is_multi(x3d_type):
        return ("for (const auto& v : value) {\n\n" + body("v") + "\n}")
    return body("value")


def build_descriptor(field, enum_defs: Optional[Dict] = None) -> FieldDescriptor:
    """Compute the full :class:`FieldDescriptor` for a parsed X3D field.

    ``enum_defs`` maps SimpleType names to :class:`~x3d_cpp_gen.model.enums.EnumDef`.
    A field whose ``simple_type`` names a BOUNDED enum is re-typed as that enum
    class (SF) or ``std::vector<EnumClass>`` (MF) instead of std::string.
    """
    x3d_type = resolve_x3d_type(field.type)

    access = field.accessType
    is_event = access == "inputOnly"
    is_readonly = access == "outputOnly"

    # --- Bounded-enum fields short-circuit the normal type resolution. ---
    enum_def = None
    simple_type = getattr(field, "simple_type", None)
    if enum_defs and simple_type and simple_type in enum_defs:
        enum_def = enum_defs[simple_type]

    if enum_def is not None:
        return _build_enum_descriptor(field, enum_def, access, is_event, is_readonly)

    # Signatures/members use the X3D type *spelling* as stored on the field
    # (e.g. 'SFBool', or the remapped 'xs_nmtoken'), NOT the resolved C++ type.
    # X3Dtypes.hpp provides the matching `typedef bool SFBool;` aliases, and the
    # golden output is emitted in terms of those spellings.
    cpp_type = field.type

    default_expr = (
        default_expr_for(x3d_type, field.default) if x3d_type is not None
        else (str(field.default).strip() if field.default is not None else None)
    )

    # Constraint resolution: a color type always clamps to [0,1]; otherwise use
    # the spec-declared inclusive bounds. Validation only exists for settable
    # (inputOutput) fields, matching the original template gate.
    is_color = x3d_type in _COLOR_TYPES
    lo = field.min_inclusive
    hi = field.max_inclusive
    if is_color:
        lo, hi = "0", "1"
    wants_validation = (
        access == "inputOutput"
        and x3d_type is not None
        and (field.min_inclusive is not None
             or field.max_inclusive is not None
             or is_color)
    )
    constraint_checks = (
        _render_constraints(field.name, x3d_type, lo, hi)
        if wants_validation else None
    )
    range_collect_body = (
        _render_range_collect(field.name, x3d_type, lo, hi)
        if wants_validation else None
    )

    name_pascal = pascal(field.name)
    needs_move = (
        access == "inputOutput"
        and x3d_type is not None
        and TypeRegistry.needs_move_overload(x3d_type)
    )

    return FieldDescriptor(
        # x3d_name is the WIRE name (e.g. "class"); the C++ identifier / accessor
        # names below stay derived from the sanitized field.name (e.g. "class_").
        x3d_name=getattr(field, "x3d_name", None) or field.name,
        cpp_ident=f"_{field.name}",
        name_pascal=name_pascal,
        x3d_type=x3d_type,
        cpp_type=cpp_type,
        access_type=access,
        description=field.description or "",
        default_init_expr=default_expr,
        value_init=default_expr is None,
        constraint_checks=constraint_checks,
        range_collect_body=range_collect_body,
        getter_name=f"get{name_pascal}",
        setter_name=f"set{name_pascal}",
        handler_name=f"on{name_pascal}",
        validator_name=f"validate{name_pascal}",
        is_event=is_event,
        is_readonly=is_readonly,
        needs_move_overload=needs_move,
        acceptable_node_types=field.acceptable_node_types,
        enum_cpp_name=None,
        inherited_from=getattr(field, "inherited_from", None),
    )


def _build_enum_descriptor(field, enum_def, access, is_event,
                           is_readonly) -> FieldDescriptor:
    """Build the descriptor for a field typed as a bounded enum class.

    SF enum fields become the enum class itself; MF enum fields become
    ``std::vector<EnumClass>``. No numeric/range constraints apply. The setter
    takes the enum by value (SF) / vector by const-ref + move (MF).
    """
    cpp = enum_def.cpp_name
    is_multi = enum_def.is_multi
    cpp_type = f"std::vector<{cpp}>" if is_multi else cpp

    default_expr = enum_default_expr(enum_def, field.default)

    name_pascal = pascal(field.name)
    # Pass-by-value (and move) the small enum scalar; move only helps the vector.
    needs_move = access == "inputOutput" and is_multi
    return FieldDescriptor(
        x3d_name=getattr(field, "x3d_name", None) or field.name,
        cpp_ident=f"_{field.name}",
        name_pascal=name_pascal,
        x3d_type=None,
        cpp_type=cpp_type,
        access_type=access,
        description=field.description or "",
        default_init_expr=default_expr,
        value_init=default_expr is None,
        constraint_checks=None,
        range_collect_body=None,
        getter_name=f"get{name_pascal}",
        setter_name=f"set{name_pascal}",
        handler_name=f"on{name_pascal}",
        validator_name=f"validate{name_pascal}",
        is_event=is_event,
        is_readonly=is_readonly,
        needs_move_overload=needs_move,
        acceptable_node_types=field.acceptable_node_types,
        enum_cpp_name=cpp,
        inherited_from=getattr(field, "inherited_from", None),
    )


def build_descriptors(fields, enum_defs: Optional[Dict] = None) -> List[FieldDescriptor]:
    return [build_descriptor(f, enum_defs) for f in fields]
