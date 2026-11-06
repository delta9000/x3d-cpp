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
  RigidBody() = default;

  ~RigidBody() = default;

  static SFFloat getDefaultAngularDampingFactor() { return 0.001; }

  static SFVec3f getDefaultAngularVelocity() { return SFVec3f{0, 0, 0}; }

  static SFBool getDefaultAutoDamp() { return false; }

  static SFBool getDefaultAutoDisable() { return false; }

  static SFVec3f getDefaultCenterOfMass() { return SFVec3f{0, 0, 0}; }

  static SFFloat getDefaultDisableAngularSpeed() { return 0; }

  static SFFloat getDefaultDisableLinearSpeed() { return 0; }

  static SFTime getDefaultDisableTime() { return 0; }

  static SFBool getDefaultEnabled() { return true; }

  static SFVec3f getDefaultFiniteRotationAxis() { return SFVec3f{0, 1, 0}; }

  static SFBool getDefaultFixed() { return false; }

  static SFMatrix3f getDefaultInertia() {
    return SFMatrix3f{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
  }

  static SFFloat getDefaultLinearDampingFactor() { return 0.001; }

  static SFVec3f getDefaultLinearVelocity() { return SFVec3f{0, 0, 0}; }

  static SFFloat getDefaultMass() { return 1; }

  static SFNode getDefaultMassDensityModel() { return nullptr; }

  static SFRotation getDefaultOrientation() { return SFRotation{0, 0, 1, 0}; }

  static SFVec3f getDefaultPosition() { return SFVec3f{0, 0, 0}; }

  static SFBool getDefaultUseFiniteRotation() { return false; }

  static SFBool getDefaultUseGlobalGravity() { return true; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "bodies"; }

  static bool isValidContainerField(const std::string &value) {
    static const std::vector<std::string> valid_values = {"body1", "body2",
                                                          "bodies"};
    return std::find(valid_values.begin(), valid_values.end(), value) !=
           valid_values.end();
  }

  static std::string componentName() { return "RigidBodyPhysics"; }

  static int componentLevel() { return 2; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details angularDampingFactor automatically damps a portion of body motion
   * over time.
   */
  const SFFloat &getAngularDampingFactor() const {
    return _angularDampingFactor;
  }

  /**
   * Access: inputOutput.
   * @details angularDampingFactor automatically damps a portion of body motion
   * over time.
   */
  void setAngularDampingFactor(const SFFloat &value) {

    _angularDampingFactor = value;
  }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details angularVelocity sets constant velocity value to object every
   * frame, and reports updates by physics model.
   */
  const SFVec3f &getAngularVelocity() const { return _angularVelocity; }

  /**
   * Access: inputOutput.
   * @details angularVelocity sets constant velocity value to object every
   * frame, and reports updates by physics model.
   */
  void setAngularVelocity(const SFVec3f &value) { _angularVelocity = value; }

  void setAngularVelocity(SFVec3f &&value) {

    _angularVelocity = std::move(value);
  }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details autoDamp enables/disables angularDampingFactor and
   * linearDampingFactor.
   */
  const SFBool &getAutoDamp() const { return _autoDamp; }

  /**
   * Access: inputOutput.
   * @details autoDamp enables/disables angularDampingFactor and
   * linearDampingFactor.
   */
  void setAutoDamp(const SFBool &value) { _autoDamp = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details autoDisable toggles operation of disableAngularSpeed,
   * disableLinearSpeed, disableTime.
   */
  const SFBool &getAutoDisable() const { return _autoDisable; }

  /**
   * Access: inputOutput.
   * @details autoDisable toggles operation of disableAngularSpeed,
   * disableLinearSpeed, disableTime.
   */
  void setAutoDisable(const SFBool &value) { _autoDisable = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details centerOfMass defines local center of mass for physics
   * calculations.
   */
  const SFVec3f &getCenterOfMass() const { return _centerOfMass; }

  /**
   * Access: inputOutput.
   * @details centerOfMass defines local center of mass for physics
   * calculations.
   */
  void setCenterOfMass(const SFVec3f &value) { _centerOfMass = value; }

  void setCenterOfMass(SFVec3f &&value) { _centerOfMass = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details disableAngularSpeed defines lower-limit tolerance value when body
   * is considered at rest and not part of rigid body calculations, reducing
   * numeric instabilities.
   */
  const SFFloat &getDisableAngularSpeed() const { return _disableAngularSpeed; }

  /**
   * Access: inputOutput.
   * @details disableAngularSpeed defines lower-limit tolerance value when body
   * is considered at rest and not part of rigid body calculations, reducing
   * numeric instabilities.
   */
  void setDisableAngularSpeed(const SFFloat &value) {

    _disableAngularSpeed = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details disableLinearSpeed defines lower-limit tolerance value when body
   * is considered at rest and not part of rigid body calculation, reducing
   * numeric instabilitiess.
   */
  const SFFloat &getDisableLinearSpeed() const { return _disableLinearSpeed; }

  /**
   * Access: inputOutput.
   * @details disableLinearSpeed defines lower-limit tolerance value when body
   * is considered at rest and not part of rigid body calculation, reducing
   * numeric instabilitiess.
   */
  void setDisableLinearSpeed(const SFFloat &value) {

    _disableLinearSpeed = value;
  }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details disableTime defines interval when body becomes at rest and not
   * part of rigid body calculations, reducing numeric instabilities.
   */
  const SFTime &getDisableTime() const { return _disableTime; }

  /**
   * Access: inputOutput.
   * @details disableTime defines interval when body becomes at rest and not
   * part of rigid body calculations, reducing numeric instabilities.
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
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setDisableTime() for range validation.
   */
  void setDisableTimeUnchecked(const SFTime &value) { _disableTime = value; }

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
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details finiteRotationAxis specifies vector around which the object
   * rotates.
   */
  const SFVec3f &getFiniteRotationAxis() const { return _finiteRotationAxis; }

  /**
   * Access: inputOutput.
   * @details finiteRotationAxis specifies vector around which the object
   * rotates.
   */
  void setFiniteRotationAxis(const SFVec3f &value) {

    _finiteRotationAxis = value;
  }

  void setFiniteRotationAxis(SFVec3f &&value) {

    _finiteRotationAxis = std::move(value);
  }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details fixed indicates whether body is able to move.
   */
  const SFBool &getFixed() const { return _fixed; }

  /**
   * Access: inputOutput.
   * @details fixed indicates whether body is able to move.
   */
  void setFixed(const SFBool &value) { _fixed = value; }

  /**
   * MFVec3f; access: inputOutput. Borrows the stored value.
   * @details forces defines linear force values applied to the object every
   * frame.
   */
  const MFVec3f &getForces() const { return _forces; }

  /**
   * Access: inputOutput.
   * @details forces defines linear force values applied to the object every
   * frame.
   */
  void setForces(const MFVec3f &value) { _forces = value; }

  void setForces(MFVec3f &&value) { _forces = std::move(value); }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details The geometry field is used to connect the body modelled by the
   * physics engine implementation to the real geometry of the scene through the
   * use of collidable nodes.
   */
  const MFNode &getGeometry() const { return _geometry; }

  /**
   * @details Permitted X3D node types: X3DNBodyCollidableNode
   */
  static const std::vector<std::string> &acceptableGeometryNodeTypes() {
    static const std::vector<std::string> types = {"X3DNBodyCollidableNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The geometry field is used to connect the body modelled by the
   * physics engine implementation to the real geometry of the scene through the
   * use of collidable nodes.
   */
  void setGeometry(const MFNode &value) { _geometry = value; }

  void setGeometry(MFNode &&value) { _geometry = std::move(value); }

  /**
   * SFMatrix3f; access: inputOutput. Borrows the stored value.
   * @details inertia matrix defines a 3x2 inertia tensor matrix.
   */
  const SFMatrix3f &getInertia() const { return _inertia; }

  /**
   * Access: inputOutput.
   * @details inertia matrix defines a 3x2 inertia tensor matrix.
   */
  void setInertia(const SFMatrix3f &value) { _inertia = value; }

  void setInertia(SFMatrix3f &&value) { _inertia = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details linearDampingFactor automatically damps a portion of body motion
   * over time.
   */
  const SFFloat &getLinearDampingFactor() const { return _linearDampingFactor; }

  /**
   * Access: inputOutput.
   * @details linearDampingFactor automatically damps a portion of body motion
   * over time.
   */
  void setLinearDampingFactor(const SFFloat &value) {

    _linearDampingFactor = value;
  }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details linearVelocity sets constant velocity value to object every frame,
   * and reports updates by physics model.
   */
  const SFVec3f &getLinearVelocity() const { return _linearVelocity; }

  /**
   * Access: inputOutput.
   * @details linearVelocity sets constant velocity value to object every frame,
   * and reports updates by physics model.
   */
  void setLinearVelocity(const SFVec3f &value) { _linearVelocity = value; }

  void setLinearVelocity(SFVec3f &&value) {

    _linearVelocity = std::move(value);
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details mass of the body in kilograms.
   */
  const SFFloat &getMass() const { return _mass; }

  /**
   * Access: inputOutput.
   * @details mass of the body in kilograms.
   */
  void setMass(const SFFloat &value) { _mass = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details The massDensityModel field is used to describe the geometry type
   * and dimensions used to calculate the mass density in the physics model.
   */
  const SFNode &getMassDensityModel() const { return _massDensityModel; }

  /**
   * @details Permitted X3D node types: Sphere, Box, Cone
   */
  static const std::vector<std::string> &acceptableMassDensityModelNodeTypes() {
    static const std::vector<std::string> types = {"Sphere", "Box", "Cone"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The massDensityModel field is used to describe the geometry type
   * and dimensions used to calculate the mass density in the physics model.
   */
  void setMassDensityModel(const SFNode &value) { _massDensityModel = value; }

  void setMassDensityModel(SFNode &&value) {

    _massDensityModel = std::move(value);
  }

  /**
   * SFRotation; access: inputOutput. Borrows the stored value.
   * @details orientation sets body direction in world space, then reports
   * physics updates.
   */
  const SFRotation &getOrientation() const { return _orientation; }

  /**
   * Access: inputOutput.
   * @details orientation sets body direction in world space, then reports
   * physics updates.
   */
  void setOrientation(const SFRotation &value) { _orientation = value; }

  void setOrientation(SFRotation &&value) { _orientation = std::move(value); }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details position sets body location in world space, then reports physics
   * updates.
   */
  const SFVec3f &getPosition() const { return _position; }

  /**
   * Access: inputOutput.
   * @details position sets body location in world space, then reports physics
   * updates.
   */
  void setPosition(const SFVec3f &value) { _position = value; }

  void setPosition(SFVec3f &&value) { _position = std::move(value); }

  /**
   * MFVec3f; access: inputOutput. Borrows the stored value.
   * @details torques defines rotational force values applied to the object
   * every frame.
   */
  const MFVec3f &getTorques() const { return _torques; }

  /**
   * Access: inputOutput.
   * @details torques defines rotational force values applied to the object
   * every frame.
   */
  void setTorques(const MFVec3f &value) { _torques = value; }

  void setTorques(MFVec3f &&value) { _torques = std::move(value); }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details useFiniteRotation enables/disables higher-resolution, higher-cost
   * computational method for calculating rotations.
   */
  const SFBool &getUseFiniteRotation() const { return _useFiniteRotation; }

  /**
   * Access: inputOutput.
   * @details useFiniteRotation enables/disables higher-resolution, higher-cost
   * computational method for calculating rotations.
   */
  void setUseFiniteRotation(const SFBool &value) { _useFiniteRotation = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details useGlobalGravity indicates whether this particular body is
   * influenced by parent RigidBodyCollection's gravity setting.
   */
  const SFBool &getUseGlobalGravity() const { return _useGlobalGravity; }

  /**
   * Access: inputOutput.
   * @details useGlobalGravity indicates whether this particular body is
   * influenced by parent RigidBodyCollection's gravity setting.
   */
  void setUseGlobalGravity(const SFBool &value) { _useGlobalGravity = value; }

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
  static void checkRangesDisableTime(const SFTime &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

private:
  static void validateDisableTime(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("disableTime below minimum of 0");
  }

  SFFloat _angularDampingFactor{0.001};

  SFVec3f _angularVelocity{SFVec3f{0, 0, 0}};

  SFBool _autoDamp{false};

  SFBool _autoDisable{false};

  SFVec3f _centerOfMass{SFVec3f{0, 0, 0}};

  SFFloat _disableAngularSpeed{0};

  SFFloat _disableLinearSpeed{0};

  SFTime _disableTime{0};

  SFBool _enabled{true};

  SFVec3f _finiteRotationAxis{SFVec3f{0, 1, 0}};

  SFBool _fixed{false};

  MFVec3f _forces{};

  MFNode _geometry{};

  SFMatrix3f _inertia{SFMatrix3f{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}}};

  SFFloat _linearDampingFactor{0.001};

  SFVec3f _linearVelocity{SFVec3f{0, 0, 0}};

  SFFloat _mass{1};

  SFNode _massDensityModel{nullptr};

  SFRotation _orientation{SFRotation{0, 0, 1, 0}};

  SFVec3f _position{SFVec3f{0, 0, 0}};

  MFVec3f _torques{};

  SFBool _useFiniteRotation{false};

  SFBool _useGlobalGravity{true};
};

} // namespace x3d::nodes
