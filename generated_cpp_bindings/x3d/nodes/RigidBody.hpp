// RigidBody.hpp
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
 * @class RigidBody
 * @brief RigidBody describes a collection of shapes with a mass distribution
 * that is affected by the physics model.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#RigidBody
 */
class RigidBody : public virtual X3DChildNode, public virtual X3DBoundedObject {
public:
  /**
   * @brief Default constructor for RigidBody
   */
  RigidBody() = default;

  /**
   * @brief Destructor for RigidBody
   */
  ~RigidBody() = default;

  /**
   * @brief Get the default value for angularDampingFactor
   * @return SFFloat The default value
   */
  static SFFloat getDefaultAngularDampingFactor() { return 0.001; }

  /**
   * @brief Get the default value for angularVelocity
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultAngularVelocity() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for autoDamp
   * @return SFBool The default value
   */
  static SFBool getDefaultAutoDamp() { return false; }

  /**
   * @brief Get the default value for autoDisable
   * @return SFBool The default value
   */
  static SFBool getDefaultAutoDisable() { return false; }

  /**
   * @brief Get the default value for centerOfMass
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultCenterOfMass() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for disableAngularSpeed
   * @return SFFloat The default value
   */
  static SFFloat getDefaultDisableAngularSpeed() { return 0; }

  /**
   * @brief Get the default value for disableLinearSpeed
   * @return SFFloat The default value
   */
  static SFFloat getDefaultDisableLinearSpeed() { return 0; }

  /**
   * @brief Get the default value for disableTime
   * @return SFTime The default value
   */
  static SFTime getDefaultDisableTime() { return 0; }

  /**
   * @brief Get the default value for enabled
   * @return SFBool The default value
   */
  static SFBool getDefaultEnabled() { return true; }

  /**
   * @brief Get the default value for finiteRotationAxis
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultFiniteRotationAxis() { return SFVec3f{0, 1, 0}; }

  /**
   * @brief Get the default value for fixed
   * @return SFBool The default value
   */
  static SFBool getDefaultFixed() { return false; }

