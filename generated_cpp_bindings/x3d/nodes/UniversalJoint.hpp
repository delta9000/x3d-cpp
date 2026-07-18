// UniversalJoint.hpp
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
 * @class UniversalJoint
 * @brief UniversalJoint is like a BallJoint that constrains an extra degree of
 * rotational freedom.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#UniversalJoint
 */
class UniversalJoint : public virtual X3DRigidJointNode {
public:
  /**
   * @brief Default constructor for UniversalJoint
   */
  UniversalJoint() = default;

  /**
   * @brief Destructor for UniversalJoint
   */
  ~UniversalJoint() = default;

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
   * @brief Get the default value for stop1Bounce
   * @return SFFloat The default value
   */
  static SFFloat getDefaultStop1Bounce() { return 0; }

  /**
   * @brief Get the default value for stop1ErrorCorrection
   * @return SFFloat The default value
   */
  static SFFloat getDefaultStop1ErrorCorrection() { return 0.8; }

  /**
   * @brief Get the default value for stop2Bounce
   * @return SFFloat The default value
   */
  static SFFloat getDefaultStop2Bounce() { return 0; }

  /**
   * @brief Get the default value for stop2ErrorCorrection
   * @return SFFloat The default value
   */
  static SFFloat getDefaultStop2ErrorCorrection() { return 0.8; }

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
  void setStop1Bounce(const SFFloat &value) {

    validateStop1Bounce(value);

    _stop1Bounce = value;
  }

  /**
   * @brief Non-validating write of stop1Bounce (runtime/reader ingest path).
   * @details Assigns without the range check setStop1Bounce() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setStop1Bounce() stays the
   *          enforcement point for programmatic callers.
   */
  void setStop1BounceUnchecked(const SFFloat &value) { _stop1Bounce = value; }

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

    validateStop1ErrorCorrection(value);

    _stop1ErrorCorrection = value;
  }

  /**
   * @brief Non-validating write of stop1ErrorCorrection (runtime/reader ingest
   * path).
   * @details Assigns without the range check setStop1ErrorCorrection()
   * enforces, so a permissive reader keeps an out-of-range authored value
   * rather than reject the whole document. setStop1ErrorCorrection() stays the
   *          enforcement point for programmatic callers.
   */
  void setStop1ErrorCorrectionUnchecked(const SFFloat &value) {
    _stop1ErrorCorrection = value;
  }

  /**
   * @brief Gets the value of stop2Bounce. AccessType: inputOutput
   * @details stop2Bounce is velocity factor for bounce back once stop point is
   * reached.
   * @return SFFloat The current value of stop2Bounce.
   */
  SFFloat getStop2Bounce() const { return _stop2Bounce; }

  /**
   * @brief Sets the value of stop2Bounce. AccessType: inputOutput
   * @details stop2Bounce is velocity factor for bounce back once stop point is
   * reached.
   * @param value The new value for stop2Bounce.
   */
  void setStop2Bounce(const SFFloat &value) {

    validateStop2Bounce(value);

    _stop2Bounce = value;
  }

  /**
   * @brief Non-validating write of stop2Bounce (runtime/reader ingest path).
   * @details Assigns without the range check setStop2Bounce() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setStop2Bounce() stays the
   *          enforcement point for programmatic callers.
   */
  void setStop2BounceUnchecked(const SFFloat &value) { _stop2Bounce = value; }

  /**
   * @brief Gets the value of stop2ErrorCorrection. AccessType: inputOutput
   * @details stop2ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   * @return SFFloat The current value of stop2ErrorCorrection.
   */
  SFFloat getStop2ErrorCorrection() const { return _stop2ErrorCorrection; }

  /**
   * @brief Sets the value of stop2ErrorCorrection. AccessType: inputOutput
   * @details stop2ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   * @param value The new value for stop2ErrorCorrection.
   */
  void setStop2ErrorCorrection(const SFFloat &value) {

    validateStop2ErrorCorrection(value);

    _stop2ErrorCorrection = value;
  }

  /**
   * @brief Non-validating write of stop2ErrorCorrection (runtime/reader ingest
   * path).
   * @details Assigns without the range check setStop2ErrorCorrection()
   * enforces, so a permissive reader keeps an out-of-range authored value
   * rather than reject the whole document. setStop2ErrorCorrection() stays the
   *          enforcement point for programmatic callers.
   */
  void setStop2ErrorCorrectionUnchecked(const SFFloat &value) {
    _stop2ErrorCorrection = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "UniversalJoint").
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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesStop1Bounce(const SFFloat &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesStop1ErrorCorrection(
      const SFFloat &value, const std::string &nodeType,
      const std::string &defName, std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesStop2Bounce(const SFFloat &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesStop2ErrorCorrection(
      const SFFloat &value, const std::string &nodeType,
      const std::string &defName, std::vector<RangeDiagnostic> &out);

private:
  static void validateStop1Bounce(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("stop1Bounce below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("stop1Bounce above maximum of 1");
  }

  static void validateStop1ErrorCorrection(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("stop1ErrorCorrection below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("stop1ErrorCorrection above maximum of 1");
  }

  static void validateStop2Bounce(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("stop2Bounce below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("stop2Bounce above maximum of 1");
  }

  static void validateStop2ErrorCorrection(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("stop2ErrorCorrection below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("stop2ErrorCorrection above maximum of 1");
  }

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
   * @brief Member variable for stop1Bounce.
   */

  SFFloat _stop1Bounce{0};

  /**
   * @brief Member variable for stop1ErrorCorrection.
   */

  SFFloat _stop1ErrorCorrection{0.8};

  /**
   * @brief Member variable for stop2Bounce.
   */

  SFFloat _stop2Bounce{0};

  /**
   * @brief Member variable for stop2ErrorCorrection.
   */

  SFFloat _stop2ErrorCorrection{0.8};
};

} // namespace x3d::nodes
