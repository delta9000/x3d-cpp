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
  X3DPickSensorNode() = default;

  virtual ~X3DPickSensorNode() = default;

  static SFString getDefaultIntersectionType() { return "BOUNDS"; }

  static PickSensorMatchCriterionChoices getDefaultMatchCriterion() {
    return PickSensorMatchCriterionChoices::MATCH_ANY;
  }

  static MFString getDefaultObjectType() {
    return std::vector<std::string>{"ALL"};
  }

  static SFNode getDefaultPickingGeometry() { return nullptr; }

  static SFString getDefaultSortOrder() { return "CLOSEST"; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Picking"; }

  static int componentLevel() { return 1; }

  /**
   * SFString; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFString &getIntersectionType() const { return _intersectionType; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setIntersectionTypeUnchecked(const SFString &value) {
    _intersectionType = value;
  }

  /**
   * PickSensorMatchCriterionChoices; access: inputOutput. Borrows the stored
   * value.
   * @details
   */
  const PickSensorMatchCriterionChoices &getMatchCriterion() const {
    return _matchCriterion;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setMatchCriterion(const PickSensorMatchCriterionChoices &value) {

    _matchCriterion = value;
  }

  /**
   * MFString; access: inputOutput. Borrows the stored value.
   * @details
   */
  const MFString &getObjectType() const { return _objectType; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setObjectType(const MFString &value) { _objectType = value; }

  void setObjectType(MFString &&value) { _objectType = std::move(value); }

  /**
   * MFNode; access: outputOnly. Borrows the stored value.
   * @details
   */
  const MFNode &getPickedGeometry() const { return _pickedGeometry; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitPickedGeometry(const MFNode &value) { _pickedGeometry = value; }

  /**
   * @details Permitted X3D node types: X3DChildNode
   */
  static const std::vector<std::string> &acceptablePickedGeometryNodeTypes() {
    static const std::vector<std::string> types = {"X3DChildNode"};
    return types;
  }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFNode &getPickingGeometry() const { return _pickingGeometry; }

  /**
   * @details Permitted X3D node types: X3DGeometryNode
   */
  static const std::vector<std::string> &acceptablePickingGeometryNodeTypes() {
    static const std::vector<std::string> types = {"X3DGeometryNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setPickingGeometry(const SFNode &value) { _pickingGeometry = value; }

  void setPickingGeometry(SFNode &&value) {

    _pickingGeometry = std::move(value);
  }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details
   */
  const MFNode &getPickTarget() const { return _pickTarget; }

  /**
   * @details Permitted X3D node types: X3DGroupingNode, X3DShapeNode, Inline
   */
  static const std::vector<std::string> &acceptablePickTargetNodeTypes() {
    static const std::vector<std::string> types = {"X3DGroupingNode",
                                                   "X3DShapeNode", "Inline"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setPickTarget(const MFNode &value) { _pickTarget = value; }

  void setPickTarget(MFNode &&value) { _pickTarget = std::move(value); }

  /**
   * SFString; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFString &getSortOrder() const { return _sortOrder; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setSortOrderUnchecked(const SFString &value) { _sortOrder = value; }

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
  SFString _intersectionType{"BOUNDS"};

  PickSensorMatchCriterionChoices _matchCriterion{
      PickSensorMatchCriterionChoices::MATCH_ANY};

  MFString _objectType{std::vector<std::string>{"ALL"}};

  MFNode _pickedGeometry{};

  SFNode _pickingGeometry{nullptr};

  MFNode _pickTarget{};

  SFString _sortOrder{"CLOSEST"};
};

} // namespace x3d::nodes
