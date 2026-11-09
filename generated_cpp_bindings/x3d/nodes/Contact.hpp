#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Contact
 * @brief Contact nodes are produced as output events when two collidable
 * objects or spaces make contact.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#Contact
 */
class Contact : public virtual X3DNode {
public:
  Contact() = default;

  ~Contact() = default;

  static std::vector<AppliedParametersChoices> getDefaultAppliedParameters() {
    return std::vector<AppliedParametersChoices>{
        AppliedParametersChoices::BOUNCE};
  }

  static SFNode getDefaultBody1() { return nullptr; }

  static SFNode getDefaultBody2() { return nullptr; }

  static SFFloat getDefaultBounce() { return 0; }

  static SFVec3f getDefaultContactNormal() { return SFVec3f{0, 1, 0}; }

  static SFFloat getDefaultDepth() { return 0; }

  static SFVec2f getDefaultFrictionCoefficients() { return SFVec2f{0, 0}; }

  static SFVec3f getDefaultFrictionDirection() { return SFVec3f{0, 1, 0}; }

  static SFNode getDefaultGeometry1() { return nullptr; }

  static SFNode getDefaultGeometry2() { return nullptr; }

  static SFFloat getDefaultMinBounceSpeed() { return 0; }

  static SFVec3f getDefaultPosition() { return SFVec3f{0, 0, 0}; }

  static SFVec2f getDefaultSlipCoefficients() { return SFVec2f{0, 0}; }

  static SFFloat getDefaultSoftnessConstantForceMix() { return 0.0001; }

  static SFFloat getDefaultSoftnessErrorCorrection() { return 0.8; }

