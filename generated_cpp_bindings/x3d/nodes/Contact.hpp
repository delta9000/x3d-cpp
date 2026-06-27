// Contact.hpp
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
  /**
   * @brief Default constructor for Contact
   */
  Contact() = default;

  /**
   * @brief Destructor for Contact
   */
  ~Contact() = default;

  /**
   * @brief Get the default value for appliedParameters
   * @return std::vector<AppliedParametersChoices> The default value
   */
  static std::vector<AppliedParametersChoices> getDefaultAppliedParameters() {
    return std::vector<AppliedParametersChoices>{
        AppliedParametersChoices::BOUNCE};
  }

  /**
   * @brief Get the default value for body1
   * @return SFNode The default value
   */
  static SFNode getDefaultBody1() { return nullptr; }

  /**
   * @brief Get the default value for body2
   * @return SFNode The default value
   */
  static SFNode getDefaultBody2() { return nullptr; }

  /**
   * @brief Get the default value for bounce
   * @return SFFloat The default value
   */
  static SFFloat getDefaultBounce() { return 0; }

  /**
   * @brief Get the default value for contactNormal
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultContactNormal() { return SFVec3f{0, 1, 0}; }

  /**
   * @brief Get the default value for depth
   * @return SFFloat The default value
   */
  static SFFloat getDefaultDepth() { return 0; }

  /**
   * @brief Get the default value for frictionCoefficients
   * @return SFVec2f The default value
   */
  static SFVec2f getDefaultFrictionCoefficients() { return SFVec2f{0, 0}; }

  /**
   * @brief Get the default value for frictionDirection
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultFrictionDirection() { return SFVec3f{0, 1, 0}; }

  /**
   * @brief Get the default value for geometry1
   * @return SFNode The default value
   */
  static SFNode getDefaultGeometry1() { return nullptr; }

  /**
   * @brief Get the default value for geometry2
   * @return SFNode The default value
   */
  static SFNode getDefaultGeometry2() { return nullptr; }

  /**
   * @brief Get the default value for minBounceSpeed
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMinBounceSpeed() { return 0; }

  /**
   * @brief Get the default value for position
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultPosition() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for slipCoefficients
   * @return SFVec2f The default value
   */
  static SFVec2f getDefaultSlipCoefficients() { return SFVec2f{0, 0}; }

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
  static std::string getDefaultContainerField() { return "contacts"; }

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
   * @brief Gets the value of body1. AccessType: inputOutput
   * @details The body1 and body2 fields specify two top-level nodes that should
   * be evaluated in the physics model as a single set of interactions with
   * respect to each other.
   * @return SFNode The current value of body1.
   */
  SFNode getBody1() const { return _body1; }

  /**
   * @brief Acceptable node types for the body1 field.
   * @details Permitted X3D node types: RigidBody
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableBody1NodeTypes() {
    static const std::vector<std::string> types = {"RigidBody"};
    return types;
  }

  /**
   * @brief Sets the value of body1. AccessType: inputOutput
   * @details The body1 and body2 fields specify two top-level nodes that should
   * be evaluated in the physics model as a single set of interactions with
   * respect to each other.
   * @param value The new value for body1.
   */
  void setBody1(const SFNode &value) { _body1 = value; }

  void setBody1(SFNode &&value) { _body1 = std::move(value); }

  /**
   * @brief Gets the value of body2. AccessType: inputOutput
   * @details The body1 and body2 fields specify two top-level nodes that should
   * be evaluated in the physics model as a single set of interactions with
   * respect to each other.
   * @return SFNode The current value of body2.
   */
  SFNode getBody2() const { return _body2; }

  /**
   * @brief Acceptable node types for the body2 field.
   * @details Permitted X3D node types: RigidBody
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableBody2NodeTypes() {
    static const std::vector<std::string> types = {"RigidBody"};
    return types;
  }

  /**
   * @brief Sets the value of body2. AccessType: inputOutput
   * @details The body1 and body2 fields specify two top-level nodes that should
   * be evaluated in the physics model as a single set of interactions with
   * respect to each other.
   * @param value The new value for body2.
   */
  void setBody2(const SFNode &value) { _body2 = value; }

  void setBody2(SFNode &&value) { _body2 = std::move(value); }

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
   * @brief Gets the value of contactNormal. AccessType: inputOutput
   * @details contactNormal is unit vector describing normal between two
   * colliding bodies.
   * @return SFVec3f The current value of contactNormal.
   */
  SFVec3f getContactNormal() const { return _contactNormal; }

  /**
   * @brief Sets the value of contactNormal. AccessType: inputOutput
   * @details contactNormal is unit vector describing normal between two
   * colliding bodies.
   * @param value The new value for contactNormal.
   */
  void setContactNormal(const SFVec3f &value) { _contactNormal = value; }

  void setContactNormal(SFVec3f &&value) { _contactNormal = std::move(value); }

  /**
   * @brief Gets the value of depth. AccessType: inputOutput
   * @details depth indicates how deep the current intersection is along normal
   * vector.
   * @return SFFloat The current value of depth.
   */
  SFFloat getDepth() const { return _depth; }

  /**
   * @brief Sets the value of depth. AccessType: inputOutput
   * @details depth indicates how deep the current intersection is along normal
   * vector.
   * @param value The new value for depth.
   */
  void setDepth(const SFFloat &value) { _depth = value; }

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
   * @brief Gets the value of frictionDirection. AccessType: inputOutput
   * @details frictionDirection controls friction vector.
   * @return SFVec3f The current value of frictionDirection.
   */
  SFVec3f getFrictionDirection() const { return _frictionDirection; }

  /**
   * @brief Sets the value of frictionDirection. AccessType: inputOutput
   * @details frictionDirection controls friction vector.
   * @param value The new value for frictionDirection.
   */
  void setFrictionDirection(const SFVec3f &value) {

    _frictionDirection = value;
  }

  void setFrictionDirection(SFVec3f &&value) {

    _frictionDirection = std::move(value);
  }

  /**
   * @brief Gets the value of geometry1. AccessType: inputOutput
   * @details The geometry1 and geometry2 fields specify collision-related
   * information about body1 and body2.
   * @return SFNode The current value of geometry1.
   */
  SFNode getGeometry1() const { return _geometry1; }

  /**
   * @brief Acceptable node types for the geometry1 field.
   * @details Permitted X3D node types: X3DNBodyCollidableNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableGeometry1NodeTypes() {
    static const std::vector<std::string> types = {"X3DNBodyCollidableNode"};
    return types;
  }

  /**
   * @brief Sets the value of geometry1. AccessType: inputOutput
   * @details The geometry1 and geometry2 fields specify collision-related
   * information about body1 and body2.
   * @param value The new value for geometry1.
   */
  void setGeometry1(const SFNode &value) { _geometry1 = value; }

  void setGeometry1(SFNode &&value) { _geometry1 = std::move(value); }

  /**
   * @brief Gets the value of geometry2. AccessType: inputOutput
   * @details The geometry1 and geometry2 fields specify collision-related
   * information about body1 and body2.
   * @return SFNode The current value of geometry2.
   */
  SFNode getGeometry2() const { return _geometry2; }

  /**
   * @brief Acceptable node types for the geometry2 field.
   * @details Permitted X3D node types: X3DNBodyCollidableNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableGeometry2NodeTypes() {
    static const std::vector<std::string> types = {"X3DNBodyCollidableNode"};
    return types;
  }

  /**
   * @brief Sets the value of geometry2. AccessType: inputOutput
   * @details The geometry1 and geometry2 fields specify collision-related
   * information about body1 and body2.
   * @param value The new value for geometry2.
   */
  void setGeometry2(const SFNode &value) { _geometry2 = value; }

  void setGeometry2(SFNode &&value) { _geometry2 = std::move(value); }

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
   * @brief Gets the value of position. AccessType: inputOutput
   * @details position (x, y, z in meters) of exact location of collision.
   * @return SFVec3f The current value of position.
   */
  SFVec3f getPosition() const { return _position; }

  /**
   * @brief Sets the value of position. AccessType: inputOutput
   * @details position (x, y, z in meters) of exact location of collision.
   * @param value The new value for position.
   */
  void setPosition(const SFVec3f &value) { _position = value; }

  void setPosition(SFVec3f &&value) { _position = std::move(value); }

  /**
   * @brief Gets the value of slipCoefficients. AccessType: inputOutput
   * @details slipCoefficients used for computing surface drag.
   * @return SFVec2f The current value of slipCoefficients.
   */
  SFVec2f getSlipCoefficients() const { return _slipCoefficients; }

  /**
   * @brief Sets the value of slipCoefficients. AccessType: inputOutput
   * @details slipCoefficients used for computing surface drag.
   * @param value The new value for slipCoefficients.
   */
  void setSlipCoefficients(const SFVec2f &value) { _slipCoefficients = value; }

  void setSlipCoefficients(SFVec2f &&value) {

    _slipCoefficients = std::move(value);
  }

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
   * @brief The X3D type name of this node (e.g. "Contact").
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
   * @brief Member variable for body1.
   */

  SFNode _body1{nullptr};

  /**
   * @brief Member variable for body2.
   */

  SFNode _body2{nullptr};

  /**
   * @brief Member variable for bounce.
   */

  SFFloat _bounce{0};

  /**
   * @brief Member variable for contactNormal.
   */

  SFVec3f _contactNormal{SFVec3f{0, 1, 0}};

  /**
   * @brief Member variable for depth.
   */

  SFFloat _depth{0};

  /**
   * @brief Member variable for frictionCoefficients.
   */

  SFVec2f _frictionCoefficients{SFVec2f{0, 0}};

  /**
   * @brief Member variable for frictionDirection.
   */

  SFVec3f _frictionDirection{SFVec3f{0, 1, 0}};

  /**
   * @brief Member variable for geometry1.
   */

  SFNode _geometry1{nullptr};

  /**
   * @brief Member variable for geometry2.
   */

  SFNode _geometry2{nullptr};

  /**
   * @brief Member variable for minBounceSpeed.
   */

  SFFloat _minBounceSpeed{0};

  /**
   * @brief Member variable for position.
   */

  SFVec3f _position{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for slipCoefficients.
   */

  SFVec2f _slipCoefficients{SFVec2f{0, 0}};

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

} // namespace x3d::nodes
