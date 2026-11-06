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

#include "x3d/nodes/X3DBoundedObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class CollisionCollection
 * @brief CollisionCollection holds a collection of objects that can be managed
 * as a single entity for resolution of inter-object collisions.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#CollisionCollection
 */
class CollisionCollection : public virtual X3DChildNode,
                            public virtual X3DBoundedObject {
public:
  CollisionCollection() = default;

  ~CollisionCollection() = default;

  static std::vector<AppliedParametersChoices> getDefaultAppliedParameters() {
    return std::vector<AppliedParametersChoices>{
        AppliedParametersChoices::BOUNCE};
  }

  static SFFloat getDefaultBounce() { return 0; }

  static SFBool getDefaultEnabled() { return true; }

  static SFVec2f getDefaultFrictionCoefficients() { return SFVec2f{0, 0}; }

  static SFFloat getDefaultMinBounceSpeed() { return 0.1; }

  static SFVec2f getDefaultSlipFactors() { return SFVec2f{0, 0}; }

  static SFFloat getDefaultSoftnessConstantForceMix() { return 0.0001; }

  static SFFloat getDefaultSoftnessErrorCorrection() { return 0.8; }

  static SFVec2f getDefaultSurfaceSpeed() { return SFVec2f{0, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "collider"; }

  static std::string componentName() { return "RigidBodyPhysics"; }

  static int componentLevel() { return 1; }

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
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details CollisionCollection node holds a collection of objects in the
   * collidables field that can be managed as a single entity for resolution of
   * inter-object collisions with other groups of collidable objects.
   */
  const MFNode &getCollidables() const { return _collidables; }

  /**
   * @details Permitted X3D node types: X3DNBodyCollisionSpaceNode,
   * X3DNBodyCollidableNode
   */
  static const std::vector<std::string> &acceptableCollidablesNodeTypes() {
    static const std::vector<std::string> types = {"X3DNBodyCollisionSpaceNode",
                                                   "X3DNBodyCollidableNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details CollisionCollection node holds a collection of objects in the
   * collidables field that can be managed as a single entity for resolution of
   * inter-object collisions with other groups of collidable objects.
   */
  void setCollidables(const MFNode &value) { _collidables = value; }

  void setCollidables(MFNode &&value) { _collidables = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Author-provided prose that describes intended purpose of the node.
   */
  const SFString &getDescription() const { return _description; }

  /**
   * Access: inputOutput.
   * @details Author-provided prose that describes intended purpose of the node.
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Enables/disables node operation.
   */
  const SFBool &getEnabled() const { return _enabled; }

  /**
   * Access: inputOutput.
   * @details Enables/disables node operation.
   */
  void setEnabled(const SFBool &value) { _enabled = value; }

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
   * SFVec2f; access: inputOutput. Borrows the stored value.
   * @details slipFactors used for computing surface drag.
   */
  const SFVec2f &getSlipFactors() const { return _slipFactors; }

  /**
   * Access: inputOutput.
   * @details slipFactors used for computing surface drag.
   */
  void setSlipFactors(const SFVec2f &value) { _slipFactors = value; }

  void setSlipFactors(SFVec2f &&value) { _slipFactors = std::move(value); }

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

  SFFloat _bounce{0};

  MFNode _collidables{};

  SFString _description{};

  SFBool _enabled{true};

  SFVec2f _frictionCoefficients{SFVec2f{0, 0}};

  SFFloat _minBounceSpeed{0.1};

  SFVec2f _slipFactors{SFVec2f{0, 0}};

  SFFloat _softnessConstantForceMix{0.0001};

  SFFloat _softnessErrorCorrection{0.8};

  SFVec2f _surfaceSpeed{SFVec2f{0, 0}};
};

} // namespace x3d::nodes