  static SFVec2f getDefaultSurfaceSpeed() { return SFVec2f{0, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "contacts"; }

  static std::string componentName() { return "RigidBodyPhysics"; }

  static int componentLevel() { return 2; }

  /**
   * std::vector<AppliedParametersChoices>; access: inputOutput. Borrows the
   * stored value.
   * @details Default global parameters for collision outputs of rigid body
   * physics system.
   */
  const std::vector<AppliedParametersChoices> &getAppliedParameters() const {
    return _appliedParameters;
  }

  /**
   * Access: inputOutput.
   * @details Default global parameters for collision outputs of rigid body
   * physics system.
   */
  void
  setAppliedParameters(const std::vector<AppliedParametersChoices> &value) {

    _appliedParameters = value;
  }

  void setAppliedParameters(std::vector<AppliedParametersChoices> &&value) {

    _appliedParameters = std::move(value);
  }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details The body1 and body2 fields specify two top-level nodes that should
   * be evaluated in the physics model as a single set of interactions with
   * respect to each other.
   */
  const SFNode &getBody1() const { return _body1; }

  /**
   * @details Permitted X3D node types: RigidBody
   */
  static const std::vector<std::string> &acceptableBody1NodeTypes() {
    static const std::vector<std::string> types = {"RigidBody"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The body1 and body2 fields specify two top-level nodes that should
   * be evaluated in the physics model as a single set of interactions with
   * respect to each other.
   */
  void setBody1(const SFNode &value) { _body1 = value; }

  void setBody1(SFNode &&value) { _body1 = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details The body1 and body2 fields specify two top-level nodes that should
   * be evaluated in the physics model as a single set of interactions with
   * respect to each other.
   */
  const SFNode &getBody2() const { return _body2; }

  /**
   * @details Permitted X3D node types: RigidBody
   */
  static const std::vector<std::string> &acceptableBody2NodeTypes() {
    static const std::vector<std::string> types = {"RigidBody"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The body1 and body2 fields specify two top-level nodes that should
   * be evaluated in the physics model as a single set of interactions with
   * respect to each other.
   */
  void setBody2(const SFNode &value) { _body2 = value; }

  void setBody2(SFNode &&value) { _body2 = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details bounce indicates bounciness (0 = no bounce at all, 1 = maximum
   * bounce).
   */
  const SFFloat &getBounce() const { return _bounce; }

  /**
   * Access: inputOutput.
   * @details bounce indicates bounciness (0 = no bounce at all, 1 = maximum
   * bounce).
   */
  void setBounce(const SFFloat &value) {

    validateBounce(value);

    _bounce = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setBounce() for range validation.
   */
  void setBounceUnchecked(const SFFloat &value) { _bounce = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details contactNormal is unit vector describing normal between two
   * colliding bodies.
   */
  const SFVec3f &getContactNormal() const { return _contactNormal; }

  /**
   * Access: inputOutput.
   * @details contactNormal is unit vector describing normal between two
   * colliding bodies.
   */
  void setContactNormal(const SFVec3f &value) { _contactNormal = value; }

  void setContactNormal(SFVec3f &&value) { _contactNormal = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details depth indicates how deep the current intersection is along normal
   * vector.
   */
  const SFFloat &getDepth() const { return _depth; }

  /**
   * Access: inputOutput.
   * @details depth indicates how deep the current intersection is along normal
   * vector.
   */
  void setDepth(const SFFloat &value) { _depth = value; }

  /**
   * SFVec2f; access: inputOutput. Borrows the stored value.
   * @details frictionCoefficients used for computing surface drag.
   */
  const SFVec2f &getFrictionCoefficients() const {
    return _frictionCoefficients;
  }

  /**
   * Access: inputOutput.
   * @details frictionCoefficients used for computing surface drag.
   */
  void setFrictionCoefficients(const SFVec2f &value) {

    validateFrictionCoefficients(value);

    _frictionCoefficients = value;
  }

  void setFrictionCoefficients(SFVec2f &&value) {

    validateFrictionCoefficients(value);

    _frictionCoefficients = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setFrictionCoefficients() for range validation.
   */
  void setFrictionCoefficientsUnchecked(const SFVec2f &value) {
    _frictionCoefficients = value;
  }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details frictionDirection controls friction vector.
   */
  const SFVec3f &getFrictionDirection() const { return _frictionDirection; }

  /**
   * Access: inputOutput.
   * @details frictionDirection controls friction vector.
   */
  void setFrictionDirection(const SFVec3f &value) {

    _frictionDirection = value;
  }

  void setFrictionDirection(SFVec3f &&value) {

    _frictionDirection = std::move(value);
  }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details The geometry1 and geometry2 fields specify collision-related
   * information about body1 and body2.
   */
  const SFNode &getGeometry1() const { return _geometry1; }

  /**
   * @details Permitted X3D node types: X3DNBodyCollidableNode
   */
  static const std::vector<std::string> &acceptableGeometry1NodeTypes() {
    static const std::vector<std::string> types = {"X3DNBodyCollidableNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The geometry1 and geometry2 fields specify collision-related
   * information about body1 and body2.
   */
  void setGeometry1(const SFNode &value) { _geometry1 = value; }

  void setGeometry1(SFNode &&value) { _geometry1 = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details The geometry1 and geometry2 fields specify collision-related
   * information about body1 and body2.
   */
  const SFNode &getGeometry2() const { return _geometry2; }

  /**
   * @details Permitted X3D node types: X3DNBodyCollidableNode
   */
  static const std::vector<std::string> &acceptableGeometry2NodeTypes() {
    static const std::vector<std::string> types = {"X3DNBodyCollidableNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The geometry1 and geometry2 fields specify collision-related
   * information about body1 and body2.
   */
  void setGeometry2(const SFNode &value) { _geometry2 = value; }

  void setGeometry2(SFNode &&value) { _geometry2 = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details minBounceSpeed m/s needed to bounce.
   */
  const SFFloat &getMinBounceSpeed() const { return _minBounceSpeed; }

  /**
   * Access: inputOutput.
   * @details minBounceSpeed m/s needed to bounce.
   */
  void setMinBounceSpeed(const SFFloat &value) {

    validateMinBounceSpeed(value);

    _minBounceSpeed = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setMinBounceSpeed() for range validation.
   */
  void setMinBounceSpeedUnchecked(const SFFloat &value) {
    _minBounceSpeed = value;
  }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details position (x, y, z in meters) of exact location of collision.
   */
  const SFVec3f &getPosition() const { return _position; }

  /**
   * Access: inputOutput.
   * @details position (x, y, z in meters) of exact location of collision.
   */
  void setPosition(const SFVec3f &value) { _position = value; }

  void setPosition(SFVec3f &&value) { _position = std::move(value); }

  /**
   * SFVec2f; access: inputOutput. Borrows the stored value.
   * @details slipCoefficients used for computing surface drag.
   */
  const SFVec2f &getSlipCoefficients() const { return _slipCoefficients; }

  /**
   * Access: inputOutput.
   * @details slipCoefficients used for computing surface drag.
   */
  void setSlipCoefficients(const SFVec2f &value) { _slipCoefficients = value; }

  void setSlipCoefficients(SFVec2f &&value) {

    _slipCoefficients = std::move(value);
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details softnessConstantForceMix value applies a constant force value to
   * make colliding surfaces appear to be somewhat soft.
   */
  const SFFloat &getSoftnessConstantForceMix() const {
    return _softnessConstantForceMix;
  }

  /**
   * Access: inputOutput.
   * @details softnessConstantForceMix value applies a constant force value to
   * make colliding surfaces appear to be somewhat soft.
   */
  void setSoftnessConstantForceMix(const SFFloat &value) {

    validateSoftnessConstantForceMix(value);

    _softnessConstantForceMix = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setSoftnessConstantForceMix() for range
   * validation.
   */
  void setSoftnessConstantForceMixUnchecked(const SFFloat &value) {
    _softnessConstantForceMix = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details softnessErrorCorrection indicates fraction of collision error
   * fixed in a set of evaluations (0 = no error correction, 1 = all errors
   * corrected in single step).
   */
  const SFFloat &getSoftnessErrorCorrection() const {
    return _softnessErrorCorrection;
  }

  /**
   * Access: inputOutput.
   * @details softnessErrorCorrection indicates fraction of collision error
   * fixed in a set of evaluations (0 = no error correction, 1 = all errors
   * corrected in single step).
   */
  void setSoftnessErrorCorrection(const SFFloat &value) {

    validateSoftnessErrorCorrection(value);

    _softnessErrorCorrection = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setSoftnessErrorCorrection() for range validation.
   */
  void setSoftnessErrorCorrectionUnchecked(const SFFloat &value) {
    _softnessErrorCorrection = value;
  }

  /**
   * SFVec2f; access: inputOutput. Borrows the stored value.
   * @details surfaceSpeed defines speed vectors for computing surface drag, if
   * contact surfaces move independently of bodies.
   */
  const SFVec2f &getSurfaceSpeed() const { return _surfaceSpeed; }

  /**
   * Access: inputOutput.
   * @details surfaceSpeed defines speed vectors for computing surface drag, if
   * contact surfaces move independently of bodies.
   */
  void setSurfaceSpeed(const SFVec2f &value) { _surfaceSpeed = value; }

  void setSurfaceSpeed(SFVec2f &&value) { _surfaceSpeed = std::move(value); }

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
  static void checkRangesBounce(const SFFloat &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesFrictionCoefficients(
      const SFVec2f &value, const std::string &nodeType,
      const std::string &defName, std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesMinBounceSpeed(const SFFloat &value,
                                        const std::string &nodeType,
                                        const std::string &defName,
                                        std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesSoftnessConstantForceMix(
      const SFFloat &value, const std::string &nodeType,
      const std::string &defName, std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesSoftnessErrorCorrection(
      const SFFloat &value, const std::string &nodeType,
      const std::string &defName, std::vector<RangeDiagnostic> &out);

private:
  static void validateBounce(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("bounce below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("bounce above maximum of 1");
  }

  static void validateFrictionCoefficients(const SFVec2f &value) {

    if (value.x < 0.0f)
      throw std::out_of_range("frictionCoefficients.x below minimum of 0");

    if (value.y < 0.0f)
      throw std::out_of_range("frictionCoefficients.y below minimum of 0");
  }

  static void validateMinBounceSpeed(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("minBounceSpeed below minimum of 0");
  }

  static void validateSoftnessConstantForceMix(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("softnessConstantForceMix below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("softnessConstantForceMix above maximum of 1");
  }

  static void validateSoftnessErrorCorrection(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("softnessErrorCorrection below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("softnessErrorCorrection above maximum of 1");
  }

  std::vector<AppliedParametersChoices> _appliedParameters{
      std::vector<AppliedParametersChoices>{AppliedParametersChoices::BOUNCE}};

  SFNode _body1{nullptr};

  SFNode _body2{nullptr};

  SFFloat _bounce{0};

  SFVec3f _contactNormal{SFVec3f{0, 1, 0}};

  SFFloat _depth{0};

  SFVec2f _frictionCoefficients{SFVec2f{0, 0}};

  SFVec3f _frictionDirection{SFVec3f{0, 1, 0}};

  SFNode _geometry1{nullptr};

  SFNode _geometry2{nullptr};

  SFFloat _minBounceSpeed{0};

  SFVec3f _position{SFVec3f{0, 0, 0}};

  SFVec2f _slipCoefficients{SFVec2f{0, 0}};

  SFFloat _softnessConstantForceMix{0.0001};

  SFFloat _softnessErrorCorrection{0.8};

  SFVec2f _surfaceSpeed{SFVec2f{0, 0}};
};

} // namespace x3d::nodes
