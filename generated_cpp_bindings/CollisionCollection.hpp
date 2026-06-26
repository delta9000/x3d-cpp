// CollisionCollection.hpp
#ifndef COLLISIONCOLLECTION_HPP
#define COLLISIONCOLLECTION_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DBoundedObject.hpp"

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
  /**
   * @brief Default constructor for CollisionCollection
   */
  CollisionCollection() = default;

  /**
   * @brief Destructor for CollisionCollection
   */
  ~CollisionCollection() = default;

  /**
   * @brief Get the default value for appliedParameters
   * @return std::vector<AppliedParametersChoices> The default value
   */
  static std::vector<AppliedParametersChoices> getDefaultAppliedParameters() {
    return std::vector<AppliedParametersChoices>{
        AppliedParametersChoices::BOUNCE};
  }

  /**
   * @brief Get the default value for bounce
   * @return SFFloat The default value
   */
  static SFFloat getDefaultBounce() { return 0; }

  /**
   * @brief Get the default value for enabled
   * @return SFBool The default value
   */
  static SFBool getDefaultEnabled() { return true; }

  /**
   * @brief Get the default value for frictionCoefficients
   * @return SFVec2f The default value
   */
  static SFVec2f getDefaultFrictionCoefficients() { return SFVec2f{0, 0}; }

  /**
   * @brief Get the default value for minBounceSpeed
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMinBounceSpeed() { return 0.1; }

  /**
   * @brief Get the default value for slipFactors
   * @return SFVec2f The default value
   */
  static SFVec2f getDefaultSlipFactors() { return SFVec2f{0, 0}; }

  /**
   * @brief Get the default value for softnessConstantForceMix
   * @return SFFloat The default value
   */
  static SFFloat getDefaultSoftnessConstantForceMix() { return 0.0001; }

  /**
   * @brief Get the default value for softnessErrorCorrection
   * @return SFFloat The default value
   */
  static SFFloat getDefaultSoftnessErrorCorrection() { return 0.8; }

  /**
   * @brief Get the default value for surfaceSpeed
   * @return SFVec2f The default value
   */
  static SFVec2f getDefaultSurfaceSpeed() { return SFVec2f{0, 0}; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "collider"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "RigidBodyPhysics"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of appliedParameters. AccessType: inputOutput
   * @details Default global parameters for collision outputs of rigid body
   * physics system.
   * @return std::vector<AppliedParametersChoices> The current value of
   * appliedParameters.
   */
  std::vector<AppliedParametersChoices> getAppliedParameters() const {
    return _appliedParameters;
  }

  /**
   * @brief Sets the value of appliedParameters. AccessType: inputOutput
   * @details Default global parameters for collision outputs of rigid body
   * physics system.
   * @param value The new value for appliedParameters.
   */
  void
  setAppliedParameters(const std::vector<AppliedParametersChoices> &value) {

    _appliedParameters = value;
  }

  void setAppliedParameters(std::vector<AppliedParametersChoices> &&value) {

    _appliedParameters = std::move(value);
  }

  /**
   * @brief Gets the value of bounce. AccessType: inputOutput
   * @details bounce indicates bounciness (0 = no bounce at all, 1 = maximum
   * bounce).
   * @return SFFloat The current value of bounce.
   */
  SFFloat getBounce() const { return _bounce; }

  /**
   * @brief Sets the value of bounce. AccessType: inputOutput
   * @details bounce indicates bounciness (0 = no bounce at all, 1 = maximum
   * bounce).
   * @param value The new value for bounce.
   */
  void setBounce(const SFFloat &value) {

    validateBounce(value);

    _bounce = value;
  }

  /**
   * @brief Non-validating write of bounce (runtime/reader ingest path).
   * @details Assigns without the range check setBounce() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setBounce() stays the
   *          enforcement point for programmatic callers.
   */
  void setBounceUnchecked(const SFFloat &value) { _bounce = value; }

  /**
   * @brief Gets the value of collidables. AccessType: inputOutput
   * @details CollisionCollection node holds a collection of objects in the
   * collidables field that can be managed as a single entity for resolution of
   * inter-object collisions with other groups of collidable objects.
   * @return MFNode The current value of collidables.
   */
  MFNode getCollidables() const { return _collidables; }

  /**
   * @brief Acceptable node types for the collidables field.
   * @details Permitted X3D node types: X3DNBodyCollisionSpaceNode,
   * X3DNBodyCollidableNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableCollidablesNodeTypes() {
    static const std::vector<std::string> types = {"X3DNBodyCollisionSpaceNode",
                                                   "X3DNBodyCollidableNode"};
    return types;
  }

  /**
   * @brief Sets the value of collidables. AccessType: inputOutput
   * @details CollisionCollection node holds a collection of objects in the
   * collidables field that can be managed as a single entity for resolution of
   * inter-object collisions with other groups of collidable objects.
   * @param value The new value for collidables.
   */
  void setCollidables(const MFNode &value) { _collidables = value; }

  void setCollidables(MFNode &&value) { _collidables = std::move(value); }

  /**
   * @brief Gets the value of description. AccessType: inputOutput
   * @details Author-provided prose that describes intended purpose of the node.
   * @return SFString The current value of description.
   */
  SFString getDescription() const { return _description; }

  /**
   * @brief Sets the value of description. AccessType: inputOutput
   * @details Author-provided prose that describes intended purpose of the node.
   * @param value The new value for description.
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

  /**
   * @brief Gets the value of enabled. AccessType: inputOutput
   * @details Enables/disables node operation.
   * @return SFBool The current value of enabled.
   */
  SFBool getEnabled() const { return _enabled; }

  /**
   * @brief Sets the value of enabled. AccessType: inputOutput
   * @details Enables/disables node operation.
   * @param value The new value for enabled.
   */
  void setEnabled(const SFBool &value) { _enabled = value; }

  /**
   * @brief Gets the value of frictionCoefficients. AccessType: inputOutput
   * @details frictionCoefficients used for computing surface drag.
   * @return SFVec2f The current value of frictionCoefficients.
   */
  SFVec2f getFrictionCoefficients() const { return _frictionCoefficients; }

  /**
   * @brief Sets the value of frictionCoefficients. AccessType: inputOutput
   * @details frictionCoefficients used for computing surface drag.
   * @param value The new value for frictionCoefficients.
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
   * @brief Non-validating write of frictionCoefficients (runtime/reader ingest
   * path).
   * @details Assigns without the range check setFrictionCoefficients()
   * enforces, so a permissive reader keeps an out-of-range authored value
   * rather than reject the whole document. setFrictionCoefficients() stays the
   *          enforcement point for programmatic callers.
   */
  void setFrictionCoefficientsUnchecked(const SFVec2f &value) {
    _frictionCoefficients = value;
  }

  /**
   * @brief Gets the value of minBounceSpeed. AccessType: inputOutput
   * @details minBounceSpeed m/s needed to bounce.
   * @return SFFloat The current value of minBounceSpeed.
   */
  SFFloat getMinBounceSpeed() const { return _minBounceSpeed; }

  /**
   * @brief Sets the value of minBounceSpeed. AccessType: inputOutput
   * @details minBounceSpeed m/s needed to bounce.
   * @param value The new value for minBounceSpeed.
   */
  void setMinBounceSpeed(const SFFloat &value) {

    validateMinBounceSpeed(value);

    _minBounceSpeed = value;
  }

  /**
   * @brief Non-validating write of minBounceSpeed (runtime/reader ingest path).
   * @details Assigns without the range check setMinBounceSpeed() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setMinBounceSpeed() stays the
   *          enforcement point for programmatic callers.
   */
  void setMinBounceSpeedUnchecked(const SFFloat &value) {
    _minBounceSpeed = value;
  }

  /**
   * @brief Gets the value of slipFactors. AccessType: inputOutput
   * @details slipFactors used for computing surface drag.
   * @return SFVec2f The current value of slipFactors.
   */
  SFVec2f getSlipFactors() const { return _slipFactors; }

  /**
   * @brief Sets the value of slipFactors. AccessType: inputOutput
   * @details slipFactors used for computing surface drag.
   * @param value The new value for slipFactors.
   */
  void setSlipFactors(const SFVec2f &value) { _slipFactors = value; }

  void setSlipFactors(SFVec2f &&value) { _slipFactors = std::move(value); }

  /**
   * @brief Gets the value of softnessConstantForceMix. AccessType: inputOutput
   * @details softnessConstantForceMix value applies a constant force value to
   * make colliding surfaces appear to be somewhat soft.
   * @return SFFloat The current value of softnessConstantForceMix.
   */
  SFFloat getSoftnessConstantForceMix() const {
    return _softnessConstantForceMix;
  }

  /**
   * @brief Sets the value of softnessConstantForceMix. AccessType: inputOutput
   * @details softnessConstantForceMix value applies a constant force value to
   * make colliding surfaces appear to be somewhat soft.
   * @param value The new value for softnessConstantForceMix.
   */
  void setSoftnessConstantForceMix(const SFFloat &value) {

    validateSoftnessConstantForceMix(value);

    _softnessConstantForceMix = value;
  }

  /**
   * @brief Non-validating write of softnessConstantForceMix (runtime/reader
   * ingest path).
   * @details Assigns without the range check setSoftnessConstantForceMix()
   * enforces, so a permissive reader keeps an out-of-range authored value
   * rather than reject the whole document. setSoftnessConstantForceMix() stays
   * the enforcement point for programmatic callers.
   */
  void setSoftnessConstantForceMixUnchecked(const SFFloat &value) {
    _softnessConstantForceMix = value;
  }

  /**
   * @brief Gets the value of softnessErrorCorrection. AccessType: inputOutput
   * @details softnessErrorCorrection indicates fraction of collision error
   * fixed in a set of evaluations (0 = no error correction, 1 = all errors
   * corrected in single step).
   * @return SFFloat The current value of softnessErrorCorrection.
   */
  SFFloat getSoftnessErrorCorrection() const {
    return _softnessErrorCorrection;
  }

  /**
   * @brief Sets the value of softnessErrorCorrection. AccessType: inputOutput
   * @details softnessErrorCorrection indicates fraction of collision error
   * fixed in a set of evaluations (0 = no error correction, 1 = all errors
   * corrected in single step).
   * @param value The new value for softnessErrorCorrection.
   */
  void setSoftnessErrorCorrection(const SFFloat &value) {

    validateSoftnessErrorCorrection(value);

    _softnessErrorCorrection = value;
  }

  /**
   * @brief Non-validating write of softnessErrorCorrection (runtime/reader
   * ingest path).
   * @details Assigns without the range check setSoftnessErrorCorrection()
   * enforces, so a permissive reader keeps an out-of-range authored value
   * rather than reject the whole document. setSoftnessErrorCorrection() stays
   * the enforcement point for programmatic callers.
   */
  void setSoftnessErrorCorrectionUnchecked(const SFFloat &value) {
    _softnessErrorCorrection = value;
  }

  /**
   * @brief Gets the value of surfaceSpeed. AccessType: inputOutput
   * @details surfaceSpeed defines speed vectors for computing surface drag, if
   * contact surfaces move independently of bodies.
   * @return SFVec2f The current value of surfaceSpeed.
   */
  SFVec2f getSurfaceSpeed() const { return _surfaceSpeed; }

  /**
   * @brief Sets the value of surfaceSpeed. AccessType: inputOutput
   * @details surfaceSpeed defines speed vectors for computing surface drag, if
   * contact surfaces move independently of bodies.
   * @param value The new value for surfaceSpeed.
   */
  void setSurfaceSpeed(const SFVec2f &value) { _surfaceSpeed = value; }

  void setSurfaceSpeed(SFVec2f &&value) { _surfaceSpeed = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "CollisionCollection").
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

    if (value < 0)
      throw std::out_of_range("bounce below minimum of 0");
    if (value > 1)
      throw std::out_of_range("bounce above maximum of 1");
  }

  static void validateFrictionCoefficients(const SFVec2f &value) {

    if (value.x < 0)
      throw std::out_of_range("frictionCoefficients.x below minimum of 0");

    if (value.y < 0)
      throw std::out_of_range("frictionCoefficients.y below minimum of 0");
  }

  static void validateMinBounceSpeed(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("minBounceSpeed below minimum of 0");
  }

  static void validateSoftnessConstantForceMix(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("softnessConstantForceMix below minimum of 0");
    if (value > 1)
      throw std::out_of_range("softnessConstantForceMix above maximum of 1");
  }

  static void validateSoftnessErrorCorrection(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("softnessErrorCorrection below minimum of 0");
    if (value > 1)
      throw std::out_of_range("softnessErrorCorrection above maximum of 1");
  }

  /**
   * @brief Member variable for appliedParameters.
   */

  std::vector<AppliedParametersChoices> _appliedParameters{
      std::vector<AppliedParametersChoices>{AppliedParametersChoices::BOUNCE}};

  /**
   * @brief Member variable for bounce.
   */

  SFFloat _bounce{0};

  /**
   * @brief Member variable for collidables.
   */

  MFNode _collidables{};

  /**
   * @brief Member variable for description.
   */

  SFString _description{};

  /**
   * @brief Member variable for enabled.
   */

  SFBool _enabled{true};

  /**
   * @brief Member variable for frictionCoefficients.
   */

  SFVec2f _frictionCoefficients{SFVec2f{0, 0}};

  /**
   * @brief Member variable for minBounceSpeed.
   */

  SFFloat _minBounceSpeed{0.1};

  /**
   * @brief Member variable for slipFactors.
   */

  SFVec2f _slipFactors{SFVec2f{0, 0}};

  /**
   * @brief Member variable for softnessConstantForceMix.
   */

  SFFloat _softnessConstantForceMix{0.0001};

  /**
   * @brief Member variable for softnessErrorCorrection.
   */

  SFFloat _softnessErrorCorrection{0.8};

  /**
   * @brief Member variable for surfaceSpeed.
   */

  SFVec2f _surfaceSpeed{SFVec2f{0, 0}};
};

#endif // COLLISIONCOLLECTION_HPP