// DoubleAxisHingeJoint.hpp
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
  /**
   * @brief Default constructor for DoubleAxisHingeJoint
   */
  DoubleAxisHingeJoint() = default;

  /**
   * @brief Destructor for DoubleAxisHingeJoint
   */
  ~DoubleAxisHingeJoint() = default;

  /**
   * @brief Get the default value for anchorPoint
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultAnchorPoint() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for axis1
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultAxis1() { return SFVec3f{1, 0, 0}; }

  /**
   * @brief Get the default value for axis2
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultAxis2() { return SFVec3f{0, 1, 0}; }

  /**
   * @brief Get the default value for desiredAngularVelocity1
   * @return SFFloat The default value
   */
  static SFFloat getDefaultDesiredAngularVelocity1() { return 0; }

  /**
   * @brief Get the default value for desiredAngularVelocity2
   * @return SFFloat The default value
   */
  static SFFloat getDefaultDesiredAngularVelocity2() { return 0; }

  /**
   * @brief Get the default value for maxAngle1
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMaxAngle1() { return 3.141592653; }

  /**
   * @brief Get the default value for maxTorque1
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMaxTorque1() { return 0; }

  /**
   * @brief Get the default value for maxTorque2
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMaxTorque2() { return 0; }

  /**
   * @brief Get the default value for minAngle1
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMinAngle1() { return -3.141592653; }

  /**
   * @brief Get the default value for stop1Bounce
   * @return SFFloat The default value
   */
  static SFFloat getDefaultStop1Bounce() { return 0; }

  /**
   * @brief Get the default value for stop1ConstantForceMix
   * @return SFFloat The default value
   */
  static SFFloat getDefaultStop1ConstantForceMix() { return 0.001; }

  /**
   * @brief Get the default value for stop1ErrorCorrection
   * @return SFFloat The default value
   */
  static SFFloat getDefaultStop1ErrorCorrection() { return 0.8; }

  /**
   * @brief Get the default value for suspensionErrorCorrection
   * @return SFFloat The default value
   */
  static SFFloat getDefaultSuspensionErrorCorrection() { return 0.8; }

  /**
   * @brief Get the default value for suspensionForce
   * @return SFFloat The default value
   */
  static SFFloat getDefaultSuspensionForce() { return 0; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "joints"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "RigidBodyPhysics"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of anchorPoint. AccessType: inputOutput
   * @details anchorPoint is joint center, specified in world coordinates.
   * @return SFVec3f The current value of anchorPoint.
   */
  SFVec3f getAnchorPoint() const { return _anchorPoint; }

  /**
   * @brief Sets the value of anchorPoint. AccessType: inputOutput
   * @details anchorPoint is joint center, specified in world coordinates.
   * @param value The new value for anchorPoint.
   */
  void setAnchorPoint(const SFVec3f &value) { _anchorPoint = value; }

  void setAnchorPoint(SFVec3f &&value) { _anchorPoint = std::move(value); }

  /**
   * @brief Gets the value of axis1. AccessType: inputOutput
   * @details axis1 defines axis vector of joint connection to body1.
   * @return SFVec3f The current value of axis1.
   */
  SFVec3f getAxis1() const { return _axis1; }

  /**
   * @brief Sets the value of axis1. AccessType: inputOutput
   * @details axis1 defines axis vector of joint connection to body1.
   * @param value The new value for axis1.
   */
  void setAxis1(const SFVec3f &value) { _axis1 = value; }

  void setAxis1(SFVec3f &&value) { _axis1 = std::move(value); }

  /**
   * @brief Gets the value of axis2. AccessType: inputOutput
   * @details axis2 defines axis vector of joint connection to body2.
   * @return SFVec3f The current value of axis2.
   */
  SFVec3f getAxis2() const { return _axis2; }

  /**
   * @brief Sets the value of axis2. AccessType: inputOutput
   * @details axis2 defines axis vector of joint connection to body2.
   * @param value The new value for axis2.
   */
  void setAxis2(const SFVec3f &value) { _axis2 = value; }

  void setAxis2(SFVec3f &&value) { _axis2 = std::move(value); }

  /**
   * @brief Gets the value of body1AnchorPoint. AccessType: outputOnly
   * @details body1AnchorPoint describes anchorPoint position relative to local
   * coordinate reference frame.
   * @return SFVec3f The current value of body1AnchorPoint.
   */
  SFVec3f getBody1AnchorPoint() const { return _body1AnchorPoint; }

  /**
   * @brief Emit an output value on body1AnchorPoint. AccessType: outputOnly
   * @details body1AnchorPoint describes anchorPoint position relative to local
   * coordinate reference frame. outputOnly fields have no author-facing setter;
   * a node's behavior or the runtime calls this to produce an output event. The
   * event cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitBody1AnchorPoint(const SFVec3f &value) { _body1AnchorPoint = value; }

  /**
   * @brief Gets the value of body1Axis. AccessType: outputOnly
   * @details body1Axis describes report the current location of the anchor
   * point relative to the corresponding body.
   * @return SFVec3f The current value of body1Axis.
   */
  SFVec3f getBody1Axis() const { return _body1Axis; }

  /**
   * @brief Emit an output value on body1Axis. AccessType: outputOnly
   * @details body1Axis describes report the current location of the anchor
   * point relative to the corresponding body. outputOnly fields have no
   * author-facing setter; a node's behavior or the runtime calls this to
   * produce an output event. The event cascade reaches it through the reflected
   * field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitBody1Axis(const SFVec3f &value) { _body1Axis = value; }

  /**
   * @brief Gets the value of body2AnchorPoint. AccessType: outputOnly
   * @details body2AnchorPoint describes anchorPoint position relative to local
   * coordinate reference frame.
   * @return SFVec3f The current value of body2AnchorPoint.
   */
  SFVec3f getBody2AnchorPoint() const { return _body2AnchorPoint; }

  /**
   * @brief Emit an output value on body2AnchorPoint. AccessType: outputOnly
   * @details body2AnchorPoint describes anchorPoint position relative to local
   * coordinate reference frame. outputOnly fields have no author-facing setter;
   * a node's behavior or the runtime calls this to produce an output event. The
   * event cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitBody2AnchorPoint(const SFVec3f &value) { _body2AnchorPoint = value; }

  /**
   * @brief Gets the value of body2Axis. AccessType: outputOnly
   * @details body1Axis describes report the current location of the anchor
   * point relative to the corresponding body.
   * @return SFVec3f The current value of body2Axis.
   */
  SFVec3f getBody2Axis() const { return _body2Axis; }

  /**
   * @brief Emit an output value on body2Axis. AccessType: outputOnly
   * @details body1Axis describes report the current location of the anchor
   * point relative to the corresponding body. outputOnly fields have no
   * author-facing setter; a node's behavior or the runtime calls this to
   * produce an output event. The event cascade reaches it through the reflected
   * field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitBody2Axis(const SFVec3f &value) { _body2Axis = value; }

  /**
   * @brief Gets the value of desiredAngularVelocity1. AccessType: inputOutput
   * @details desiredAngularVelocity1 is goal rotation rate for hinge connection
   * to body1.
   * @return SFFloat The current value of desiredAngularVelocity1.
   */
  SFFloat getDesiredAngularVelocity1() const {
    return _desiredAngularVelocity1;
  }

  /**
   * @brief Sets the value of desiredAngularVelocity1. AccessType: inputOutput
   * @details desiredAngularVelocity1 is goal rotation rate for hinge connection
   * to body1.
   * @param value The new value for desiredAngularVelocity1.
   */
  void setDesiredAngularVelocity1(const SFFloat &value) {

    _desiredAngularVelocity1 = value;
  }

  /**
   * @brief Gets the value of desiredAngularVelocity2. AccessType: inputOutput
   * @details desiredAngularVelocity2 is goal rotation rate for hinge connection
   * to body2.
   * @return SFFloat The current value of desiredAngularVelocity2.
   */
  SFFloat getDesiredAngularVelocity2() const {
    return _desiredAngularVelocity2;
  }

  /**
   * @brief Sets the value of desiredAngularVelocity2. AccessType: inputOutput
   * @details desiredAngularVelocity2 is goal rotation rate for hinge connection
   * to body2.
   * @param value The new value for desiredAngularVelocity2.
   */
  void setDesiredAngularVelocity2(const SFFloat &value) {

    _desiredAngularVelocity2 = value;
  }

  /**
   * @brief Gets the value of hinge1Angle. AccessType: outputOnly
   * @details
   * @return SFFloat The current value of hinge1Angle.
   */
  SFFloat getHinge1Angle() const { return _hinge1Angle; }

  /**
   * @brief Emit an output value on hinge1Angle. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitHinge1Angle(const SFFloat &value) { _hinge1Angle = value; }

  /**
   * @brief Gets the value of hinge1AngleRate. AccessType: outputOnly
   * @details
   * @return SFFloat The current value of hinge1AngleRate.
   */
  SFFloat getHinge1AngleRate() const { return _hinge1AngleRate; }

  /**
   * @brief Emit an output value on hinge1AngleRate. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitHinge1AngleRate(const SFFloat &value) { _hinge1AngleRate = value; }

  /**
   * @brief Gets the value of hinge2Angle. AccessType: outputOnly
   * @details
   * @return SFFloat The current value of hinge2Angle.
   */
  SFFloat getHinge2Angle() const { return _hinge2Angle; }

  /**
   * @brief Emit an output value on hinge2Angle. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitHinge2Angle(const SFFloat &value) { _hinge2Angle = value; }

  /**
   * @brief Gets the value of hinge2AngleRate. AccessType: outputOnly
   * @details
   * @return SFFloat The current value of hinge2AngleRate.
   */
  SFFloat getHinge2AngleRate() const { return _hinge2AngleRate; }

  /**
   * @brief Emit an output value on hinge2AngleRate. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitHinge2AngleRate(const SFFloat &value) { _hinge2AngleRate = value; }

  /**
   * @brief Gets the value of maxAngle1. AccessType: inputOutput
   * @details maxAngle1 is maximum rotation angle for hinge.
   * @return SFFloat The current value of maxAngle1.
   */
  SFFloat getMaxAngle1() const { return _maxAngle1; }

  /**
   * @brief Sets the value of maxAngle1. AccessType: inputOutput
   * @details maxAngle1 is maximum rotation angle for hinge.
   * @param value The new value for maxAngle1.
   */
  void setMaxAngle1(const SFFloat &value) { _maxAngle1 = value; }

  /**
   * @brief Gets the value of maxTorque1. AccessType: inputOutput
   * @details maxTorque1 is maximum rotational torque applied by corresponding
   * motor axis to achieve desiredAngularVelocity1.
   * @return SFFloat The current value of maxTorque1.
   */
  SFFloat getMaxTorque1() const { return _maxTorque1; }

  /**
   * @brief Sets the value of maxTorque1. AccessType: inputOutput
   * @details maxTorque1 is maximum rotational torque applied by corresponding
   * motor axis to achieve desiredAngularVelocity1.
   * @param value The new value for maxTorque1.
   */
  void setMaxTorque1(const SFFloat &value) { _maxTorque1 = value; }

  /**
   * @brief Gets the value of maxTorque2. AccessType: inputOutput
   * @details maxTorque2 is maximum rotational torque applied by corresponding
   * motor axis to achieve desiredAngularVelocity2.
   * @return SFFloat The current value of maxTorque2.
   */
  SFFloat getMaxTorque2() const { return _maxTorque2; }

  /**
   * @brief Sets the value of maxTorque2. AccessType: inputOutput
   * @details maxTorque2 is maximum rotational torque applied by corresponding
   * motor axis to achieve desiredAngularVelocity2.
   * @param value The new value for maxTorque2.
   */
  void setMaxTorque2(const SFFloat &value) { _maxTorque2 = value; }

  /**
   * @brief Gets the value of minAngle1. AccessType: inputOutput
   * @details minAngle1 is minimum rotation angle for hinge.
   * @return SFFloat The current value of minAngle1.
   */
  SFFloat getMinAngle1() const { return _minAngle1; }

  /**
   * @brief Sets the value of minAngle1. AccessType: inputOutput
   * @details minAngle1 is minimum rotation angle for hinge.
   * @param value The new value for minAngle1.
   */
  void setMinAngle1(const SFFloat &value) { _minAngle1 = value; }

  /**
   * @brief Gets the value of stop1Bounce. AccessType: inputOutput
   * @details stop1Bounce is velocity factor for bounce back once stop point is
   * reached.
   * @return SFFloat The current value of stop1Bounce.
   */
  SFFloat getStop1Bounce() const { return _stop1Bounce; }

  /**
   * @brief Sets the value of stop1Bounce. AccessType: inputOutput
   * @details stop1Bounce is velocity factor for bounce back once stop point is
   * reached.
   * @param value The new value for stop1Bounce.
   */
  void setStop1Bounce(const SFFloat &value) { _stop1Bounce = value; }

  /**
   * @brief Gets the value of stop1ConstantForceMix. AccessType: inputOutput
   * @details stop1ConstantForceMix value applies a constant force value to make
   * colliding surfaces appear to be somewhat soft.
   * @return SFFloat The current value of stop1ConstantForceMix.
   */
  SFFloat getStop1ConstantForceMix() const { return _stop1ConstantForceMix; }

  /**
   * @brief Sets the value of stop1ConstantForceMix. AccessType: inputOutput
   * @details stop1ConstantForceMix value applies a constant force value to make
   * colliding surfaces appear to be somewhat soft.
   * @param value The new value for stop1ConstantForceMix.
   */
  void setStop1ConstantForceMix(const SFFloat &value) {

    _stop1ConstantForceMix = value;
  }

  /**
   * @brief Gets the value of stop1ErrorCorrection. AccessType: inputOutput
   * @details stop1ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   * @return SFFloat The current value of stop1ErrorCorrection.
   */
  SFFloat getStop1ErrorCorrection() const { return _stop1ErrorCorrection; }

  /**
   * @brief Sets the value of stop1ErrorCorrection. AccessType: inputOutput
   * @details stop1ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   * @param value The new value for stop1ErrorCorrection.
   */
  void setStop1ErrorCorrection(const SFFloat &value) {

    _stop1ErrorCorrection = value;
  }

  /**
   * @brief Gets the value of suspensionErrorCorrection. AccessType: inputOutput
   * @details suspensionErrorCorrection describes how quickly the system
   * resolves intersection errors due to floating-point inaccuracies.
   * @return SFFloat The current value of suspensionErrorCorrection.
   */
  SFFloat getSuspensionErrorCorrection() const {
    return _suspensionErrorCorrection;
  }

  /**
   * @brief Sets the value of suspensionErrorCorrection. AccessType: inputOutput
   * @details suspensionErrorCorrection describes how quickly the system
   * resolves intersection errors due to floating-point inaccuracies.
   * @param value The new value for suspensionErrorCorrection.
   */
  void setSuspensionErrorCorrection(const SFFloat &value) {

    _suspensionErrorCorrection = value;
  }

  /**
   * @brief Gets the value of suspensionForce. AccessType: inputOutput
   * @details suspensionForce describes how quickly the system resolves
   * intersection errors due to floating-point inaccuracies.
   * @return SFFloat The current value of suspensionForce.
   */
  SFFloat getSuspensionForce() const { return _suspensionForce; }

  /**
   * @brief Sets the value of suspensionForce. AccessType: inputOutput
   * @details suspensionForce describes how quickly the system resolves
   * intersection errors due to floating-point inaccuracies.
   * @param value The new value for suspensionForce.
   */
  void setSuspensionForce(const SFFloat &value) { _suspensionForce = value; }

  /**
   * @brief The X3D type name of this node (e.g. "DoubleAxisHingeJoint").
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
   * @brief Member variable for anchorPoint.
   */

  SFVec3f _anchorPoint{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for axis1.
   */

  SFVec3f _axis1{SFVec3f{1, 0, 0}};

  /**
   * @brief Member variable for axis2.
   */

  SFVec3f _axis2{SFVec3f{0, 1, 0}};

  /**
   * @brief Member variable for body1AnchorPoint.
   */

  SFVec3f _body1AnchorPoint{};

  /**
   * @brief Member variable for body1Axis.
   */

  SFVec3f _body1Axis{};

  /**
   * @brief Member variable for body2AnchorPoint.
   */

  SFVec3f _body2AnchorPoint{};

  /**
   * @brief Member variable for body2Axis.
   */

  SFVec3f _body2Axis{};

  /**
   * @brief Member variable for desiredAngularVelocity1.
   */

  SFFloat _desiredAngularVelocity1{0};

  /**
   * @brief Member variable for desiredAngularVelocity2.
   */

  SFFloat _desiredAngularVelocity2{0};

  /**
   * @brief Member variable for hinge1Angle.
   */

  SFFloat _hinge1Angle{};

  /**
   * @brief Member variable for hinge1AngleRate.
   */

  SFFloat _hinge1AngleRate{};

  /**
   * @brief Member variable for hinge2Angle.
   */

  SFFloat _hinge2Angle{};

  /**
   * @brief Member variable for hinge2AngleRate.
   */

  SFFloat _hinge2AngleRate{};

  /**
   * @brief Member variable for maxAngle1.
   */

  SFFloat _maxAngle1{3.141592653};

  /**
   * @brief Member variable for maxTorque1.
   */

  SFFloat _maxTorque1{0};

  /**
   * @brief Member variable for maxTorque2.
   */

  SFFloat _maxTorque2{0};

  /**
   * @brief Member variable for minAngle1.
   */

  SFFloat _minAngle1{-3.141592653};

  /**
   * @brief Member variable for stop1Bounce.
   */

  SFFloat _stop1Bounce{0};

  /**
   * @brief Member variable for stop1ConstantForceMix.
   */

  SFFloat _stop1ConstantForceMix{0.001};

  /**
   * @brief Member variable for stop1ErrorCorrection.
   */

  SFFloat _stop1ErrorCorrection{0.8};

  /**
   * @brief Member variable for suspensionErrorCorrection.
   */

  SFFloat _suspensionErrorCorrection{0.8};

  /**
   * @brief Member variable for suspensionForce.
   */

  SFFloat _suspensionForce{0};
};

} // namespace x3d::nodes
