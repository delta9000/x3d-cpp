# src/x3d_cpp_gen/emit/registry.py
"""Generation of X3DInterfaceRegistry.hpp/.cpp: a node-type-name -> transitive
interface-set lookup.

Behaviors must ask "does this node implement interface X" without dynamic_cast
or string-name sniffing. The registry answers that from the SAME inheritance
data the headers are generated from: every abstract node type and mixin
object-type becomes an InterfaceId; every node (concrete + abstract) maps to the
TRANSITIVE CLOSURE of its abstract ancestors. Emitted centrally (one .hpp/.cpp
pair, like X3DNodeFactory) so the per-node golden headers are untouched.
"""
from typing import Dict, List

from x3d_cpp_gen.parser import X3DNode, resolve_inheritance_chain


def interface_ids(nodes: Dict[str, X3DNode]) -> List[str]:
    """Sorted names of every abstract node type + mixin object-type.

    These are the InterfaceId enumerators. Sorting gives deterministic output
    (required for the golden gate)."""
    return sorted(n.name for n in nodes.values() if n.is_abstract)


def interfaces_of(node_name: str, nodes: Dict[str, X3DNode],
                  graph: Dict[str, List[str]]) -> List[str]:
    """Transitive-closure abstract ancestors of ``node_name`` (sorted).

    resolve_inheritance_chain already walks primary + additional bases
    recursively and de-dupes; we keep only the ABSTRACT ones (the interfaces)."""
    abstract = {n.name for n in nodes.values() if n.is_abstract}
    chain = resolve_inheritance_chain(node_name, graph, set())
    return sorted(b for b in chain if b in abstract)


def gen_interface_registry_header(nodes: Dict[str, X3DNode]) -> str:
    ids = interface_ids(nodes)
    lines: List[str] = []
    lines.append("// X3DInterfaceRegistry.hpp")
    lines.append("// Auto-generated: node-type-name -> transitive interface set.")
    lines.append("#pragma once")
    lines.append("")
    lines.append("#include <cstdint>")
    lines.append("#include <span>")
    lines.append("#include <string>")
    lines.append("#include <vector>")
    lines.append("")
    lines.append("namespace x3d::nodes {")
    lines.append("")
    lines.append("class X3DNode;")
    lines.append("")
    lines.append("/// One enumerator per X3D abstract node type / mixin object-type.")
    lines.append("enum class InterfaceId : uint16_t {")
    for name in ids:
        lines.append(f"    {name},")
    lines.append("};")
    lines.append("")
    lines.append("/**")
    lines.append(" * @brief Queryable node-type -> interface-set registry (replaces dynamic_cast")
    lines.append(" *        / string-name type tests). Definitions live in the .cpp.")
    lines.append(" */")
    lines.append("class X3DInterfaceRegistry {")
    lines.append("public:")
    lines.append("    /// Transitive interface set for a node-type name (empty if unknown).")
    lines.append("    static std::span<const InterfaceId> interfacesOf(const std::string& nodeTypeName);")
    lines.append("    /// True if the named node-type implements ``iface`` (O(k), k = its #interfaces).")
    lines.append("    static bool nodeImplements(const std::string& nodeTypeName, InterfaceId iface);")
    lines.append("    /// Convenience overload: resolves the type name from a live node.")
    lines.append("    static bool nodeImplements(const X3DNode* node, InterfaceId iface);")
    lines.append("    /// All node-type names implementing ``iface`` (built once, sorted).")
    lines.append("    static const std::vector<std::string>& nodesImplementing(InterfaceId iface);")
    lines.append("};")
    lines.append("")
    lines.append("} // namespace x3d::nodes")
    lines.append("")
    return "\n".join(lines)


def gen_interface_registry_source(nodes: Dict[str, X3DNode],
                                  graph: Dict[str, List[str]]) -> str:
    # Emit a row for EVERY node (concrete + abstract) so the query works on any
    # type name. Sorted for deterministic golden output.
    names = sorted(nodes)
    lines: List[str] = []
    lines.append("// X3DInterfaceRegistry.cpp")
    lines.append("// Auto-generated: the membership tables + lookups.")
    lines.append('#include "x3d/nodes/X3DInterfaceRegistry.hpp"')
    lines.append("")
    lines.append('#include "x3d/nodes/X3DNode.hpp"')
    lines.append("")
    lines.append("#include <algorithm>")
    lines.append("#include <unordered_map>")
    lines.append("")
    lines.append("namespace x3d::nodes {")
    lines.append("")
    lines.append("namespace {")
    lines.append("// Each node-type's transitive interface set, stored contiguously.")
    lines.append("const std::unordered_map<std::string, std::vector<InterfaceId>>& table() {")
    lines.append("    static const std::unordered_map<std::string, std::vector<InterfaceId>> t = {")
    for name in names:
        ifaces = interfaces_of(name, nodes, graph)
        ids = ", ".join(f"InterfaceId::{i}" for i in ifaces)
        lines.append(f'        {{"{name}", {{{ids}}}}},')
    lines.append("    };")
    lines.append("    return t;")
    lines.append("}")
    lines.append("}  // namespace")
    lines.append("")
    lines.append("std::span<const InterfaceId>")
    lines.append("X3DInterfaceRegistry::interfacesOf(const std::string& nodeTypeName) {")
    lines.append("    static const std::vector<InterfaceId> empty;")
    lines.append("    auto it = table().find(nodeTypeName);")
    lines.append("    return it == table().end() ? std::span<const InterfaceId>(empty)")
    lines.append("                              : std::span<const InterfaceId>(it->second);")
    lines.append("}")
    lines.append("")
    lines.append("bool X3DInterfaceRegistry::nodeImplements(const std::string& nodeTypeName,")
    lines.append("                                          InterfaceId iface) {")
    lines.append("    for (InterfaceId i : interfacesOf(nodeTypeName))")
    lines.append("        if (i == iface) return true;")
    lines.append("    return false;")
    lines.append("}")
    lines.append("")
    lines.append("bool X3DInterfaceRegistry::nodeImplements(const X3DNode* node, InterfaceId iface) {")
    lines.append("    return node && nodeImplements(node->nodeTypeName(), iface);")
    lines.append("}")
    lines.append("")
    lines.append("const std::vector<std::string>&")
    lines.append("X3DInterfaceRegistry::nodesImplementing(InterfaceId iface) {")
    lines.append("    static std::unordered_map<InterfaceId, std::vector<std::string>> cache;")
    lines.append("    auto it = cache.find(iface);")
    lines.append("    if (it != cache.end()) return it->second;")
    lines.append("    std::vector<std::string> out;")
    lines.append("    for (const auto& [name, ifaces] : table())")
    lines.append("        for (InterfaceId i : ifaces)")
    lines.append("            if (i == iface) { out.push_back(name); break; }")
    lines.append("    std::sort(out.begin(), out.end());")
    lines.append("    return cache.emplace(iface, std::move(out)).first->second;")
    lines.append("}")
    lines.append("")
    lines.append("} // namespace x3d::nodes")
    lines.append("")
    return "\n".join(lines)
