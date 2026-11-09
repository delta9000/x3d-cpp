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
  X3DViewpointNode() = default;

  virtual ~X3DViewpointNode() = default;

  static SFFloat getDefaultFarDistance() { return -1; }

  static SFBool getDefaultJump() { return true; }

  static SFNode getDefaultNavigationInfo() { return nullptr; }

  static SFFloat getDefaultNearDistance() { return -1; }

  static SFRotation getDefaultOrientation() { return SFRotation{0, 0, 1, 0}; }

  static SFBool getDefaultRetainUserOffsets() { return false; }

  static SFBool getDefaultViewAll() { return false; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Navigation"; }

  static int componentLevel() { return 1; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFString &getDescription() const { return _description; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFFloat &getFarDistance() const { return _farDistance; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setFarDistance(const SFFloat &value) { _farDistance = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getJump() const { return _jump; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setJump(const SFBool &value) { _jump = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFNode &getNavigationInfo() const { return _navigationInfo; }

  /**
   * @details Permitted X3D node types: NavigationInfo
   */
  static const std::vector<std::string> &acceptableNavigationInfoNodeTypes() {
    static const std::vector<std::string> types = {"NavigationInfo"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setNavigationInfo(const SFNode &value) { _navigationInfo = value; }

  void setNavigationInfo(SFNode &&value) { _navigationInfo = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFFloat &getNearDistance() const { return _nearDistance; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setNearDistance(const SFFloat &value) { _nearDistance = value; }

  /**
   * SFRotation; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFRotation &getOrientation() const { return _orientation; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setOrientation(const SFRotation &value) { _orientation = value; }

  void setOrientation(SFRotation &&value) { _orientation = std::move(value); }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getRetainUserOffsets() const { return _retainUserOffsets; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setRetainUserOffsets(const SFBool &value) { _retainUserOffsets = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getViewAll() const { return _viewAll; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setViewAll(const SFBool &value) { _viewAll = value; }

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
  SFString _description{};

  SFFloat _farDistance{-1};

  SFBool _jump{true};

  SFNode _navigationInfo{nullptr};

  SFFloat _nearDistance{-1};

  SFRotation _orientation{SFRotation{0, 0, 1, 0}};

  SFBool _retainUserOffsets{false};

  SFBool _viewAll{false};
};

} // namespace x3d::nodes
