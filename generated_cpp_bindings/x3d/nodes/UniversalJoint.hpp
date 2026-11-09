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
  UniversalJoint() = default;

  ~UniversalJoint() = default;

  static SFVec3f getDefaultAnchorPoint() { return SFVec3f{0, 0, 0}; }

  static SFVec3f getDefaultAxis1() { return SFVec3f{1, 0, 0}; }

  static SFVec3f getDefaultAxis2() { return SFVec3f{0, 1, 0}; }

  static SFFloat getDefaultStop1Bounce() { return 0; }

  static SFFloat getDefaultStop1ErrorCorrection() { return 0.8; }

  static SFFloat getDefaultStop2Bounce() { return 0; }

  static SFFloat getDefaultStop2ErrorCorrection() { return 0.8; }

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
   * @details stop1Bounce is velocity factor for bounce back once stop point is
   * reached.
   */
  const SFFloat &getStop1Bounce() const { return _stop1Bounce; }

  /**
   * Access: inputOutput.
   * @details stop1Bounce is velocity factor for bounce back once stop point is
   * reached.
   */
  void setStop1Bounce(const SFFloat &value) {

    validateStop1Bounce(value);

    _stop1Bounce = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setStop1Bounce() for range validation.
   */
  void setStop1BounceUnchecked(const SFFloat &value) { _stop1Bounce = value; }

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

    validateStop1ErrorCorrection(value);

    _stop1ErrorCorrection = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setStop1ErrorCorrection() for range validation.
   */
  void setStop1ErrorCorrectionUnchecked(const SFFloat &value) {
    _stop1ErrorCorrection = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details stop2Bounce is velocity factor for bounce back once stop point is
   * reached.
   */
  const SFFloat &getStop2Bounce() const { return _stop2Bounce; }

  /**
   * Access: inputOutput.
   * @details stop2Bounce is velocity factor for bounce back once stop point is
   * reached.
   */
  void setStop2Bounce(const SFFloat &value) {

    validateStop2Bounce(value);

    _stop2Bounce = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setStop2Bounce() for range validation.
   */
  void setStop2BounceUnchecked(const SFFloat &value) { _stop2Bounce = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details stop2ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   */
  const SFFloat &getStop2ErrorCorrection() const {
    return _stop2ErrorCorrection;
  }

  /**
   * Access: inputOutput.
   * @details stop2ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   */
  void setStop2ErrorCorrection(const SFFloat &value) {

    validateStop2ErrorCorrection(value);

    _stop2ErrorCorrection = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setStop2ErrorCorrection() for range validation.
   */
  void setStop2ErrorCorrectionUnchecked(const SFFloat &value) {
    _stop2ErrorCorrection = value;
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

  SFVec3f _anchorPoint{SFVec3f{0, 0, 0}};

  SFVec3f _axis1{SFVec3f{1, 0, 0}};

  SFVec3f _axis2{SFVec3f{0, 1, 0}};

  SFVec3f _body1AnchorPoint{};

  SFVec3f _body1Axis{};

  SFVec3f _body2AnchorPoint{};

  SFVec3f _body2Axis{};

  SFFloat _stop1Bounce{0};

  SFFloat _stop1ErrorCorrection{0.8};

  SFFloat _stop2Bounce{0};

  SFFloat _stop2ErrorCorrection{0.8};
};

} // namespace x3d::nodes
