"""Generation of ``X3DNodeFactory.hpp``: a name -> node instance registry.

A codec parsing an X3D document reads an element name (the node type) and needs
to instantiate the matching C++ node. The factory maps each CONCRETE node's X3D
type name to a ``std::shared_ptr<X3DNode>`` builder. Abstract node types and the
mixin object-types are intentionally absent — they are never instantiated
directly.

The factory is a small singleton-backed free function ``createX3DNode(name)``
returning ``nullptr`` for an unknown name, plus ``X3DNodeFactory::registry()``
exposing the full map for callers that want to enumerate creatable types.
"""

from typing import Dict, List

from x3d_cpp_gen.parser import X3DNode


def gen_node_factory_header(nodes: Dict[str, X3DNode]) -> str:
    """Render the declarations-only ``X3DNodeFactory.hpp`` (no node includes)."""
    lines: List[str] = []
    lines.append("// X3DNodeFactory.hpp")
    lines.append("// Auto-generated: maps an X3D node-type name to a fresh node instance.")
    lines.append("#ifndef X3D_NODE_FACTORY_HPP")
    lines.append("#define X3D_NODE_FACTORY_HPP")
    lines.append("")
    lines.append("#include <functional>")
    lines.append("#include <memory>")
    lines.append("#include <string>")
    lines.append("#include <unordered_map>")
    lines.append("")
    lines.append("class X3DNode;")
    lines.append("")
    lines.append("/**")
    lines.append(" * @brief Registry of concrete X3D node constructors, keyed by X3D type name.")
    lines.append(" * @details Definitions live in X3DNodeFactory.cpp (compiled into the node")
    lines.append(" *          library) so consumers do not parse every node header.")
    lines.append(" */")
    lines.append("class X3DNodeFactory {")
    lines.append("public:")
    lines.append("    using Creator = std::function<std::shared_ptr<X3DNode>()>;")
    lines.append("    /// The full name -> creator map (built once).")
    lines.append("    static const std::unordered_map<std::string, Creator>& registry();")
    lines.append("    /// Create a node by X3D type name, or nullptr if the name is unknown.")
    lines.append("    static std::shared_ptr<X3DNode> create(const std::string& typeName);")
    lines.append("};")
    lines.append("")
    lines.append("/// Convenience free function mirroring X3DNodeFactory::create.")
    lines.append("std::shared_ptr<X3DNode> createX3DNode(const std::string& typeName);")
    lines.append("")
    lines.append("#endif // X3D_NODE_FACTORY_HPP")
    lines.append("")
    return "\n".join(lines)


def gen_node_factory_source(nodes: Dict[str, X3DNode]) -> str:
    """Render ``X3DNodeFactory.cpp``: includes concrete nodes + defines the registry."""
    concrete: List[str] = sorted(
        n.name for n in nodes.values() if not n.is_abstract
    )
    lines: List[str] = []
    lines.append("// X3DNodeFactory.cpp")
    lines.append("// Auto-generated: the registry definition (compiled once into the node lib).")
    lines.append('#include "X3DNodeFactory.hpp"')
    lines.append("")
    lines.append('#include "X3DNode.hpp"')
    for name in concrete:
        lines.append(f'#include "{name}.hpp"')
    lines.append("")
    lines.append("const std::unordered_map<std::string, X3DNodeFactory::Creator>&")
    lines.append("X3DNodeFactory::registry() {")
    lines.append("    static const std::unordered_map<std::string, Creator> reg = {")
    for name in concrete:
        lines.append(
            f'        {{"{name}", [] {{ return std::make_shared<{name}>(); }}}},'
        )
    lines.append("    };")
    lines.append("    return reg;")
    lines.append("}")
    lines.append("")
    lines.append("std::shared_ptr<X3DNode> X3DNodeFactory::create(const std::string& typeName) {")
    lines.append("    const auto& reg = registry();")
    lines.append("    auto it = reg.find(typeName);")
    lines.append("    return it == reg.end() ? nullptr : it->second();")
    lines.append("}")
    lines.append("")
    lines.append("std::shared_ptr<X3DNode> createX3DNode(const std::string& typeName) {")
    lines.append("    return X3DNodeFactory::create(typeName);")
    lines.append("}")
    lines.append("")
    return "\n".join(lines)
