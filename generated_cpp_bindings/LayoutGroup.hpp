// LayoutGroup.hpp
#ifndef LAYOUTGROUP_HPP
#define LAYOUTGROUP_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DBoundedObject.hpp"

#include "X3DGroupingNode.hpp"

/**
 * @class LayoutGroup
 * @brief LayoutGroup is a Grouping node that can contain most nodes, whose
 * children are related by a common layout within a parent layout.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/layout.html#LayoutGroup
 */
class LayoutGroup : public virtual X3DNode, public virtual X3DGroupingNode {
public:
  /**
   * @brief Default constructor for LayoutGroup
   */
  LayoutGroup() = default;

  /**
   * @brief Destructor for LayoutGroup
   */
  ~LayoutGroup() = default;

  /**
   * @brief Get the default value for layout
   * @return SFNode The default value
   */
  static SFNode getDefaultLayout() { return nullptr; }

  /**
   * @brief Get the default value for viewport
   * @return SFNode The default value
   */
  static SFNode getDefaultViewport() { return nullptr; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Layout"; }

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
   * @details Grouping nodes contain an ordered list of children nodes.
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
   * @details Grouping nodes contain an ordered list of children nodes.
   * @param value The new value for children.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

  /**
   * @brief Gets the value of layout. AccessType: inputOutput
   * @details The layout field contains an X3DLayoutNode node that provides the
   * information required to locate and size the layout region of the
   * LayoutGroup node relative to its parent’s layout region, and also to scale
   * the contents of the LayoutGroup.
   * @return SFNode The current value of layout.
   */
  SFNode getLayout() const { return _layout; }

  /**
   * @brief Acceptable node types for the layout field.
   * @details Permitted X3D node types: X3DLayoutNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableLayoutNodeTypes() {
    static const std::vector<std::string> types = {"X3DLayoutNode"};
    return types;
  }

  /**
   * @brief Sets the value of layout. AccessType: inputOutput
   * @details The layout field contains an X3DLayoutNode node that provides the
   * information required to locate and size the layout region of the
   * LayoutGroup node relative to its parent’s layout region, and also to scale
   * the contents of the LayoutGroup.
   * @param value The new value for layout.
   */
  void setLayout(const SFNode &value) { _layout = value; }

  void setLayout(SFNode &&value) { _layout = std::move(value); }

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
   * @brief Gets the value of viewport. AccessType: inputOutput
   * @details The content of the LayoutGroup is clipped by the specified
   * viewport.
   * @return SFNode The current value of viewport.
   */
  SFNode getViewport() const { return _viewport; }

  /**
   * @brief Acceptable node types for the viewport field.
   * @details Permitted X3D node types: X3DViewportNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableViewportNodeTypes() {
    static const std::vector<std::string> types = {"X3DViewportNode"};
    return types;
  }

  /**
   * @brief Sets the value of viewport. AccessType: inputOutput
   * @details The content of the LayoutGroup is clipped by the specified
   * viewport.
   * @param value The new value for viewport.
   */
  void setViewport(const SFNode &value) { _viewport = value; }

  void setViewport(SFNode &&value) { _viewport = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "LayoutGroup").
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
   * @brief Member variable for layout.
   */

  SFNode _layout{nullptr};

  /**
   * @brief Registered event handler for removeChildren (inputOnly); empty until
   * set.
   */
  std::function<void(const MFNode &)> _onRemoveChildrenHandler{};

  /**
   * @brief Member variable for viewport.
   */

  SFNode _viewport{nullptr};
};

#endif // LAYOUTGROUP_HPP