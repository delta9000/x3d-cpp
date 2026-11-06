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
 * @class RigidBodyCollection
 * @brief RigidBodyCollection represents a system of bodies that interact within
 * a single physics model.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#RigidBodyCollection
 */
class RigidBodyCollection : public virtual X3DChildNode,
                            public virtual X3DBoundedObject {
public:
  RigidBodyCollection() = default;

  ~RigidBodyCollection() = default;

  static SFBool getDefaultAutoDisable() { return false; }

  static SFNode getDefaultCollider() { return nullptr; }

  static SFFloat getDefaultConstantForceMix() { return 0.0001; }

  static SFFloat getDefaultContactSurfaceThickness() { return 0; }

  static SFFloat getDefaultDisableAngularSpeed() { return 0; }

  static SFFloat getDefaultDisableLinearSpeed() { return 0; }

  static SFTime getDefaultDisableTime() { return 0; }

  static SFBool getDefaultEnabled() { return true; }

  static SFFloat getDefaultErrorCorrection() { return 0.8; }

  static SFVec3f getDefaultGravity() { return SFVec3f{0, -9.8, 0}; }

  static SFInt32 getDefaultIterations() { return 10; }

  static SFFloat getDefaultMaxCorrectionSpeed() { return -1; }

  static SFBool getDefaultPreferAccuracy() { return false; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "RigidBodyPhysics"; }

  static int componentLevel() { return 2; }

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
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details Collection of top-level nodes that comprise a set of bodies
   * evaluated as a single set of interactions.
   */
  const MFNode &getBodies() const { return _bodies; }

  /**
   * @details Permitted X3D node types: RigidBody
   */
  static const std::vector<std::string> &acceptableBodiesNodeTypes() {
    static const std::vector<std::string> types = {"RigidBody"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Collection of top-level nodes that comprise a set of bodies
   * evaluated as a single set of interactions.
   */
  void setBodies(const MFNode &value) { _bodies = value; }

  void setBodies(MFNode &&value) { _bodies = std::move(value); }

  /**
   * SFNode; access: initializeOnly. Borrows the stored value.
   * @details The collider field associates a collision collection with this
   * rigid body collection allowing seamless updates and integration without the
   * need to use the X3D event model.
   */
  const SFNode &getCollider() const { return _collider; }

  /**
   * @details Permitted X3D node types: CollisionCollection
   */
  static const std::vector<std::string> &acceptableColliderNodeTypes() {
    static const std::vector<std::string> types = {"CollisionCollection"};
    return types;
  }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setColliderUnchecked(const SFNode &value) { _collider = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details constantForceMix modifies damping calculations by violating normal
   * constraints while applying small, constant forces in those calculations.
   */
  const SFFloat &getConstantForceMix() const { return _constantForceMix; }

  /**
   * Access: inputOutput.
   * @details constantForceMix modifies damping calculations by violating normal
   * constraints while applying small, constant forces in those calculations.
   */
  void setConstantForceMix(const SFFloat &value) { _constantForceMix = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details contactSurfaceThickness defines how far bodies may interpenetrate
   * after a collision, allowing simulation of softer bodies that deform
   * somewhat during collision.
   */
  const SFFloat &getContactSurfaceThickness() const {
    return _contactSurfaceThickness;
  }

  /**
   * Access: inputOutput.
   * @details contactSurfaceThickness defines how far bodies may interpenetrate
   * after a collision, allowing simulation of softer bodies that deform
   * somewhat during collision.
   */
  void setContactSurfaceThickness(const SFFloat &value) {

    _contactSurfaceThickness = value;
  }

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
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details errorCorrection describes how quickly intersection errors due to
   * floating-point inaccuracies are resolved (0=no correction, 1=all corrected
   * in single step).
   */
  const SFFloat &getErrorCorrection() const { return _errorCorrection; }

  /**
   * Access: inputOutput.
   * @details errorCorrection describes how quickly intersection errors due to
   * floating-point inaccuracies are resolved (0=no correction, 1=all corrected
   * in single step).
   */
  void setErrorCorrection(const SFFloat &value) { _errorCorrection = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details gravity indicates direction and strength of local gravity vector
   * for this collection of bodies (units m/sec^2).
   */
  const SFVec3f &getGravity() const { return _gravity; }

  /**
   * Access: inputOutput.
   * @details gravity indicates direction and strength of local gravity vector
   * for this collection of bodies (units m/sec^2).
   */
  void setGravity(const SFVec3f &value) { _gravity = value; }

  void setGravity(SFVec3f &&value) { _gravity = std::move(value); }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details iterations controls number of iterations performed over
   * collectioned joints and bodies during each evaluation.
   */
  const SFInt32 &getIterations() const { return _iterations; }

  /**
   * Access: inputOutput.
   * @details iterations controls number of iterations performed over
   * collectioned joints and bodies during each evaluation.
   */
  void setIterations(const SFInt32 &value) { _iterations = value; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details The joints field is used to register all joints between bodies
   * contained in this collection.
   */
  const MFNode &getJoints() const { return _joints; }

  /**
   * @details Permitted X3D node types: X3DRigidJointNode
   */
  static const std::vector<std::string> &acceptableJointsNodeTypes() {
    static const std::vector<std::string> types = {"X3DRigidJointNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The joints field is used to register all joints between bodies
   * contained in this collection.
   */
  void setJoints(const MFNode &value) { _joints = value; }

  void setJoints(MFNode &&value) { _joints = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details or -1, maxCorrectionSpeed.
   */
  const SFFloat &getMaxCorrectionSpeed() const { return _maxCorrectionSpeed; }

  /**
   * Access: inputOutput.
   * @details or -1, maxCorrectionSpeed.
   */
  void setMaxCorrectionSpeed(const SFFloat &value) {

    _maxCorrectionSpeed = value;
  }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details preferAccuracy provides hint for performance preference: higher
   * accuracy or faster computational speed.
   */
  const SFBool &getPreferAccuracy() const { return _preferAccuracy; }

  /**
   * Access: inputOutput.
   * @details preferAccuracy provides hint for performance preference: higher
   * accuracy or faster computational speed.
   */
  void setPreferAccuracy(const SFBool &value) { _preferAccuracy = value; }

  /**
   * @details Permitted X3D node types: Contact
   */
  static const std::vector<std::string> &acceptableSet_contactsNodeTypes() {
    static const std::vector<std::string> types = {"Contact"};
    return types;
  }

  /**
   * MFNode; access: inputOnly.
   * @details set_contacts input field for Contact nodes provides per-frame
   * information about contacts between bodies. Dispatches to the handler
   * registered via setOnSet_contactsHandler(); a no-op if none is set.
   */
  void onSet_contacts(const MFNode &value) {
    if (_onSet_contactsHandler)
      _onSet_contactsHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_contactsHandler(std::function<void(const MFNode &)> handler) {
    _onSet_contactsHandler = std::move(handler);
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
  static void checkRangesDisableTime(const SFTime &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

private:
  static void validateDisableTime(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("disableTime below minimum of 0");
  }

  SFBool _autoDisable{false};

  MFNode _bodies{};

  SFNode _collider{nullptr};

  SFFloat _constantForceMix{0.0001};

  SFFloat _contactSurfaceThickness{0};

  SFFloat _disableAngularSpeed{0};

  SFFloat _disableLinearSpeed{0};

  SFTime _disableTime{0};

  SFBool _enabled{true};

  SFFloat _errorCorrection{0.8};

  SFVec3f _gravity{SFVec3f{0, -9.8, 0}};

  SFInt32 _iterations{10};

  MFNode _joints{};

  SFFloat _maxCorrectionSpeed{-1};

  SFBool _preferAccuracy{false};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFNode &)> _onSet_contactsHandler{};
};

} // namespace x3d::nodes
