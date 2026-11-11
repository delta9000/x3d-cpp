"""Project UOM node descriptions into executable SAI fields.

The parser preserves the UOM's encoding and host/document attributes. This
module is the narrower runtime boundary; see docs/generated-field-projection.md.
"""

from x3d_sai_gen.emit.descriptors import build_reflection_descriptors
from x3d_sai_gen.parser import get_own_fields


def wire_name(field):
    return getattr(field, "x3d_name", None) or field.name


# These UOM entries describe encoding identity, prototype binding syntax, or
# host-document integration, not ordinary fields in this standalone kernel.
_NON_RUNTIME_NAMES = frozenset({"DEF", "USE", "IS", "class", "id"})


def is_runtime_field(node, field):
    name = wire_name(field)
    if name in _NON_RUNTIME_NAMES:
        return False
    # Classify CSS style by its declaration, not by spelling alone: FontStyle
    # and ScreenFontStyle declare a real text-style field with the same name.
    declaring_node = field.inherited_from or node.name
    return not (name == "style" and declaring_node == "X3DNode")


def resolved_node_fields(node, nodes, graph, enum_defs=None):
    """Return real own/inherited runtime fields in UOM order.

    Encoding identity/bindings and host attributes stay in the raw parsed model
    but never become generic values, event ports, or generated typed field keys.
    """
    own_field_names = {
        candidate.name: {wire_name(field) for field in get_own_fields(candidate)}
        for candidate in nodes.values()
    }
    seen = []
    pending = list(graph.get(node.name, []))
    while pending:
        base = pending.pop(0)
        if base in seen:
            continue
        seen.append(base)
        pending.extend(graph.get(base, []))
    descriptors = build_reflection_descriptors(
        node,
        own_field_names=own_field_names,
        ancestors=seen,
        enum_defs=enum_defs,
    )
    # ISO/IEC 19775-1, 10.2.1: grouping children cannot repeat a non-NULL
    # node. Resolve this from actual grouping ancestry, never from the field
    # spelling alone or the adapter's provisional containment classification.
    grouping = node.name == "X3DGroupingNode" or "X3DGroupingNode" in seen
    for field in descriptors:
        field.unique_non_null_nodes = (
            grouping and field.x3d_name == "children"
            and field.runtime_field_type == "MFNode"
        )
    return [field for field in descriptors if is_runtime_field(node, field)]
