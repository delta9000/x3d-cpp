#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DRigidJointNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class BallJoint
 * @brief BallJoint represents an unconstrained joint between two bodies that
 * pivot about a common anchor point.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#BallJoint
 */
class BallJoint : public virtual X3DRigidJointNode {
public:
  BallJoint() = default;

  ~BallJoint() = default;

  static SFVec3f getDefaultAnchorPoint() { return SFVec3f{0, 0, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "joints"; }

  static std::string componentName() { return "RigidBodyPhysics"; }

  static int componentLevel() { return 2; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details anchorPoint is joint center, specified in world coordinates.
   */
  const SFVec3f &getAnchorPoint() const { return _anchorPoint; }

  /**
   * Access: inputOutput.
   * @details anchorPoint is joint center, specified in world coordinates.
   */
  void setAnchorPoint(const SFVec3f &value) { _anchorPoint = value; }

  void setAnchorPoint(SFVec3f &&value) { _anchorPoint = std::move(value); }

  /**
   * SFVec3f; access: outputOnly. Borrows the stored value.
   * @details body1AnchorPoint describes anchorPoint position relative to local
   * coordinate reference frame.
   */
  const SFVec3f &getBody1AnchorPoint() const { return _body1AnchorPoint; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details body1AnchorPoint describes anchorPoint position relative to local
   * coordinate reference frame.
   */
  void emitBody1AnchorPoint(const SFVec3f &value) { _body1AnchorPoint = value; }

  /**
   * SFVec3f; access: outputOnly. Borrows the stored value.
   * @details body2AnchorPoint describes anchorPoint position relative to local
   * coordinate reference frame.
   */
  const SFVec3f &getBody2AnchorPoint() const { return _body2AnchorPoint; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details body2AnchorPoint describes anchorPoint position relative to local
   * coordinate reference frame.
   */
  void emitBody2AnchorPoint(const SFVec3f &value) { _body2AnchorPoint = value; }

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
  SFVec3f _anchorPoint{SFVec3f{0, 0, 0}};

  SFVec3f _body1AnchorPoint{};

  SFVec3f _body2AnchorPoint{};
};

} // namespace x3d::nodes
