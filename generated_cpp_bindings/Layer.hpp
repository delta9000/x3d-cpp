// Layer.hpp
#ifndef LAYER_HPP
#define LAYER_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DPickableObject.hpp"

#include "X3DLayerNode.hpp"

/**
 * @class Layer
 * @brief Layer contains a list of children nodes that define the contents of
 * the layer.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/layering.html#Layer
 */
class Layer : public virtual X3DLayerNode {
public:
  /**
   * @brief Default constructor for Layer
   */
  Layer() = default;

  /**
   * @brief Destructor for Layer
   */
  ~Layer() = default;

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "layers"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Layering"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Acceptable node types for the addChildren field.
   * @details Permitted X3D node types: X3DChildNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableAddChildrenNodeTypes() {
    static const std::vector<std::string> types = {"X3DChildNode"};
    return types;
  }

  /**
   * @brief Event handler invoked when an event is received on addChildren.
   * AccessType: inputOnly
   * @details
   *          Dispatches to the handler registered via
   * setOnAddChildrenHandler(); a no-op if none is set. The event cascade
   * reaches this through the node's reflected field table, so routing is
   * node-agnostic.
   * @param value The value of the received event.
   */
  void onAddChildren(const MFNode &value) {
    if (_onAddChildrenHandler)
      _onAddChildrenHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on addChildren.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnAddChildrenHandler(std::function<void(const MFNode &)> handler) {
    _onAddChildrenHandler = std::move(handler);
  }

  /**
   * @brief Gets the value of children. AccessType: inputOutput
   * @details Nodes making up this layer.
   * @return MFNode The current value of children.
   */
  MFNode getChildren() const { return _children; }

  /**
   * @brief Acceptable node types for the children field.
   * @details Permitted X3D node types: X3DChildNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableChildrenNodeTypes() {
    static const std::vector<std::string> types = {"X3DChildNode"};
    return types;
  }

  /**
   * @brief Sets the value of children. AccessType: inputOutput
   * @details Nodes making up this layer.
   * @param value The new value for children.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

  /**
   * @brief Acceptable node types for the removeChildren field.
   * @details Permitted X3D node types: X3DChildNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableRemoveChildrenNodeTypes() {
    static const std::vector<std::string> types = {"X3DChildNode"};
    return types;
  }

  /**
   * @brief Event handler invoked when an event is received on removeChildren.
   * AccessType: inputOnly
   * @details
   *          Dispatches to the handler registered via
   * setOnRemoveChildrenHandler(); a no-op if none is set. The event cascade
   * reaches this through the node's reflected field table, so routing is
   * node-agnostic.
   * @param value The value of the received event.
   */
  void onRemoveChildren(const MFNode &value) {
    if (_onRemoveChildrenHandler)
      _onRemoveChildrenHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * removeChildren.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnRemoveChildrenHandler(std::function<void(const MFNode &)> handler) {
    _onRemoveChildrenHandler = std::move(handler);
  }

  /**
   * @brief The X3D type name of this node (e.g. "Layer").
   */
  std::string nodeTypeName() const override;

  /**
   * @brief This node's default containerField: the parent field it attaches
   *        to when an X3D-XML element gives no explicit containerField. Virtual
   *        so codecs can resolve it polymorphically through an X3DNode base
   *        pointer (the static getDefaultContainerField() is not reachable that
   *        way). Mirrors getDefaultContainerField().
   */
  std::string defaultContainerField() const override;

  /**
   * @brief Reflected field table for this node (own + inherited fields).
   * @details Built once (function-local static) from this node's descriptors.
   *          Each FieldInfo carries type-erased get/set thunks bound to this
   *          node's strongly-typed accessors so codecs need no per-node code.
   */
  const FieldTable &fields() const override;

  /**
   * @brief Visitor double-dispatch entry point.
   */
  void accept(NodeVisitor &visitor) const override;

private:
  /**
   * @brief Registered event handler for addChildren (inputOnly); empty until
   * set.
   */
  std::function<void(const MFNode &)> _onAddChildrenHandler{};

  /**
   * @brief Member variable for children.
   */

  MFNode _children{};

  /**
   * @brief Registered event handler for removeChildren (inputOnly); empty until
   * set.
   */
  std::function<void(const MFNode &)> _onRemoveChildrenHandler{};
};

#endif // LAYER_HPP