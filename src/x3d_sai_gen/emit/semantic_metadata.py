"""Generate an owning, instance-free catalog of X3D semantic descriptors.

The runtime reflection table deliberately contains callable ``std::any``
thunks.  External language bindings need the schema facts without constructing
nodes or inheriting runtime identity, so this emitter produces a separate
read-only catalog from the same parsed UOM model.
"""

import hashlib
import json
from typing import Dict, List, Optional

from x3d_cpp_gen import __version__
from x3d_cpp_gen.emit.descriptors import build_reflection_descriptors
from x3d_cpp_gen.emit.naming import cpp_str
from x3d_cpp_gen.emit.registry import interfaces_of
from x3d_cpp_gen.parser import X3DNode, get_own_fields


def _quoted(value: str) -> str:
    return f'"{cpp_str(value or "")}"'


def _optional(value: Optional[str]) -> str:
    return "std::nullopt" if value is None else _quoted(value)


def _strings(values) -> str:
    body = ", ".join(_quoted(value) for value in (values or []))
    return f"std::vector<std::string>{{{body}}}"


def gen_semantic_metadata_header() -> str:
    return """// X3DSemanticMetadataRegistry.hpp
// Auto-generated: owning, instance-free X3D semantic descriptors.
#pragma once

#include "x3d/core/X3DReflection.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace x3d::nodes {

struct SemanticFieldDescriptor {
    std::string name;
    x3d::core::X3DFieldType type;
    x3d::core::AccessType access;
    std::optional<std::string> defaultValue;
    std::vector<std::string> acceptableNodeTypes;
    std::optional<std::string> unitCategory;
};

struct SemanticNodeDescriptor {
    std::string name;
    bool abstract = false;
    std::string component;
    int level = 0;
    std::vector<std::string> interfaces;
    std::vector<SemanticFieldDescriptor> fields;
};

class X3DSemanticMetadataRegistry {
public:
    static std::string_view specificationVersion() noexcept;
    static std::string_view modelFingerprint() noexcept;
    static std::string_view generatorVersion() noexcept;
    static bool unitCategoriesComplete() noexcept;
    static std::span<const SemanticNodeDescriptor> nodes();
    static const SemanticNodeDescriptor* find(const std::string& name);
};

} // namespace x3d::nodes
"""


def gen_semantic_metadata_source(nodes: Dict[str, X3DNode], graph,
                                 enum_defs=None, spec_version="unknown") -> str:
    def wire(field):
        return getattr(field, "x3d_name", None) or field.name

    own_field_names = {
        node.name: {wire(field) for field in get_own_fields(node)}
        for node in nodes.values()
    }

    def ancestors(node_name):
        seen = []
        pending = list(graph.get(node_name, []))
        while pending:
            base = pending.pop(0)
            if base in seen:
                continue
            seen.append(base)
            pending.extend(graph.get(base, []))
        return seen

    semantic_model = []
    lines: List[str] = [
        "// X3DSemanticMetadataRegistry.cpp",
        "// Auto-generated: owning semantic descriptor catalog.",
        '#include "x3d/nodes/X3DSemanticMetadataRegistry.hpp"',
        "",
        "#include <algorithm>",
        "",
        "namespace x3d::nodes {",
        "using x3d::core::AccessType;",
        "using x3d::core::X3DFieldType;",
        "namespace {",
        "const std::vector<SemanticNodeDescriptor>& catalog() {",
        "    static const std::vector<SemanticNodeDescriptor> value = {",
    ]
    for name in sorted(nodes):
        node = nodes[name]
        component = node.component.name if node.component else ""
        level = node.component.level if node.component and node.component.level is not None else 0
        interfaces = interfaces_of(name, nodes, graph)
        semantic_fields = []
        lines.append("        SemanticNodeDescriptor{")
        lines.append(
            f"            {_quoted(node.name)}, {'true' if node.is_abstract else 'false'}, "
            f"{_quoted(component)}, {level},")
        lines.append(f"            {_strings(interfaces)},")
        lines.append("            std::vector<SemanticFieldDescriptor>{")
        rendered = build_reflection_descriptors(
            node, own_field_names=own_field_names,
            ancestors=ancestors(node.name), enum_defs=enum_defs)
        raw_fields = {wire(field): field for field in node.fields}
        for descriptor in rendered:
            field = raw_fields[descriptor.x3d_name]
            unit = getattr(field, "unit_category", None)
            semantic_fields.append({
                "name": descriptor.x3d_name,
                "type": descriptor.runtime_field_type,
                "access": descriptor.runtime_access,
                "default": field.default,
                "accepted_node_types": list(
                    descriptor.acceptable_node_types or []),
                "unit_category": unit,
            })
            lines.append(
                "                SemanticFieldDescriptor{"
                f"{_quoted(descriptor.x3d_name)}, "
                f"X3DFieldType::{descriptor.runtime_field_type}, "
                f"AccessType::{descriptor.runtime_access}, "
                f"{_optional(field.default)}, "
                f"{_strings(descriptor.acceptable_node_types)}, "
                f"{_optional(unit)}"
                "},")
        lines.extend(["            },", "        },"])
        semantic_model.append({
            "name": node.name,
            "abstract": bool(node.is_abstract),
            "component": component,
            "level": level,
            "interfaces": list(interfaces),
            "fields": semantic_fields,
        })
    fingerprint_payload = json.dumps(
        {"specification_version": spec_version, "nodes": semantic_model},
        ensure_ascii=False, separators=(",", ":"), sort_keys=True,
    ).encode("utf-8")
    model_fingerprint = hashlib.sha256(fingerprint_payload).hexdigest()
    lines.extend([
        "    };",
        "    return value;",
        "}",
        "} // namespace",
        "",
        "std::string_view X3DSemanticMetadataRegistry::specificationVersion() noexcept {",
        f"    return {_quoted(spec_version)};",
        "}",
        "",
        "std::string_view X3DSemanticMetadataRegistry::modelFingerprint() noexcept {",
        f"    return {_quoted(model_fingerprint)};",
        "}",
        "",
        "std::string_view X3DSemanticMetadataRegistry::generatorVersion() noexcept {",
        f"    return {_quoted(__version__)};",
        "}",
        "",
        "bool X3DSemanticMetadataRegistry::unitCategoriesComplete() noexcept {",
        "    return false;",
        "}",
        "",
        "std::span<const SemanticNodeDescriptor> X3DSemanticMetadataRegistry::nodes() {",
        "    return catalog();",
        "}",
        "",
        "const SemanticNodeDescriptor* X3DSemanticMetadataRegistry::find(const std::string& name) {",
        "    const auto& values = catalog();",
        "    const auto found = std::lower_bound(values.begin(), values.end(), name,",
        "        [](const SemanticNodeDescriptor& value, const std::string& key) {",
        "            return value.name < key;",
        "        });",
        "    return found != values.end() && found->name == name ? &*found : nullptr;",
        "}",
        "",
        "} // namespace x3d::nodes",
        "",
    ])
    return "\n".join(lines)
