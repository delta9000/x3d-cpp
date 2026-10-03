"""Shared resolution of complete, executable node field descriptors."""

from x3d_sai_gen.emit.descriptors import build_reflection_descriptors
from x3d_sai_gen.parser import get_own_fields


def wire_name(field):
    return getattr(field, "x3d_name", None) or field.name


# The UOM also describes document syntax and HTML attributes as fields on
# X3DNode. They are not event ports in the abstract node interface. DEF/USE
# and IS are represented by names, shared identity and declaration bindings.
_X3D_NODE_SYNTAX = frozenset({"DEF", "USE", "IS", "class", "id", "style"})


def resolved_node_fields(node, nodes, graph, enum_defs=None):
    """Return the abstract node interface, excluding document syntax."""
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
    return [
        field for field in descriptors if field.x3d_name not in _X3D_NODE_SYNTAX
    ]
