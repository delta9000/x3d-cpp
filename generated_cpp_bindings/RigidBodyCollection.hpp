// RigidBodyCollection.hpp
#ifndef RIGIDBODYCOLLECTION_HPP
#define RIGIDBODYCOLLECTION_HPP

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
 * @class RigidBodyCollection
 * @brief RigidBodyCollection represents a system of bodies that interact within
 * a single physics model.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#RigidBodyCollection
 */
class RigidBodyCollection : public virtual X3DChildNode,
                            public virtual X3DBoundedObject {
public:
  /**
   * @brief Default constructor for RigidBodyCollection
   */
  RigidBodyCollection() = default;

  /**
   * @brief Destructor for RigidBodyCollection
   */
  ~RigidBodyCollection() = default;

  /**
   * @brief Get the default value for autoDisable
   * @return SFBool The default value
   */
  static SFBool getDefaultAutoDisable() { return false; }

  /**
   * @brief Get the default value for collider
   * @return SFNode The default value
   */
  static SFNode getDefaultCollider() { return nullptr; }

  /**
   * @brief Get the default value for constantForceMix
   * @return SFFloat The default value
   */
  static SFFloat getDefaultConstantForceMix() { return 0.0001; }

  /**
   * @brief Get the default value for contactSurfaceThickness
   * @return SFFloat The default value
   */
  static SFFloat getDefaultContactSurfaceThickness() { return 0; }

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
   * @brief Get the default value for errorCorrection
   * @return SFFloat The default value
   */
  static SFFloat getDefaultErrorCorrection() { return 0.8; }

  /**
   * @brief Get the default value for gravity
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultGravity() { return SFVec3f{0, -9.8, 0}; }

  /**
   * @brief Get the default value for iterations
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultIterations() { return 10; }

  /**
   * @brief Get the default value for maxCorrectionSpeed
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMaxCorrectionSpeed() { return -1; }

  /**
   * @brief Get the default value for preferAccuracy
   * @return SFBool The default value
   */
  static SFBool getDefaultPreferAccuracy() { return false; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

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
   * @brief Gets the value of bodies. AccessType: inputOutput
   * @details Collection of top-level nodes that comprise a set of bodies
   * evaluated as a single set of interactions.
   * @return MFNode The current value of bodies.
   */
  MFNode getBodies() const { return _bodies; }

  /**
   * @brief Acceptable node types for the bodies field.
   * @details Permitted X3D node types: RigidBody
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableBodiesNodeTypes() {
    static const std::vector<std::string> types = {"RigidBody"};
    return types;
  }

  /**
   * @brief Sets the value of bodies. AccessType: inputOutput
   * @details Collection of top-level nodes that comprise a set of bodies
   * evaluated as a single set of interactions.
   * @param value The new value for bodies.
   */
  void setBodies(const MFNode &value) { _bodies = value; }

  void setBodies(MFNode &&value) { _bodies = std::move(value); }

  /**
   * @brief Gets the value of collider. AccessType: initializeOnly
   * @details The collider field associates a collision collection with this
   * rigid body collection allowing seamless updates and integration without the
   * need to use the X3D event model.
   * @return SFNode The current value of collider.
   */
  SFNode getCollider() const { return _collider; }

  /**
   * @brief Acceptable node types for the collider field.
   * @details Permitted X3D node types: CollisionCollection
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableColliderNodeTypes() {
    static const std::vector<std::string> types = {"CollisionCollection"};
    return types;
  }
  /**
   * @brief Data-layer write of collider (reader/init ingest path).
   * @details collider is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setCollider().
   */
  void setColliderUnchecked(const SFNode &value) { _collider = value; }

  /**
   * @brief Gets the value of constantForceMix. AccessType: inputOutput
   * @details constantForceMix modifies damping calculations by violating normal
   * constraints while applying small, constant forces in those calculations.
   * @return SFFloat The current value of constantForceMix.
   */
  SFFloat getConstantForceMix() const { return _constantForceMix; }

  /**
   * @brief Sets the value of constantForceMix. AccessType: inputOutput
   * @details constantForceMix modifies damping calculations by violating normal
   * constraints while applying small, constant forces in those calculations.
   * @param value The new value for constantForceMix.
   */
  void setConstantForceMix(const SFFloat &value) { _constantForceMix = value; }

  /**
   * @brief Gets the value of contactSurfaceThickness. AccessType: inputOutput
   * @details contactSurfaceThickness defines how far bodies may interpenetrate
   * after a collision, allowing simulation of softer bodies that deform
   * somewhat during collision.
   * @return SFFloat The current value of contactSurfaceThickness.
   */
  SFFloat getContactSurfaceThickness() const {
    return _contactSurfaceThickness;
  }

  /**
   * @brief Sets the value of contactSurfaceThickness. AccessType: inputOutput
   * @details contactSurfaceThickness defines how far bodies may interpenetrate
   * after a collision, allowing simulation of softer bodies that deform
   * somewhat during collision.
   * @param value The new value for contactSurfaceThickness.
   */
  void setContactSurfaceThickness(const SFFloat &value) {

    _contactSurfaceThickness = value;
  }

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
   * @brief Gets the value of errorCorrection. AccessType: inputOutput
   * @details errorCorrection describes how quickly intersection errors due to
   * floating-point inaccuracies are resolved (0=no correction, 1=all corrected
   * in single step).
   * @return SFFloat The current value of errorCorrection.
   */
  SFFloat getErrorCorrection() const { return _errorCorrection; }

  /**
   * @brief Sets the value of errorCorrection. AccessType: inputOutput
   * @details errorCorrection describes how quickly intersection errors due to
   * floating-point inaccuracies are resolved (0=no correction, 1=all corrected
   * in single step).
   * @param value The new value for errorCorrection.
   */
  void setErrorCorrection(const SFFloat &value) { _errorCorrection = value; }

  /**
   * @brief Gets the value of gravity. AccessType: inputOutput
   * @details gravity indicates direction and strength of local gravity vector
   * for this collection of bodies (units m/sec^2).
   * @return SFVec3f The current value of gravity.
   */
  SFVec3f getGravity() const { return _gravity; }

  /**
   * @brief Sets the value of gravity. AccessType: inputOutput
   * @details gravity indicates direction and strength of local gravity vector
   * for this collection of bodies (units m/sec^2).
   * @param value The new value for gravity.
   */
  void setGravity(const SFVec3f &value) { _gravity = value; }

  void setGravity(SFVec3f &&value) { _gravity = std::move(value); }

  /**
   * @brief Gets the value of iterations. AccessType: inputOutput
   * @details iterations controls number of iterations performed over
   * collectioned joints and bodies during each evaluation.
   * @return SFInt32 The current value of iterations.
   */
  SFInt32 getIterations() const { return _iterations; }

  /**
   * @brief Sets the value of iterations. AccessType: inputOutput
   * @details iterations controls number of iterations performed over
   * collectioned joints and bodies during each evaluation.
   * @param value The new value for iterations.
   */
  void setIterations(const SFInt32 &value) { _iterations = value; }

  /**
   * @brief Gets the value of joints. AccessType: inputOutput
   * @details The joints field is used to register all joints between bodies
   * contained in this collection.
   * @return MFNode The current value of joints.
   */
  MFNode getJoints() const { return _joints; }

  /**
   * @brief Acceptable node types for the joints field.
   * @details Permitted X3D node types: X3DRigidJointNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableJointsNodeTypes() {
    static const std::vector<std::string> types = {"X3DRigidJointNode"};
    return types;
  }

  /**
   * @brief Sets the value of joints. AccessType: inputOutput
   * @details The joints field is used to register all joints between bodies
   * contained in this collection.
   * @param value The new value for joints.
   */
  void setJoints(const MFNode &value) { _joints = value; }

  void setJoints(MFNode &&value) { _joints = std::move(value); }

  /**
   * @brief Gets the value of maxCorrectionSpeed. AccessType: inputOutput
   * @details or -1, maxCorrectionSpeed.
   * @return SFFloat The current value of maxCorrectionSpeed.
   */
  SFFloat getMaxCorrectionSpeed() const { return _maxCorrectionSpeed; }

  /**
   * @brief Sets the value of maxCorrectionSpeed. AccessType: inputOutput
   * @details or -1, maxCorrectionSpeed.
   * @param value The new value for maxCorrectionSpeed.
   */
  void setMaxCorrectionSpeed(const SFFloat &value) {

    _maxCorrectionSpeed = value;
  }

  /**
   * @brief Gets the value of preferAccuracy. AccessType: inputOutput
   * @details preferAccuracy provides hint for performance preference: higher
   * accuracy or faster computational speed.
   * @return SFBool The current value of preferAccuracy.
   */
  SFBool getPreferAccuracy() const { return _preferAccuracy; }

  /**
   * @brief Sets the value of preferAccuracy. AccessType: inputOutput
   * @details preferAccuracy provides hint for performance preference: higher
   * accuracy or faster computational speed.
   * @param value The new value for preferAccuracy.
   */
  void setPreferAccuracy(const SFBool &value) { _preferAccuracy = value; }

  /**
   * @brief Acceptable node types for the set_contacts field.
   * @details Permitted X3D node types: Contact
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableSet_contactsNodeTypes() {
    static const std::vector<std::string> types = {"Contact"};
    return types;
  }

  /**
   * @brief Event handler invoked when an event is received on set_contacts.
   * AccessType: inputOnly
   * @details set_contacts input field for Contact nodes provides per-frame
   * information about contacts between bodies. Dispatches to the handler
   * registered via setOnSet_contactsHandler(); a no-op if none is set. The
   * event cascade reaches this through the node's reflected field table, so
   * routing is node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_contacts(const MFNode &value) {
    if (_onSet_contactsHandler)
      _onSet_contactsHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on set_contacts.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnSet_contactsHandler(std::function<void(const MFNode &)> handler) {
    _onSet_contactsHandler = std::move(handler);
  }

  /**
   * @brief The X3D type name of this node (e.g. "RigidBodyCollection").
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
   * @brief Member variable for autoDisable.
   */

  SFBool _autoDisable{false};

  /**
   * @brief Member variable for bodies.
   */

  MFNode _bodies{};

  /**
   * @brief Member variable for collider.
   */

  SFNode _collider{nullptr};

  /**
   * @brief Member variable for constantForceMix.
   */

  SFFloat _constantForceMix{0.0001};

  /**
   * @brief Member variable for contactSurfaceThickness.
   */

  SFFloat _contactSurfaceThickness{0};

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
   * @brief Member variable for errorCorrection.
   */

  SFFloat _errorCorrection{0.8};

  /**
   * @brief Member variable for gravity.
   */

  SFVec3f _gravity{SFVec3f{0, -9.8, 0}};

  /**
   * @brief Member variable for iterations.
   */

  SFInt32 _iterations{10};

  /**
   * @brief Member variable for joints.
   */

  MFNode _joints{};

  /**
   * @brief Member variable for maxCorrectionSpeed.
   */

  SFFloat _maxCorrectionSpeed{-1};

  /**
   * @brief Member variable for preferAccuracy.
   */

  SFBool _preferAccuracy{false};

  /**
   * @brief Registered event handler for set_contacts (inputOnly); empty until
   * set.
   */
  std::function<void(const MFNode &)> _onSet_contactsHandler{};
};

#endif // RIGIDBODYCOLLECTION_HPP