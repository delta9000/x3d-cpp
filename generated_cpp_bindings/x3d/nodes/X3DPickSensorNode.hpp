// X3DPickSensorNode.hpp
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

#include "x3d/nodes/X3DSensorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DPickSensorNode
 * @brief The X3DPickSensorNode abstract node type is the base node type that
 * represents the lowest common denominator of picking capabilities.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/picking.html#X3DPickSensorNode
 */
class X3DPickSensorNode : public virtual X3DSensorNode {
public:
  /**
   * @brief Default constructor for X3DPickSensorNode
   */
  X3DPickSensorNode() = default;

  /**
   * @brief Virtual destructor for X3DPickSensorNode
   */
  virtual ~X3DPickSensorNode() = default;

  /**
   * @brief Get the default value for intersectionType
   * @return SFString The default value
   */
  static SFString getDefaultIntersectionType() { return "BOUNDS"; }

  /**
   * @brief Get the default value for matchCriterion
   * @return PickSensorMatchCriterionChoices The default value
   */
  static PickSensorMatchCriterionChoices getDefaultMatchCriterion() {
    return PickSensorMatchCriterionChoices::MATCH_ANY;
  }

  /**
   * @brief Get the default value for objectType
   * @return MFString The default value
   */
  static MFString getDefaultObjectType() {
    return std::vector<std::string>{"ALL"};
  }

  /**
   * @brief Get the default value for pickingGeometry
   * @return SFNode The default value
   */
  static SFNode getDefaultPickingGeometry() { return nullptr; }

  /**
   * @brief Get the default value for sortOrder
   * @return SFString The default value
   */
  static SFString getDefaultSortOrder() { return "CLOSEST"; }

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
  static std::string componentName() { return "Picking"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of intersectionType. AccessType: initializeOnly
   * @details
   * @return SFString The current value of intersectionType.
   */
  SFString getIntersectionType() const { return _intersectionType; }
  /**
   * @brief Data-layer write of intersectionType (reader/init ingest path).
   * @details intersectionType is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setIntersectionType().
   */
  void setIntersectionTypeUnchecked(const SFString &value) {
    _intersectionType = value;
  }

  /**
   * @brief Gets the value of matchCriterion. AccessType: inputOutput
   * @details
   * @return PickSensorMatchCriterionChoices The current value of
   * matchCriterion.
   */
  PickSensorMatchCriterionChoices getMatchCriterion() const {
    return _matchCriterion;
  }

  /**
   * @brief Sets the value of matchCriterion. AccessType: inputOutput
   * @details
   * @param value The new value for matchCriterion.
   */
  void setMatchCriterion(const PickSensorMatchCriterionChoices &value) {

    _matchCriterion = value;
  }

  /**
   * @brief Gets the value of objectType. AccessType: inputOutput
   * @details
   * @return MFString The current value of objectType.
   */
  MFString getObjectType() const { return _objectType; }

  /**
   * @brief Sets the value of objectType. AccessType: inputOutput
   * @details
   * @param value The new value for objectType.
   */
  void setObjectType(const MFString &value) { _objectType = value; }

  void setObjectType(MFString &&value) { _objectType = std::move(value); }

  /**
   * @brief Gets the value of pickedGeometry. AccessType: outputOnly
   * @details
   * @return MFNode The current value of pickedGeometry.
   */
  MFNode getPickedGeometry() const { return _pickedGeometry; }

  /**
   * @brief Emit an output value on pickedGeometry. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitPickedGeometry(const MFNode &value) { _pickedGeometry = value; }

  /**
   * @brief Acceptable node types for the pickedGeometry field.
   * @details Permitted X3D node types: X3DChildNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptablePickedGeometryNodeTypes() {
    static const std::vector<std::string> types = {"X3DChildNode"};
    return types;
  }

  /**
   * @brief Gets the value of pickingGeometry. AccessType: inputOutput
   * @details
   * @return SFNode The current value of pickingGeometry.
   */
  SFNode getPickingGeometry() const { return _pickingGeometry; }

  /**
   * @brief Acceptable node types for the pickingGeometry field.
   * @details Permitted X3D node types: X3DGeometryNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptablePickingGeometryNodeTypes() {
    static const std::vector<std::string> types = {"X3DGeometryNode"};
    return types;
  }

  /**
   * @brief Sets the value of pickingGeometry. AccessType: inputOutput
   * @details
   * @param value The new value for pickingGeometry.
   */
  void setPickingGeometry(const SFNode &value) { _pickingGeometry = value; }

  void setPickingGeometry(SFNode &&value) {

    _pickingGeometry = std::move(value);
  }

  /**
   * @brief Gets the value of pickTarget. AccessType: inputOutput
   * @details
   * @return MFNode The current value of pickTarget.
   */
  MFNode getPickTarget() const { return _pickTarget; }

  /**
   * @brief Acceptable node types for the pickTarget field.
   * @details Permitted X3D node types: X3DGroupingNode, X3DShapeNode, Inline
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptablePickTargetNodeTypes() {
    static const std::vector<std::string> types = {"X3DGroupingNode",
                                                   "X3DShapeNode", "Inline"};
    return types;
  }

  /**
   * @brief Sets the value of pickTarget. AccessType: inputOutput
   * @details
   * @param value The new value for pickTarget.
   */
  void setPickTarget(const MFNode &value) { _pickTarget = value; }

  void setPickTarget(MFNode &&value) { _pickTarget = std::move(value); }

  /**
   * @brief Gets the value of sortOrder. AccessType: initializeOnly
   * @details
   * @return SFString The current value of sortOrder.
   */
  SFString getSortOrder() const { return _sortOrder; }
  /**
   * @brief Data-layer write of sortOrder (reader/init ingest path).
   * @details sortOrder is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setSortOrder().
   */
  void setSortOrderUnchecked(const SFString &value) { _sortOrder = value; }

  /**
   * @brief The X3D type name of this node (e.g. "X3DPickSensorNode").
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
   * @brief Member variable for intersectionType.
   */

  SFString _intersectionType{"BOUNDS"};

  /**
   * @brief Member variable for matchCriterion.
   */

  PickSensorMatchCriterionChoices _matchCriterion{
      PickSensorMatchCriterionChoices::MATCH_ANY};

  /**
   * @brief Member variable for objectType.
   */

  MFString _objectType{std::vector<std::string>{"ALL"}};

  /**
   * @brief Member variable for pickedGeometry.
   */

  MFNode _pickedGeometry{};

  /**
   * @brief Member variable for pickingGeometry.
   */

  SFNode _pickingGeometry{nullptr};

  /**
   * @brief Member variable for pickTarget.
   */

  MFNode _pickTarget{};

  /**
   * @brief Member variable for sortOrder.
   */

  SFString _sortOrder{"CLOSEST"};
};

} // namespace x3d::nodes
