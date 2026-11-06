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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DGroupingNode
 * @brief Grouping nodes can contain other nodes as children, thus making up the
 * backbone of a scene graph.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/grouping.html#X3DGroupingNode
 */
class X3DGroupingNode : public virtual X3DChildNode,
                        public virtual X3DBoundedObject {
public:
  X3DGroupingNode() = default;

  virtual ~X3DGroupingNode() = default;

  static SFVec3f getDefaultBboxCenter() { return SFVec3f{0, 0, 0}; }

  static SFBool getDefaultBboxDisplay() { return false; }

  static SFVec3f getDefaultBboxSize() { return SFVec3f{-1, -1, -1}; }

  static SFBool getDefaultVisible() { return true; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Grouping"; }

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
   * SFVec3f; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFVec3f &getBboxCenter() const { return _bboxCenter; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setBboxCenterUnchecked(const SFVec3f &value) { _bboxCenter = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getBboxDisplay() const { return _bboxDisplay; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setBboxDisplay(const SFBool &value) { _bboxDisplay = value; }

  /**
   * SFVec3f; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFVec3f &getBboxSize() const { return _bboxSize; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setBboxSizeUnchecked(const SFVec3f &value) { _bboxSize = value; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details
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
   * @details
   */
  void setChildren(const MFNode &value) { _children = value; }

  void setChildren(MFNode &&value) { _children = std::move(value); }

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
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getVisible() const { return _visible; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setVisible(const SFBool &value) { _visible = value; }

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

  SFVec3f _bboxCenter{SFVec3f{0, 0, 0}};

  SFBool _bboxDisplay{false};

  SFVec3f _bboxSize{SFVec3f{-1, -1, -1}};

  MFNode _children{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFNode &)> _onRemoveChildrenHandler{};

  SFBool _visible{true};
};

} // namespace x3d::nodes
