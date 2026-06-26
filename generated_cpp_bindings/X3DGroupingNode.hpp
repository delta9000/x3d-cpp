// X3DGroupingNode.hpp
#ifndef X3DGROUPINGNODE_HPP
#define X3DGROUPINGNODE_HPP

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
  /**
   * @brief Default constructor for X3DGroupingNode
   */
  X3DGroupingNode() = default;

  /**
   * @brief Virtual destructor for X3DGroupingNode
   */
  virtual ~X3DGroupingNode() = default;

  /**
   * @brief Get the default value for bboxCenter
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultBboxCenter() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for bboxDisplay
   * @return SFBool The default value
   */
  static SFBool getDefaultBboxDisplay() { return false; }

  /**
   * @brief Get the default value for bboxSize
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultBboxSize() { return SFVec3f{-1, -1, -1}; }

  /**
   * @brief Get the default value for visible
   * @return SFBool The default value
   */
  static SFBool getDefaultVisible() { return true; }

  /**
   * @brief Get the default container field value
   * @details No explicit containerField in the spec for this node; falls back
   *          to the X3D default field "children".
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Grouping"; }

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
   * @brief Gets the value of bboxCenter. AccessType: initializeOnly
   * @details
   * @return SFVec3f The current value of bboxCenter.
   */
  SFVec3f getBboxCenter() const { return _bboxCenter; }
  /**
   * @brief Data-layer write of bboxCenter (reader/init ingest path).
   * @details bboxCenter is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setBboxCenter().
   */
  void setBboxCenterUnchecked(const SFVec3f &value) { _bboxCenter = value; }

  /**
   * @brief Gets the value of bboxDisplay. AccessType: inputOutput
   * @details
   * @return SFBool The current value of bboxDisplay.
   */
  SFBool getBboxDisplay() const { return _bboxDisplay; }

  /**
   * @brief Sets the value of bboxDisplay. AccessType: inputOutput
   * @details
   * @param value The new value for bboxDisplay.
   */
  void setBboxDisplay(const SFBool &value) { _bboxDisplay = value; }

  /**
   * @brief Gets the value of bboxSize. AccessType: initializeOnly
   * @details
   * @return SFVec3f The current value of bboxSize.
   */
  SFVec3f getBboxSize() const { return _bboxSize; }
  /**
   * @brief Data-layer write of bboxSize (reader/init ingest path).
   * @details bboxSize is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setBboxSize().
   */
  void setBboxSizeUnchecked(const SFVec3f &value) { _bboxSize = value; }

  /**
   * @brief Gets the value of children. AccessType: inputOutput
   * @details
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
   * @details
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
   * @brief Gets the value of visible. AccessType: inputOutput
   * @details
   * @return SFBool The current value of visible.
   */
  SFBool getVisible() const { return _visible; }

  /**
   * @brief Sets the value of visible. AccessType: inputOutput
   * @details
   * @param value The new value for visible.
   */
  void setVisible(const SFBool &value) { _visible = value; }

  /**
   * @brief The X3D type name of this node (e.g. "X3DGroupingNode").
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
   * @brief Member variable for bboxCenter.
   */

  SFVec3f _bboxCenter{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for bboxDisplay.
   */

  SFBool _bboxDisplay{false};

  /**
   * @brief Member variable for bboxSize.
   */

  SFVec3f _bboxSize{SFVec3f{-1, -1, -1}};

  /**
   * @brief Member variable for children.
   */

  MFNode _children{};

  /**
   * @brief Registered event handler for removeChildren (inputOnly); empty until
   * set.
   */
  std::function<void(const MFNode &)> _onRemoveChildrenHandler{};

  /**
   * @brief Member variable for visible.
   */

  SFBool _visible{true};
};

#endif // X3DGROUPINGNODE_HPP