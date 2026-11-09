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
 * @class DoubleAxisHingeJoint
 * @brief DoubleAxisHingeJoint has two independent axes located around a common
 * anchor point.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#DoubleAxisHingeJoint
 */
class DoubleAxisHingeJoint : public virtual X3DRigidJointNode {
public:
  DoubleAxisHingeJoint() = default;

  ~DoubleAxisHingeJoint() = default;

  static SFVec3f getDefaultAnchorPoint() { return SFVec3f{0, 0, 0}; }

  static SFVec3f getDefaultAxis1() { return SFVec3f{1, 0, 0}; }

  static SFVec3f getDefaultAxis2() { return SFVec3f{0, 1, 0}; }

  static SFFloat getDefaultDesiredAngularVelocity1() { return 0; }

  static SFFloat getDefaultDesiredAngularVelocity2() { return 0; }

  static SFFloat getDefaultMaxAngle1() { return 3.141592653; }

  static SFFloat getDefaultMaxTorque1() { return 0; }

  static SFFloat getDefaultMaxTorque2() { return 0; }

  static SFFloat getDefaultMinAngle1() { return -3.141592653; }

  static SFFloat getDefaultStop1Bounce() { return 0; }

  static SFFloat getDefaultStop1ConstantForceMix() { return 0.001; }

  static SFFloat getDefaultStop1ErrorCorrection() { return 0.8; }

  static SFFloat getDefaultSuspensionErrorCorrection() { return 0.8; }

  static SFFloat getDefaultSuspensionForce() { return 0; }

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
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details axis1 defines axis vector of joint connection to body1.
   */
  const SFVec3f &getAxis1() const { return _axis1; }

  /**
   * Access: inputOutput.
   * @details axis1 defines axis vector of joint connection to body1.
   */
  void setAxis1(const SFVec3f &value) { _axis1 = value; }

  void setAxis1(SFVec3f &&value) { _axis1 = std::move(value); }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details axis2 defines axis vector of joint connection to body2.
   */
  const SFVec3f &getAxis2() const { return _axis2; }

  /**
   * Access: inputOutput.
   * @details axis2 defines axis vector of joint connection to body2.
   */
  void setAxis2(const SFVec3f &value) { _axis2 = value; }

