#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DChildNode.hpp"

#include "x3d/nodes/X3DBoundedObject.hpp"

#include "x3d/nodes/X3DGroupingNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class LayoutGroup
 * @brief LayoutGroup is a Grouping node that can contain most nodes, whose
 * children are related by a common layout within a parent layout.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/layout.html#LayoutGroup
 */
class LayoutGroup : public virtual X3DNode, public virtual X3DGroupingNode {
public:
  LayoutGroup() = default;

  ~LayoutGroup() = default;

  static SFNode getDefaultLayout() { return nullptr; }

  static SFNode getDefaultViewport() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Layout"; }

  static int componentLevel() { return 1; }

  /**
   * @details Permitted X3D node types: X3DChildNode
   */
  static const std::vector<std::string> &acceptableAddChildrenNodeTypes() {
    static const std::vector<std::string> types = {"X3DChildNode"};
    return types;
  }

  /**
   * MFNode; access: inputOnly.
   * @details
   *          Dispatches to the handler registered via
   * setOnAddChildrenHandler(); a no-op if none is set.
   */
  void onAddChildren(const MFNode &value) {
    if (_onAddChildrenHandler)
      _onAddChildrenHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnAddChildrenHandler(std::function<void(const MFNode &)> handler) {
    _onAddChildrenHandler = std::move(handler);
  }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details Grouping nodes contain an ordered list of children nodes.
   */
  const MFNode &getChildren() const { return _children; }

  /**
   * @details Permitted X3D node types: X3DChildNode
   */
  static const std::vector<std::string> &acceptableChildrenNodeTypes() {
    static const std::vector<std::string> types = {"X3DChildNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Grouping nodes contain an ordered list of children nodes.
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details The layout field contains an X3DLayoutNode node that provides the
   * information required to locate and size the layout region of the
   * LayoutGroup node relative to its parent’s layout region, and also to scale
   * the contents of the LayoutGroup.
   */
  const SFNode &getLayout() const { return _layout; }

  /**
   * @details Permitted X3D node types: X3DLayoutNode
   */
  static const std::vector<std::string> &acceptableLayoutNodeTypes() {
    static const std::vector<std::string> types = {"X3DLayoutNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The layout field contains an X3DLayoutNode node that provides the
   * information required to locate and size the layout region of the
   * LayoutGroup node relative to its parent’s layout region, and also to scale
   * the contents of the LayoutGroup.
   */
  void setLayout(const SFNode &value) { _layout = value; }

  void setLayout(SFNode &&value) { _layout = std::move(value); }

  /**
   * @details Permitted X3D node types: X3DChildNode
   */
  static const std::vector<std::string> &acceptableRemoveChildrenNodeTypes() {
    static const std::vector<std::string> types = {"X3DChildNode"};
    return types;
  }

  /**
   * MFNode; access: inputOnly.
   * @details
   *          Dispatches to the handler registered via
   * setOnRemoveChildrenHandler(); a no-op if none is set.
   */
  void onRemoveChildren(const MFNode &value) {
    if (_onRemoveChildrenHandler)
      _onRemoveChildrenHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnRemoveChildrenHandler(std::function<void(const MFNode &)> handler) {
    _onRemoveChildrenHandler = std::move(handler);
  }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details The content of the LayoutGroup is clipped by the specified
   * viewport.
   */
  const SFNode &getViewport() const { return _viewport; }

  /**
   * @details Permitted X3D node types: X3DViewportNode
   */
  static const std::vector<std::string> &acceptableViewportNodeTypes() {
    static const std::vector<std::string> types = {"X3DViewportNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The content of the LayoutGroup is clipped by the specified
   * viewport.
   */
  void setViewport(const SFNode &value) { _viewport = value; }

  void setViewport(SFNode &&value) { _viewport = std::move(value); }

  std::string nodeTypeName() const override;

  /**
   * Parent field used when an X3D-XML element omits containerField.
   */
  std::string defaultContainerField() const override;

  /**
   * Own and inherited fields; the table lives for the lifetime of the program.
   */
  const FieldTable &fields() const override;

  void accept(NodeVisitor &visitor) const override;

private:
  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFNode &)> _onAddChildrenHandler{};

  MFNode _children{};

  SFNode _layout{nullptr};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFNode &)> _onRemoveChildrenHandler{};

  SFNode _viewport{nullptr};
};

} // namespace x3d::nodes