  /**
   * @brief Get the default value for inertia
   * @return SFMatrix3f The default value
   */
  static SFMatrix3f getDefaultInertia() {
    return SFMatrix3f{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
  }

  /**
   * @brief Get the default value for linearDampingFactor
   * @return SFFloat The default value
   */
  static SFFloat getDefaultLinearDampingFactor() { return 0.001; }

  /**
   * @brief Get the default value for linearVelocity
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultLinearVelocity() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for mass
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMass() { return 1; }

  /**
   * @brief Get the default value for massDensityModel
   * @return SFNode The default value
   */
  static SFNode getDefaultMassDensityModel() { return nullptr; }

  /**
   * @brief Get the default value for orientation
   * @return SFRotation The default value
   */
  static SFRotation getDefaultOrientation() { return SFRotation{0, 0, 1, 0}; }

  /**
   * @brief Get the default value for position
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultPosition() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for useFiniteRotation
   * @return SFBool The default value
   */
  static SFBool getDefaultUseFiniteRotation() { return false; }

  /**
   * @brief Get the default value for useGlobalGravity
   * @return SFBool The default value
   */
  static SFBool getDefaultUseGlobalGravity() { return true; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "bodies"; }

  /**
   * @brief Check if a container field value is valid
   * @param value The value to check
   * @return bool True if the value is valid
   */
  static bool isValidContainerField(const std::string &value) {
    static const std::vector<std::string> valid_values = {"body1", "body2",
                                                          "bodies"};
    return std::find(valid_values.begin(), valid_values.end(), value) !=
           valid_values.end();
  }

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
   * @brief Gets the value of angularDampingFactor. AccessType: inputOutput
   * @details angularDampingFactor automatically damps a portion of body motion
   * over time.
   * @return SFFloat The current value of angularDampingFactor.
   */
  SFFloat getAngularDampingFactor() const { return _angularDampingFactor; }

  /**
   * @brief Sets the value of angularDampingFactor. AccessType: inputOutput
   * @details angularDampingFactor automatically damps a portion of body motion
   * over time.
   * @param value The new value for angularDampingFactor.
   */
  void setAngularDampingFactor(const SFFloat &value) {

    _angularDampingFactor = value;
  }

  /**
   * @brief Gets the value of angularVelocity. AccessType: inputOutput
   * @details angularVelocity sets constant velocity value to object every
   * frame, and reports updates by physics model.
   * @return SFVec3f The current value of angularVelocity.
   */
  SFVec3f getAngularVelocity() const { return _angularVelocity; }

  /**
   * @brief Sets the value of angularVelocity. AccessType: inputOutput
   * @details angularVelocity sets constant velocity value to object every
   * frame, and reports updates by physics model.
   * @param value The new value for angularVelocity.
   */
  void setAngularVelocity(const SFVec3f &value) { _angularVelocity = value; }

  void setAngularVelocity(SFVec3f &&value) {

    _angularVelocity = std::move(value);
  }

  /**
   * @brief Gets the value of autoDamp. AccessType: inputOutput
   * @details autoDamp enables/disables angularDampingFactor and
   * linearDampingFactor.
   * @return SFBool The current value of autoDamp.
   */
  SFBool getAutoDamp() const { return _autoDamp; }

  /**
   * @brief Sets the value of autoDamp. AccessType: inputOutput
   * @details autoDamp enables/disables angularDampingFactor and
   * linearDampingFactor.
   * @param value The new value for autoDamp.
   */
  void setAutoDamp(const SFBool &value) { _autoDamp = value; }

  /**
   * @brief Gets the value of autoDisable. AccessType: inputOutput
   * @details autoDisable toggles operation of disableAngularSpeed,
   * disableLinearSpeed, disableTime.
   * @return SFBool The current value of autoDisable.
   */
  SFBool getAutoDisable() const { return _autoDisable; }

  /**
   * @brief Sets the value of autoDisable. AccessType: inputOutput
   * @details autoDisable toggles operation of disableAngularSpeed,
   * disableLinearSpeed, disableTime.
   * @param value The new value for autoDisable.
   */
  void setAutoDisable(const SFBool &value) { _autoDisable = value; }

  /**
   * @brief Gets the value of centerOfMass. AccessType: inputOutput
   * @details centerOfMass defines local center of mass for physics
   * calculations.
   * @return SFVec3f The current value of centerOfMass.
   */
  SFVec3f getCenterOfMass() const { return _centerOfMass; }

  /**
   * @brief Sets the value of centerOfMass. AccessType: inputOutput
   * @details centerOfMass defines local center of mass for physics
   * calculations.
   * @param value The new value for centerOfMass.
   */
  void setCenterOfMass(const SFVec3f &value) { _centerOfMass = value; }

  void setCenterOfMass(SFVec3f &&value) { _centerOfMass = std::move(value); }

  /**
   * @brief Gets the value of disableAngularSpeed. AccessType: inputOutput
   * @details disableAngularSpeed defines lower-limit tolerance value when body
   * is considered at rest and not part of rigid body calculations, reducing
   * numeric instabilities.
   * @return SFFloat The current value of disableAngularSpeed.
   */
  SFFloat getDisableAngularSpeed() const { return _disableAngularSpeed; }

  /**
   * @brief Sets the value of disableAngularSpeed. AccessType: inputOutput
   * @details disableAngularSpeed defines lower-limit tolerance value when body
   * is considered at rest and not part of rigid body calculations, reducing
   * numeric instabilities.
   * @param value The new value for disableAngularSpeed.
   */
  void setDisableAngularSpeed(const SFFloat &value) {

    _disableAngularSpeed = value;
  }

  /**
   * @brief Gets the value of disableLinearSpeed. AccessType: inputOutput
   * @details disableLinearSpeed defines lower-limit tolerance value when body
   * is considered at rest and not part of rigid body calculation, reducing
   * numeric instabilitiess.
   * @return SFFloat The current value of disableLinearSpeed.
   */
  SFFloat getDisableLinearSpeed() const { return _disableLinearSpeed; }

  /**
   * @brief Sets the value of disableLinearSpeed. AccessType: inputOutput
   * @details disableLinearSpeed defines lower-limit tolerance value when body
   * is considered at rest and not part of rigid body calculation, reducing
   * numeric instabilitiess.
   * @param value The new value for disableLinearSpeed.
   */
  void setDisableLinearSpeed(const SFFloat &value) {

    _disableLinearSpeed = value;
  }

  /**
   * @brief Gets the value of disableTime. AccessType: inputOutput
   * @details disableTime defines interval when body becomes at rest and not
   * part of rigid body calculations, reducing numeric instabilities.
   * @return SFTime The current value of disableTime.
   */
  SFTime getDisableTime() const { return _disableTime; }

  /**
   * @brief Sets the value of disableTime. AccessType: inputOutput
   * @details disableTime defines interval when body becomes at rest and not
   * part of rigid body calculations, reducing numeric instabilities.
   * @param value The new value for disableTime.
   */
  void setDisableTime(const SFTime &value) {

    validateDisableTime(value);

    _disableTime = value;
  }

  void setDisableTime(SFTime &&value) {

    validateDisableTime(value);

    _disableTime = std::move(value);
  }

  /**
   * @brief Non-validating write of disableTime (runtime/reader ingest path).
   * @details Assigns without the range check setDisableTime() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setDisableTime() stays the
   *          enforcement point for programmatic callers.
   */
  void setDisableTimeUnchecked(const SFTime &value) { _disableTime = value; }

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
   * @brief Gets the value of finiteRotationAxis. AccessType: inputOutput
   * @details finiteRotationAxis specifies vector around which the object
   * rotates.
   * @return SFVec3f The current value of finiteRotationAxis.
   */
  SFVec3f getFiniteRotationAxis() const { return _finiteRotationAxis; }

  /**
   * @brief Sets the value of finiteRotationAxis. AccessType: inputOutput
   * @details finiteRotationAxis specifies vector around which the object
   * rotates.
   * @param value The new value for finiteRotationAxis.
   */
  void setFiniteRotationAxis(const SFVec3f &value) {

    _finiteRotationAxis = value;
  }

  void setFiniteRotationAxis(SFVec3f &&value) {

    _finiteRotationAxis = std::move(value);
  }

  /**
   * @brief Gets the value of fixed. AccessType: inputOutput
   * @details fixed indicates whether body is able to move.
   * @return SFBool The current value of fixed.
   */
  SFBool getFixed() const { return _fixed; }

  /**
   * @brief Sets the value of fixed. AccessType: inputOutput
   * @details fixed indicates whether body is able to move.
   * @param value The new value for fixed.
   */
  void setFixed(const SFBool &value) { _fixed = value; }

  /**
   * @brief Gets the value of forces. AccessType: inputOutput
   * @details forces defines linear force values applied to the object every
   * frame.
   * @return MFVec3f The current value of forces.
   */
  MFVec3f getForces() const { return _forces; }

  /**
   * @brief Sets the value of forces. AccessType: inputOutput
   * @details forces defines linear force values applied to the object every
   * frame.
   * @param value The new value for forces.
   */
  void setForces(const MFVec3f &value) { _forces = value; }

  void setForces(MFVec3f &&value) { _forces = std::move(value); }

  /**
   * @brief Gets the value of geometry. AccessType: inputOutput
   * @details The geometry field is used to connect the body modelled by the
   * physics engine implementation to the real geometry of the scene through the
   * use of collidable nodes.
   * @return MFNode The current value of geometry.
   */
  MFNode getGeometry() const { return _geometry; }

  /**
   * @brief Acceptable node types for the geometry field.
   * @details Permitted X3D node types: X3DNBodyCollidableNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableGeometryNodeTypes() {
    static const std::vector<std::string> types = {"X3DNBodyCollidableNode"};
    return types;
  }

  /**
   * @brief Sets the value of geometry. AccessType: inputOutput
   * @details The geometry field is used to connect the body modelled by the
   * physics engine implementation to the real geometry of the scene through the
   * use of collidable nodes.
   * @param value The new value for geometry.
   */
  void setGeometry(const MFNode &value) { _geometry = value; }

  void setGeometry(MFNode &&value) { _geometry = std::move(value); }

  /**
   * @brief Gets the value of inertia. AccessType: inputOutput
   * @details inertia matrix defines a 3x2 inertia tensor matrix.
   * @return SFMatrix3f The current value of inertia.
   */
  SFMatrix3f getInertia() const { return _inertia; }

  /**
   * @brief Sets the value of inertia. AccessType: inputOutput
   * @details inertia matrix defines a 3x2 inertia tensor matrix.
   * @param value The new value for inertia.
   */
  void setInertia(const SFMatrix3f &value) { _inertia = value; }

  void setInertia(SFMatrix3f &&value) { _inertia = std::move(value); }

  /**
   * @brief Gets the value of linearDampingFactor. AccessType: inputOutput
   * @details linearDampingFactor automatically damps a portion of body motion
   * over time.
   * @return SFFloat The current value of linearDampingFactor.
   */
  SFFloat getLinearDampingFactor() const { return _linearDampingFactor; }

  /**
   * @brief Sets the value of linearDampingFactor. AccessType: inputOutput
   * @details linearDampingFactor automatically damps a portion of body motion
   * over time.
   * @param value The new value for linearDampingFactor.
   */
  void setLinearDampingFactor(const SFFloat &value) {

    _linearDampingFactor = value;
  }

  /**
   * @brief Gets the value of linearVelocity. AccessType: inputOutput
   * @details linearVelocity sets constant velocity value to object every frame,
   * and reports updates by physics model.
   * @return SFVec3f The current value of linearVelocity.
   */
  SFVec3f getLinearVelocity() const { return _linearVelocity; }

  /**
   * @brief Sets the value of linearVelocity. AccessType: inputOutput
   * @details linearVelocity sets constant velocity value to object every frame,
   * and reports updates by physics model.
   * @param value The new value for linearVelocity.
   */
  void setLinearVelocity(const SFVec3f &value) { _linearVelocity = value; }

  void setLinearVelocity(SFVec3f &&value) {

    _linearVelocity = std::move(value);
  }

  /**
   * @brief Gets the value of mass. AccessType: inputOutput
   * @details mass of the body in kilograms.
   * @return SFFloat The current value of mass.
   */
  SFFloat getMass() const { return _mass; }

  /**
   * @brief Sets the value of mass. AccessType: inputOutput
   * @details mass of the body in kilograms.
   * @param value The new value for mass.
   */
  void setMass(const SFFloat &value) { _mass = value; }

  /**
   * @brief Gets the value of massDensityModel. AccessType: inputOutput
   * @details The massDensityModel field is used to describe the geometry type
   * and dimensions used to calculate the mass density in the physics model.
   * @return SFNode The current value of massDensityModel.
   */
  SFNode getMassDensityModel() const { return _massDensityModel; }

  /**
   * @brief Acceptable node types for the massDensityModel field.
   * @details Permitted X3D node types: Sphere, Box, Cone
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableMassDensityModelNodeTypes() {
    static const std::vector<std::string> types = {"Sphere", "Box", "Cone"};
    return types;
  }

  /**
   * @brief Sets the value of massDensityModel. AccessType: inputOutput
   * @details The massDensityModel field is used to describe the geometry type
   * and dimensions used to calculate the mass density in the physics model.
   * @param value The new value for massDensityModel.
   */
  void setMassDensityModel(const SFNode &value) { _massDensityModel = value; }

  void setMassDensityModel(SFNode &&value) {

    _massDensityModel = std::move(value);
  }

  /**
   * @brief Gets the value of orientation. AccessType: inputOutput
   * @details orientation sets body direction in world space, then reports
   * physics updates.
   * @return SFRotation The current value of orientation.
   */
  SFRotation getOrientation() const { return _orientation; }

  /**
   * @brief Sets the value of orientation. AccessType: inputOutput
   * @details orientation sets body direction in world space, then reports
   * physics updates.
   * @param value The new value for orientation.
   */
  void setOrientation(const SFRotation &value) { _orientation = value; }

  void setOrientation(SFRotation &&value) { _orientation = std::move(value); }

  /**
   * @brief Gets the value of position. AccessType: inputOutput
   * @details position sets body location in world space, then reports physics
   * updates.
   * @return SFVec3f The current value of position.
   */
  SFVec3f getPosition() const { return _position; }

  /**
   * @brief Sets the value of position. AccessType: inputOutput
   * @details position sets body location in world space, then reports physics
   * updates.
   * @param value The new value for position.
   */
  void setPosition(const SFVec3f &value) { _position = value; }

  void setPosition(SFVec3f &&value) { _position = std::move(value); }

  /**
   * @brief Gets the value of torques. AccessType: inputOutput
   * @details torques defines rotational force values applied to the object
   * every frame.
   * @return MFVec3f The current value of torques.
   */
  MFVec3f getTorques() const { return _torques; }

  /**
   * @brief Sets the value of torques. AccessType: inputOutput
   * @details torques defines rotational force values applied to the object
   * every frame.
   * @param value The new value for torques.
   */
  void setTorques(const MFVec3f &value) { _torques = value; }

  void setTorques(MFVec3f &&value) { _torques = std::move(value); }

  /**
   * @brief Gets the value of useFiniteRotation. AccessType: inputOutput
   * @details useFiniteRotation enables/disables higher-resolution, higher-cost
   * computational method for calculating rotations.
   * @return SFBool The current value of useFiniteRotation.
   */
  SFBool getUseFiniteRotation() const { return _useFiniteRotation; }

  /**
   * @brief Sets the value of useFiniteRotation. AccessType: inputOutput
   * @details useFiniteRotation enables/disables higher-resolution, higher-cost
   * computational method for calculating rotations.
   * @param value The new value for useFiniteRotation.
   */
  void setUseFiniteRotation(const SFBool &value) { _useFiniteRotation = value; }

  /**
   * @brief Gets the value of useGlobalGravity. AccessType: inputOutput
   * @details useGlobalGravity indicates whether this particular body is
   * influenced by parent RigidBodyCollection's gravity setting.
   * @return SFBool The current value of useGlobalGravity.
   */
  SFBool getUseGlobalGravity() const { return _useGlobalGravity; }

  /**
   * @brief Sets the value of useGlobalGravity. AccessType: inputOutput
   * @details useGlobalGravity indicates whether this particular body is
   * influenced by parent RigidBodyCollection's gravity setting.
   * @param value The new value for useGlobalGravity.
   */
  void setUseGlobalGravity(const SFBool &value) { _useGlobalGravity = value; }

  /**
   * @brief The X3D type name of this node (e.g. "RigidBody").
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
  static void checkRangesDisableTime(const SFTime &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

private:
  static void validateDisableTime(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("disableTime below minimum of 0");
  }

  /**
   * @brief Member variable for angularDampingFactor.
   */

  SFFloat _angularDampingFactor{0.001};

  /**
   * @brief Member variable for angularVelocity.
   */

  SFVec3f _angularVelocity{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for autoDamp.
   */

  SFBool _autoDamp{false};

  /**
   * @brief Member variable for autoDisable.
   */

  SFBool _autoDisable{false};

  /**
   * @brief Member variable for centerOfMass.
   */

  SFVec3f _centerOfMass{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for disableAngularSpeed.
   */

  SFFloat _disableAngularSpeed{0};

  /**
   * @brief Member variable for disableLinearSpeed.
   */

  SFFloat _disableLinearSpeed{0};

  /**
   * @brief Member variable for disableTime.
   */

  SFTime _disableTime{0};

  /**
   * @brief Member variable for enabled.
   */

  SFBool _enabled{true};

  /**
   * @brief Member variable for finiteRotationAxis.
   */

  SFVec3f _finiteRotationAxis{SFVec3f{0, 1, 0}};

  /**
   * @brief Member variable for fixed.
   */

  SFBool _fixed{false};

  /**
   * @brief Member variable for forces.
   */

  MFVec3f _forces{};

  /**
   * @brief Member variable for geometry.
   */

  MFNode _geometry{};

  /**
   * @brief Member variable for inertia.
   */

  SFMatrix3f _inertia{SFMatrix3f{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}}};

  /**
   * @brief Member variable for linearDampingFactor.
   */

  SFFloat _linearDampingFactor{0.001};

  /**
   * @brief Member variable for linearVelocity.
   */

  SFVec3f _linearVelocity{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for mass.
   */

  SFFloat _mass{1};

  /**
   * @brief Member variable for massDensityModel.
   */

  SFNode _massDensityModel{nullptr};

  /**
   * @brief Member variable for orientation.
   */

  SFRotation _orientation{SFRotation{0, 0, 1, 0}};

  /**
   * @brief Member variable for position.
   */

  SFVec3f _position{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for torques.
   */

  MFVec3f _torques{};

  /**
   * @brief Member variable for useFiniteRotation.
   */

  SFBool _useFiniteRotation{false};

  /**
   * @brief Member variable for useGlobalGravity.
   */

  SFBool _useGlobalGravity{true};
};

} // namespace x3d::nodes