  void setAxis2(SFVec3f &&value) { _axis2 = std::move(value); }

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
   * @details body1Axis describes report the current location of the anchor
   * point relative to the corresponding body.
   */
  const SFVec3f &getBody1Axis() const { return _body1Axis; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details body1Axis describes report the current location of the anchor
   * point relative to the corresponding body.
   */
  void emitBody1Axis(const SFVec3f &value) { _body1Axis = value; }

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
   * SFVec3f; access: outputOnly. Borrows the stored value.
   * @details body1Axis describes report the current location of the anchor
   * point relative to the corresponding body.
   */
  const SFVec3f &getBody2Axis() const { return _body2Axis; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details body1Axis describes report the current location of the anchor
   * point relative to the corresponding body.
   */
  void emitBody2Axis(const SFVec3f &value) { _body2Axis = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details desiredAngularVelocity1 is goal rotation rate for hinge connection
   * to body1.
   */
  const SFFloat &getDesiredAngularVelocity1() const {
    return _desiredAngularVelocity1;
  }

  /**
   * Access: inputOutput.
   * @details desiredAngularVelocity1 is goal rotation rate for hinge connection
   * to body1.
   */
  void setDesiredAngularVelocity1(const SFFloat &value) {

    _desiredAngularVelocity1 = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details desiredAngularVelocity2 is goal rotation rate for hinge connection
   * to body2.
   */
  const SFFloat &getDesiredAngularVelocity2() const {
    return _desiredAngularVelocity2;
  }

  /**
   * Access: inputOutput.
   * @details desiredAngularVelocity2 is goal rotation rate for hinge connection
   * to body2.
   */
  void setDesiredAngularVelocity2(const SFFloat &value) {

    _desiredAngularVelocity2 = value;
  }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFFloat &getHinge1Angle() const { return _hinge1Angle; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitHinge1Angle(const SFFloat &value) { _hinge1Angle = value; }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFFloat &getHinge1AngleRate() const { return _hinge1AngleRate; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitHinge1AngleRate(const SFFloat &value) { _hinge1AngleRate = value; }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFFloat &getHinge2Angle() const { return _hinge2Angle; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitHinge2Angle(const SFFloat &value) { _hinge2Angle = value; }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFFloat &getHinge2AngleRate() const { return _hinge2AngleRate; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitHinge2AngleRate(const SFFloat &value) { _hinge2AngleRate = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details maxAngle1 is maximum rotation angle for hinge.
   */
  const SFFloat &getMaxAngle1() const { return _maxAngle1; }

  /**
   * Access: inputOutput.
   * @details maxAngle1 is maximum rotation angle for hinge.
   */
  void setMaxAngle1(const SFFloat &value) { _maxAngle1 = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details maxTorque1 is maximum rotational torque applied by corresponding
   * motor axis to achieve desiredAngularVelocity1.
   */
  const SFFloat &getMaxTorque1() const { return _maxTorque1; }

  /**
   * Access: inputOutput.
   * @details maxTorque1 is maximum rotational torque applied by corresponding
   * motor axis to achieve desiredAngularVelocity1.
   */
  void setMaxTorque1(const SFFloat &value) { _maxTorque1 = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details maxTorque2 is maximum rotational torque applied by corresponding
   * motor axis to achieve desiredAngularVelocity2.
   */
  const SFFloat &getMaxTorque2() const { return _maxTorque2; }

  /**
   * Access: inputOutput.
   * @details maxTorque2 is maximum rotational torque applied by corresponding
   * motor axis to achieve desiredAngularVelocity2.
   */
  void setMaxTorque2(const SFFloat &value) { _maxTorque2 = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details minAngle1 is minimum rotation angle for hinge.
   */
  const SFFloat &getMinAngle1() const { return _minAngle1; }

  /**
   * Access: inputOutput.
   * @details minAngle1 is minimum rotation angle for hinge.
   */
  void setMinAngle1(const SFFloat &value) { _minAngle1 = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details stop1Bounce is velocity factor for bounce back once stop point is
   * reached.
   */
  const SFFloat &getStop1Bounce() const { return _stop1Bounce; }

  /**
   * Access: inputOutput.
   * @details stop1Bounce is velocity factor for bounce back once stop point is
   * reached.
   */
  void setStop1Bounce(const SFFloat &value) { _stop1Bounce = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details stop1ConstantForceMix value applies a constant force value to make
   * colliding surfaces appear to be somewhat soft.
   */
  const SFFloat &getStop1ConstantForceMix() const {
    return _stop1ConstantForceMix;
  }

  /**
   * Access: inputOutput.
   * @details stop1ConstantForceMix value applies a constant force value to make
   * colliding surfaces appear to be somewhat soft.
   */
  void setStop1ConstantForceMix(const SFFloat &value) {

    _stop1ConstantForceMix = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details stop1ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   */
  const SFFloat &getStop1ErrorCorrection() const {
    return _stop1ErrorCorrection;
  }

  /**
   * Access: inputOutput.
   * @details stop1ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   */
  void setStop1ErrorCorrection(const SFFloat &value) {

    _stop1ErrorCorrection = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details suspensionErrorCorrection describes how quickly the system
   * resolves intersection errors due to floating-point inaccuracies.
   */
  const SFFloat &getSuspensionErrorCorrection() const {
    return _suspensionErrorCorrection;
  }

  /**
   * Access: inputOutput.
   * @details suspensionErrorCorrection describes how quickly the system
   * resolves intersection errors due to floating-point inaccuracies.
   */
  void setSuspensionErrorCorrection(const SFFloat &value) {

    _suspensionErrorCorrection = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details suspensionForce describes how quickly the system resolves
   * intersection errors due to floating-point inaccuracies.
   */
  const SFFloat &getSuspensionForce() const { return _suspensionForce; }

  /**
   * Access: inputOutput.
   * @details suspensionForce describes how quickly the system resolves
   * intersection errors due to floating-point inaccuracies.
   */
  void setSuspensionForce(const SFFloat &value) { _suspensionForce = value; }

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

  SFVec3f _axis1{SFVec3f{1, 0, 0}};

  SFVec3f _axis2{SFVec3f{0, 1, 0}};

  SFVec3f _body1AnchorPoint{};

  SFVec3f _body1Axis{};

  SFVec3f _body2AnchorPoint{};

  SFVec3f _body2Axis{};

  SFFloat _desiredAngularVelocity1{0};

  SFFloat _desiredAngularVelocity2{0};

  SFFloat _hinge1Angle{};

  SFFloat _hinge1AngleRate{};

  SFFloat _hinge2Angle{};

  SFFloat _hinge2AngleRate{};

  SFFloat _maxAngle1{3.141592653};

  SFFloat _maxTorque1{0};

  SFFloat _maxTorque2{0};

  SFFloat _minAngle1{-3.141592653};

  SFFloat _stop1Bounce{0};

  SFFloat _stop1ConstantForceMix{0.001};

  SFFloat _stop1ErrorCorrection{0.8};

  SFFloat _suspensionErrorCorrection{0.8};

  SFFloat _suspensionForce{0};
};

} // namespace x3d::nodes
