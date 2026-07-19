"""Shared resolution of complete, executable node field descriptors."""

from x3d_cpp_gen.emit.descriptors import build_reflection_descriptors
from x3d_cpp_gen.parser import get_own_fields


def wire_name(field):
    return getattr(field, "x3d_name", None) or field.name


def resolved_node_fields(node, nodes, graph, enum_defs=None):
    """Return own plus real inherited fields, dropping UOM phantom fields."""
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
    return build_reflection_descriptors(
        node,
        own_field_names=own_field_names,
        ancestors=seen,
        enum_defs=enum_defs,
    )
