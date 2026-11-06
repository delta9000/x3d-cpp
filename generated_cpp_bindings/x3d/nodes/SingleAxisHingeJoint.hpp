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
 * @class SingleAxisHingeJoint
 * @brief SingleAxisHingeJoint has single axis about which to rotate, similar to
 * a traditional door hinge. Contains two RigidBody nodes (containerField values
 * body1, body2).
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#SingleAxisHingeJoint
 */
class SingleAxisHingeJoint : public virtual X3DRigidJointNode {
public:
  SingleAxisHingeJoint() = default;

  ~SingleAxisHingeJoint() = default;

  static SFVec3f getDefaultAnchorPoint() { return SFVec3f{0, 0, 0}; }

  static SFVec3f getDefaultAxis() { return SFVec3f{0, 1, 0}; }

  static SFFloat getDefaultMaxAngle() { return 3.141592653; }

  static SFFloat getDefaultMinAngle() { return -3.141592653; }

  static SFFloat getDefaultStopBounce() { return 0; }

  static SFFloat getDefaultStopErrorCorrection() { return 0.8; }

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
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFFloat &getAngle() const { return _angle; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitAngle(const SFFloat &value) { _angle = value; }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFFloat &getAngleRate() const { return _angleRate; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitAngleRate(const SFFloat &value) { _angleRate = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details axis defines vector of joint connection between body1 and body2.
   */
  const SFVec3f &getAxis() const { return _axis; }

  /**
   * Access: inputOutput.
   * @details axis defines vector of joint connection between body1 and body2.
   */
  void setAxis(const SFVec3f &value) { _axis = value; }

  void setAxis(SFVec3f &&value) { _axis = std::move(value); }

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

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details maxAngle is maximum rotation angle for hinge.
   */
  const SFFloat &getMaxAngle() const { return _maxAngle; }

  /**
   * Access: inputOutput.
   * @details maxAngle is maximum rotation angle for hinge.
   */
  void setMaxAngle(const SFFloat &value) { _maxAngle = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details minAngle is minimum rotation angle for hinge.
   */
  const SFFloat &getMinAngle() const { return _minAngle; }

  /**
   * Access: inputOutput.
   * @details minAngle is minimum rotation angle for hinge.
   */
  void setMinAngle(const SFFloat &value) { _minAngle = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details stopBounce is velocity factor for bounce back once stop point is
   * reached.
   */
  const SFFloat &getStopBounce() const { return _stopBounce; }

  /**
   * Access: inputOutput.
   * @details stopBounce is velocity factor for bounce back once stop point is
   * reached.
   */
  void setStopBounce(const SFFloat &value) { _stopBounce = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details stopErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   */
  const SFFloat &getStopErrorCorrection() const { return _stopErrorCorrection; }

  /**
   * Access: inputOutput.
   * @details stopErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   */
  void setStopErrorCorrection(const SFFloat &value) {

    _stopErrorCorrection = value;
  }

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

  SFFloat _angle{};

  SFFloat _angleRate{};

  SFVec3f _axis{SFVec3f{0, 1, 0}};

  SFVec3f _body1AnchorPoint{};

  SFVec3f _body2AnchorPoint{};

  SFFloat _maxAngle{3.141592653};

  SFFloat _minAngle{-3.141592653};

  SFFloat _stopBounce{0};

  SFFloat _stopErrorCorrection{0.8};
};

} // namespace x3d::nodes
