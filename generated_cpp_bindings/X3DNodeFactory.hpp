// X3DNodeFactory.hpp
// Auto-generated: maps an X3D node-type name to a fresh node instance.
#ifndef X3D_NODE_FACTORY_HPP
#define X3D_NODE_FACTORY_HPP

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class X3DNode;

/**
 * @brief Registry of concrete X3D node constructors, keyed by X3D type name.
 * @details Definitions live in X3DNodeFactory.cpp (compiled into the node
 *          library) so consumers do not parse every node header.
 */
class X3DNodeFactory {
public:
    using Creator = std::function<std::shared_ptr<X3DNode>()>;
    /// The full name -> creator map (built once).
    static const std::unordered_map<std::string, Creator>& registry();
    /// Create a node by X3D type name, or nullptr if the name is unknown.
    static std::shared_ptr<X3DNode> create(const std::string& typeName);
};

/// Convenience free function mirroring X3DNodeFactory::create.
std::shared_ptr<X3DNode> createX3DNode(const std::string& typeName);

#endif // X3D_NODE_FACTORY_HPP
