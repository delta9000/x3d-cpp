// X3DViewpointNode.hpp
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

#include "x3d/nodes/X3DBindableNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DViewpointNode
 * @brief Node type X3DViewpointNode defines a specific location in the local
 * coordinate system from which the user may view the scene, and also defines a
 * viewpoint binding stack.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/navigation.html#X3DViewpointNode
 */
class X3DViewpointNode : public virtual X3DBindableNode {
public:
  /**
   * @brief Default constructor for X3DViewpointNode
   */
  X3DViewpointNode() = default;

  /**
   * @brief Virtual destructor for X3DViewpointNode
   */
  virtual ~X3DViewpointNode() = default;

  /**
   * @brief Get the default value for farDistance
   * @return SFFloat The default value
   */
  static SFFloat getDefaultFarDistance() { return -1; }

  /**
   * @brief Get the default value for jump
   * @return SFBool The default value
   */
  static SFBool getDefaultJump() { return true; }

  /**
   * @brief Get the default value for navigationInfo
   * @return SFNode The default value
   */
  static SFNode getDefaultNavigationInfo() { return nullptr; }

  /**
   * @brief Get the default value for nearDistance
   * @return SFFloat The default value
   */
  static SFFloat getDefaultNearDistance() { return -1; }

  /**
   * @brief Get the default value for orientation
   * @return SFRotation The default value
   */
  static SFRotation getDefaultOrientation() { return SFRotation{0, 0, 1, 0}; }

  /**
   * @brief Get the default value for retainUserOffsets
   * @return SFBool The default value
   */
  static SFBool getDefaultRetainUserOffsets() { return false; }

  /**
   * @brief Get the default value for viewAll
   * @return SFBool The default value
   */
  static SFBool getDefaultViewAll() { return false; }

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
  static std::string componentName() { return "Navigation"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of description. AccessType: inputOutput
   * @details
   * @return SFString The current value of description.
   */
  SFString getDescription() const { return _description; }

  /**
   * @brief Sets the value of description. AccessType: inputOutput
   * @details
   * @param value The new value for description.
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

  /**
   * @brief Gets the value of farDistance. AccessType: inputOutput
   * @details
   * @return SFFloat The current value of farDistance.
   */
  SFFloat getFarDistance() const { return _farDistance; }

  /**
   * @brief Sets the value of farDistance. AccessType: inputOutput
   * @details
   * @param value The new value for farDistance.
   */
  void setFarDistance(const SFFloat &value) { _farDistance = value; }

  /**
   * @brief Gets the value of jump. AccessType: inputOutput
   * @details
   * @return SFBool The current value of jump.
   */
  SFBool getJump() const { return _jump; }

  /**
   * @brief Sets the value of jump. AccessType: inputOutput
   * @details
   * @param value The new value for jump.
   */
  void setJump(const SFBool &value) { _jump = value; }

  /**
   * @brief Gets the value of navigationInfo. AccessType: inputOutput
   * @details
   * @return SFNode The current value of navigationInfo.
   */
  SFNode getNavigationInfo() const { return _navigationInfo; }

  /**
   * @brief Acceptable node types for the navigationInfo field.
   * @details Permitted X3D node types: NavigationInfo
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableNavigationInfoNodeTypes() {
    static const std::vector<std::string> types = {"NavigationInfo"};
    return types;
  }

  /**
   * @brief Sets the value of navigationInfo. AccessType: inputOutput
   * @details
   * @param value The new value for navigationInfo.
   */
  void setNavigationInfo(const SFNode &value) { _navigationInfo = value; }

  void setNavigationInfo(SFNode &&value) { _navigationInfo = std::move(value); }

  /**
   * @brief Gets the value of nearDistance. AccessType: inputOutput
   * @details
   * @return SFFloat The current value of nearDistance.
   */
  SFFloat getNearDistance() const { return _nearDistance; }

  /**
   * @brief Sets the value of nearDistance. AccessType: inputOutput
   * @details
   * @param value The new value for nearDistance.
   */
  void setNearDistance(const SFFloat &value) { _nearDistance = value; }

  /**
   * @brief Gets the value of orientation. AccessType: inputOutput
   * @details
   * @return SFRotation The current value of orientation.
   */
  SFRotation getOrientation() const { return _orientation; }

  /**
   * @brief Sets the value of orientation. AccessType: inputOutput
   * @details
   * @param value The new value for orientation.
   */
  void setOrientation(const SFRotation &value) { _orientation = value; }

  void setOrientation(SFRotation &&value) { _orientation = std::move(value); }

  /**
   * @brief Gets the value of retainUserOffsets. AccessType: inputOutput
   * @details
   * @return SFBool The current value of retainUserOffsets.
   */
  SFBool getRetainUserOffsets() const { return _retainUserOffsets; }

  /**
   * @brief Sets the value of retainUserOffsets. AccessType: inputOutput
   * @details
   * @param value The new value for retainUserOffsets.
   */
  void setRetainUserOffsets(const SFBool &value) { _retainUserOffsets = value; }

  /**
   * @brief Gets the value of viewAll. AccessType: inputOutput
   * @details
   * @return SFBool The current value of viewAll.
   */
  SFBool getViewAll() const { return _viewAll; }

  /**
   * @brief Sets the value of viewAll. AccessType: inputOutput
   * @details
   * @param value The new value for viewAll.
   */
  void setViewAll(const SFBool &value) { _viewAll = value; }

  /**
   * @brief The X3D type name of this node (e.g. "X3DViewpointNode").
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
   * @brief Member variable for description.
   */

  SFString _description{};

  /**
   * @brief Member variable for farDistance.
   */

  SFFloat _farDistance{-1};

  /**
   * @brief Member variable for jump.
   */

  SFBool _jump{true};

  /**
   * @brief Member variable for navigationInfo.
   */

  SFNode _navigationInfo{nullptr};

  /**
   * @brief Member variable for nearDistance.
   */

  SFFloat _nearDistance{-1};

  /**
   * @brief Member variable for orientation.
   */

  SFRotation _orientation{SFRotation{0, 0, 1, 0}};

  /**
   * @brief Member variable for retainUserOffsets.
   */

  SFBool _retainUserOffsets{false};

  /**
   * @brief Member variable for viewAll.
   */

  SFBool _viewAll{false};
};

} // namespace x3d::nodes
