// EspduTransform.hpp
#ifndef ESPDUTRANSFORM_HPP
#define ESPDUTRANSFORM_HPP

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

#include "X3DGroupingNode.hpp"

#include "X3DSensorNode.hpp"

#include "X3DNetworkSensorNode.hpp"

/**
 * @class EspduTransform
 * @brief EspduTransform is a networked Transform node that can contain most
 * nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/dis.html#EspduTransform
 */
class EspduTransform : public virtual X3DGroupingNode,
                       public virtual X3DNetworkSensorNode {
public:
  /**
   * @brief Default constructor for EspduTransform
   */
  EspduTransform() = default;

  /**
   * @brief Destructor for EspduTransform
   */
  ~EspduTransform() = default;

  /**
   * @brief Get the default value for address
   * @return SFString The default value
   */
  static SFString getDefaultAddress() { return "localhost"; }

  /**
   * @brief Get the default value for applicationID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultApplicationID() { return 0; }

  /**
   * @brief Get the default value for articulationParameterCount
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultArticulationParameterCount() { return 0; }

  /**
   * @brief Get the default value for center
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultCenter() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for collisionType
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultCollisionType() { return 0; }

  /**
   * @brief Get the default value for deadReckoning
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultDeadReckoning() { return 0; }

  /**
   * @brief Get the default value for detonationLocation
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultDetonationLocation() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for detonationRelativeLocation
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultDetonationRelativeLocation() {
    return SFVec3f{0, 0, 0};
  }

  /**
   * @brief Get the default value for detonationResult
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultDetonationResult() { return 0; }

  /**
   * @brief Get the default value for enabled
   * @return SFBool The default value
   */
  static SFBool getDefaultEnabled() { return true; }

  /**
   * @brief Get the default value for entityCategory
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultEntityCategory() { return 0; }

  /**
   * @brief Get the default value for entityCountry
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultEntityCountry() { return 0; }

  /**
   * @brief Get the default value for entityDomain
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultEntityDomain() { return 0; }

  /**
   * @brief Get the default value for entityExtra
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultEntityExtra() { return 0; }

  /**
   * @brief Get the default value for entityID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultEntityID() { return 0; }

  /**
   * @brief Get the default value for entityKind
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultEntityKind() { return 0; }

  /**
   * @brief Get the default value for entitySpecific
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultEntitySpecific() { return 0; }

  /**
   * @brief Get the default value for entitySubcategory
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultEntitySubcategory() { return 0; }

  /**
   * @brief Get the default value for eventApplicationID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultEventApplicationID() { return 0; }

  /**
   * @brief Get the default value for eventEntityID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultEventEntityID() { return 0; }

  /**
   * @brief Get the default value for eventNumber
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultEventNumber() { return 0; }

  /**
   * @brief Get the default value for eventSiteID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultEventSiteID() { return 0; }

  /**
   * @brief Get the default value for fired1
   * @return SFBool The default value
   */
  static SFBool getDefaultFired1() { return false; }

  /**
   * @brief Get the default value for fired2
   * @return SFBool The default value
   */
  static SFBool getDefaultFired2() { return false; }

  /**
   * @brief Get the default value for fireMissionIndex
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultFireMissionIndex() { return 0; }

  /**
   * @brief Get the default value for firingRange
   * @return SFFloat The default value
   */
  static SFFloat getDefaultFiringRange() { return 0; }

  /**
   * @brief Get the default value for firingRate
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultFiringRate() { return 0; }

  /**
   * @brief Get the default value for forceID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultForceID() { return 0; }

  /**
   * @brief Get the default value for fuse
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultFuse() { return 0; }

  /**
   * @brief Get the default value for geoCoords
   * @return SFVec3d The default value
   */
  static SFVec3d getDefaultGeoCoords() { return SFVec3d{0, 0, 0}; }

  /**
   * @brief Get the default value for geoSystem
   * @return MFString The default value
   */
  static MFString getDefaultGeoSystem() {
    return std::vector<std::string>{"GD", "WE"};
  }

  /**
   * @brief Get the default value for linearAcceleration
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultLinearAcceleration() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for linearVelocity
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultLinearVelocity() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for multicastRelayPort
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultMulticastRelayPort() { return 0; }

  /**
   * @brief Get the default value for munitionApplicationID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultMunitionApplicationID() { return 0; }

  /**
   * @brief Get the default value for munitionEndPoint
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultMunitionEndPoint() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for munitionEntityID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultMunitionEntityID() { return 0; }

  /**
   * @brief Get the default value for munitionQuantity
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultMunitionQuantity() { return 0; }

  /**
   * @brief Get the default value for munitionSiteID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultMunitionSiteID() { return 0; }

  /**
   * @brief Get the default value for munitionStartPoint
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultMunitionStartPoint() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for networkMode
   * @return NetworkModeChoices The default value
   */
  static NetworkModeChoices getDefaultNetworkMode() {
    return NetworkModeChoices::STANDALONE;
  }

  /**
   * @brief Get the default value for port
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultPort() { return 0; }

  /**
   * @brief Get the default value for readInterval
   * @return SFTime The default value
   */
  static SFTime getDefaultReadInterval() { return 0.1; }

  /**
   * @brief Get the default value for rotation
   * @return SFRotation The default value
   */
  static SFRotation getDefaultRotation() { return SFRotation{0, 0, 1, 0}; }

  /**
   * @brief Get the default value for rtpHeaderExpected
   * @return SFBool The default value
   */
  static SFBool getDefaultRtpHeaderExpected() { return false; }

  /**
   * @brief Get the default value for scale
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultScale() { return SFVec3f{1, 1, 1}; }

  /**
   * @brief Get the default value for scaleOrientation
   * @return SFRotation The default value
   */
  static SFRotation getDefaultScaleOrientation() {
    return SFRotation{0, 0, 1, 0};
  }

  /**
   * @brief Get the default value for siteID
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultSiteID() { return 0; }

  /**
   * @brief Get the default value for translation
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultTranslation() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for warhead
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultWarhead() { return 0; }

  /**
   * @brief Get the default value for writeInterval
   * @return SFTime The default value
   */
  static SFTime getDefaultWriteInterval() { return 1.0; }

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
  static std::string componentName() { return "DIS"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of address. AccessType: inputOutput
   * @details Multicast network address, or else 'localhost'; Example: 224.
   * @return SFString The current value of address.
   */
  SFString getAddress() const { return _address; }

  /**
   * @brief Sets the value of address. AccessType: inputOutput
   * @details Multicast network address, or else 'localhost'; Example: 224.
   * @param value The new value for address.
   */
  void setAddress(const SFString &value) { _address = value; }

  void setAddress(SFString &&value) { _address = std::move(value); }

  /**
   * @brief Gets the value of applicationID. AccessType: inputOutput
   * @details Simulation/exercise applicationID is unique for application at
   * that site.
   * @return SFInt32 The current value of applicationID.
   */
  SFInt32 getApplicationID() const { return _applicationID; }

  /**
   * @brief Sets the value of applicationID. AccessType: inputOutput
   * @details Simulation/exercise applicationID is unique for application at
   * that site.
   * @param value The new value for applicationID.
   */
  void setApplicationID(const SFInt32 &value) { _applicationID = value; }

  /**
   * @brief Gets the value of articulationParameterArray. AccessType:
   * inputOutput
   * @details Information required for representation of the entity's visual
   * appearance and position of its articulated parts.
   * @return MFFloat The current value of articulationParameterArray.
   */
  MFFloat getArticulationParameterArray() const {
    return _articulationParameterArray;
  }

  /**
   * @brief Sets the value of articulationParameterArray. AccessType:
   * inputOutput
   * @details Information required for representation of the entity's visual
   * appearance and position of its articulated parts.
   * @param value The new value for articulationParameterArray.
   */
  void setArticulationParameterArray(const MFFloat &value) {

    _articulationParameterArray = value;
  }

  void setArticulationParameterArray(MFFloat &&value) {

    _articulationParameterArray = std::move(value);
  }

  /**
   * @brief Gets the value of articulationParameterChangeIndicatorArray.
   * AccessType: inputOutput
   * @details Array of change counters, each incremented when an articulated
   * parameter is updated.
   * @return MFInt32 The current value of
   * articulationParameterChangeIndicatorArray.
   */
  MFInt32 getArticulationParameterChangeIndicatorArray() const {
    return _articulationParameterChangeIndicatorArray;
  }

  /**
   * @brief Sets the value of articulationParameterChangeIndicatorArray.
   * AccessType: inputOutput
   * @details Array of change counters, each incremented when an articulated
   * parameter is updated.
   * @param value The new value for articulationParameterChangeIndicatorArray.
   */
  void setArticulationParameterChangeIndicatorArray(const MFInt32 &value) {

    _articulationParameterChangeIndicatorArray = value;
  }

  void setArticulationParameterChangeIndicatorArray(MFInt32 &&value) {

    _articulationParameterChangeIndicatorArray = std::move(value);
  }

  /**
   * @brief Gets the value of articulationParameterCount. AccessType:
   * inputOutput
   * @details Number of articulated parameters attached to this entity state
   * PDU.
   * @return SFInt32 The current value of articulationParameterCount.
   */
  SFInt32 getArticulationParameterCount() const {
    return _articulationParameterCount;
  }

  /**
   * @brief Sets the value of articulationParameterCount. AccessType:
   * inputOutput
   * @details Number of articulated parameters attached to this entity state
   * PDU.
   * @param value The new value for articulationParameterCount.
   */
  void setArticulationParameterCount(const SFInt32 &value) {

    _articulationParameterCount = value;
  }

  /**
   * @brief Gets the value of articulationParameterDesignatorArray. AccessType:
   * inputOutput
   * @details Array of designators for each articulated parameter.
   * @return MFInt32 The current value of articulationParameterDesignatorArray.
   */
  MFInt32 getArticulationParameterDesignatorArray() const {
    return _articulationParameterDesignatorArray;
  }

  /**
   * @brief Sets the value of articulationParameterDesignatorArray. AccessType:
   * inputOutput
   * @details Array of designators for each articulated parameter.
   * @param value The new value for articulationParameterDesignatorArray.
   */
  void setArticulationParameterDesignatorArray(const MFInt32 &value) {

    _articulationParameterDesignatorArray = value;
  }

  void setArticulationParameterDesignatorArray(MFInt32 &&value) {

    _articulationParameterDesignatorArray = std::move(value);
  }

  /**
   * @brief Gets the value of articulationParameterIdPartAttachedToArray.
   * AccessType: inputOutput
   * @details Array of ID parts that each articulated parameter is attached to.
   * @return MFInt32 The current value of
   * articulationParameterIdPartAttachedToArray.
   */
  MFInt32 getArticulationParameterIdPartAttachedToArray() const {
    return _articulationParameterIdPartAttachedToArray;
  }

  /**
   * @brief Sets the value of articulationParameterIdPartAttachedToArray.
   * AccessType: inputOutput
   * @details Array of ID parts that each articulated parameter is attached to.
   * @param value The new value for articulationParameterIdPartAttachedToArray.
   */
  void setArticulationParameterIdPartAttachedToArray(const MFInt32 &value) {

    _articulationParameterIdPartAttachedToArray = value;
  }

  void setArticulationParameterIdPartAttachedToArray(MFInt32 &&value) {

    _articulationParameterIdPartAttachedToArray = std::move(value);
  }

  /**
   * @brief Gets the value of articulationParameterTypeArray. AccessType:
   * inputOutput
   * @details Array of type enumerations for each articulated parameter element.
   * @return MFInt32 The current value of articulationParameterTypeArray.
   */
  MFInt32 getArticulationParameterTypeArray() const {
    return _articulationParameterTypeArray;
  }

  /**
   * @brief Sets the value of articulationParameterTypeArray. AccessType:
   * inputOutput
   * @details Array of type enumerations for each articulated parameter element.
   * @param value The new value for articulationParameterTypeArray.
   */
  void setArticulationParameterTypeArray(const MFInt32 &value) {

    _articulationParameterTypeArray = value;
  }

  void setArticulationParameterTypeArray(MFInt32 &&value) {

    _articulationParameterTypeArray = std::move(value);
  }

  /**
   * @brief Gets the value of articulationParameterValue0_changed. AccessType:
   * outputOnly
   * @details Get element of user-defined payload array.
   * @return SFFloat The current value of articulationParameterValue0_changed.
   */
  SFFloat getArticulationParameterValue0_changed() const {
    return _articulationParameterValue0_changed;
  }

  /**
   * @brief Emit an output value on articulationParameterValue0_changed.
   * AccessType: outputOnly
   * @details Get element of user-defined payload array.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitArticulationParameterValue0_changed(const SFFloat &value) {
    _articulationParameterValue0_changed = value;
  }

  /**
   * @brief Gets the value of articulationParameterValue1_changed. AccessType:
   * outputOnly
   * @details Get element of user-defined payload array.
   * @return SFFloat The current value of articulationParameterValue1_changed.
   */
  SFFloat getArticulationParameterValue1_changed() const {
    return _articulationParameterValue1_changed;
  }

  /**
   * @brief Emit an output value on articulationParameterValue1_changed.
   * AccessType: outputOnly
   * @details Get element of user-defined payload array.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitArticulationParameterValue1_changed(const SFFloat &value) {
    _articulationParameterValue1_changed = value;
  }

  /**
   * @brief Gets the value of articulationParameterValue2_changed. AccessType:
   * outputOnly
   * @details Get element of user-defined payload array.
   * @return SFFloat The current value of articulationParameterValue2_changed.
   */
  SFFloat getArticulationParameterValue2_changed() const {
    return _articulationParameterValue2_changed;
  }

  /**
   * @brief Emit an output value on articulationParameterValue2_changed.
   * AccessType: outputOnly
   * @details Get element of user-defined payload array.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitArticulationParameterValue2_changed(const SFFloat &value) {
    _articulationParameterValue2_changed = value;
  }

  /**
   * @brief Gets the value of articulationParameterValue3_changed. AccessType:
   * outputOnly
   * @details Get element of user-defined payload array.
   * @return SFFloat The current value of articulationParameterValue3_changed.
   */
  SFFloat getArticulationParameterValue3_changed() const {
    return _articulationParameterValue3_changed;
  }

  /**
   * @brief Emit an output value on articulationParameterValue3_changed.
   * AccessType: outputOnly
   * @details Get element of user-defined payload array.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitArticulationParameterValue3_changed(const SFFloat &value) {
    _articulationParameterValue3_changed = value;
  }

  /**
   * @brief Gets the value of articulationParameterValue4_changed. AccessType:
   * outputOnly
   * @details Get element of user-defined payload array.
   * @return SFFloat The current value of articulationParameterValue4_changed.
   */
  SFFloat getArticulationParameterValue4_changed() const {
    return _articulationParameterValue4_changed;
  }

  /**
   * @brief Emit an output value on articulationParameterValue4_changed.
   * AccessType: outputOnly
   * @details Get element of user-defined payload array.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitArticulationParameterValue4_changed(const SFFloat &value) {
    _articulationParameterValue4_changed = value;
  }

  /**
   * @brief Gets the value of articulationParameterValue5_changed. AccessType:
   * outputOnly
   * @details Get element of user-defined payload array.
   * @return SFFloat The current value of articulationParameterValue5_changed.
   */
  SFFloat getArticulationParameterValue5_changed() const {
    return _articulationParameterValue5_changed;
  }

  /**
   * @brief Emit an output value on articulationParameterValue5_changed.
   * AccessType: outputOnly
   * @details Get element of user-defined payload array.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitArticulationParameterValue5_changed(const SFFloat &value) {
    _articulationParameterValue5_changed = value;
  }

  /**
   * @brief Gets the value of articulationParameterValue6_changed. AccessType:
   * outputOnly
   * @details Get element of user-defined payload array.
   * @return SFFloat The current value of articulationParameterValue6_changed.
   */
  SFFloat getArticulationParameterValue6_changed() const {
    return _articulationParameterValue6_changed;
  }

  /**
   * @brief Emit an output value on articulationParameterValue6_changed.
   * AccessType: outputOnly
   * @details Get element of user-defined payload array.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitArticulationParameterValue6_changed(const SFFloat &value) {
    _articulationParameterValue6_changed = value;
  }

  /**
   * @brief Gets the value of articulationParameterValue7_changed. AccessType:
   * outputOnly
   * @details Get element of user-defined payload array.
   * @return SFFloat The current value of articulationParameterValue7_changed.
   */
  SFFloat getArticulationParameterValue7_changed() const {
    return _articulationParameterValue7_changed;
  }

  /**
   * @brief Emit an output value on articulationParameterValue7_changed.
   * AccessType: outputOnly
   * @details Get element of user-defined payload array.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitArticulationParameterValue7_changed(const SFFloat &value) {
    _articulationParameterValue7_changed = value;
  }

  /**
   * @brief Gets the value of center. AccessType: inputOutput
   * @details Translation offset from origin of local coordinate system.
   * @return SFVec3f The current value of center.
   */
  SFVec3f getCenter() const { return _center; }

  /**
   * @brief Sets the value of center. AccessType: inputOutput
   * @details Translation offset from origin of local coordinate system.
   * @param value The new value for center.
   */
  void setCenter(const SFVec3f &value) { _center = value; }

  void setCenter(SFVec3f &&value) { _center = std::move(value); }

  /**
   * @brief Gets the value of collideTime. AccessType: outputOnly
   * @details When were we collided with? Warning: it is an error to define this
   * transient outputOnly field in an X3D file, instead only use it a source for
   * ROUTE events.
   * @return SFTime The current value of collideTime.
   */
  SFTime getCollideTime() const { return _collideTime; }

  /**
   * @brief Emit an output value on collideTime. AccessType: outputOnly
   * @details When were we collided with? Warning: it is an error to define this
   * transient outputOnly field in an X3D file, instead only use it a source for
   * ROUTE events. outputOnly fields have no author-facing setter; a node's
   * behavior or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitCollideTime(const SFTime &value) { _collideTime = value; }

  /**
   * @brief Gets the value of collisionType. AccessType: inputOutput
   * @details Integer enumeration for type of collision: ELASTIC or INELASTIC.
   * @return SFInt32 The current value of collisionType.
   */
  SFInt32 getCollisionType() const { return _collisionType; }

  /**
   * @brief Sets the value of collisionType. AccessType: inputOutput
   * @details Integer enumeration for type of collision: ELASTIC or INELASTIC.
   * @param value The new value for collisionType.
   */
  void setCollisionType(const SFInt32 &value) { _collisionType = value; }

  /**
   * @brief Gets the value of deadReckoning. AccessType: inputOutput
   * @details Dead reckoning algorithm being used to project
   * position/orientation with velocities/accelerations.
   * @return SFInt32 The current value of deadReckoning.
   */
  SFInt32 getDeadReckoning() const { return _deadReckoning; }

  /**
   * @brief Sets the value of deadReckoning. AccessType: inputOutput
   * @details Dead reckoning algorithm being used to project
   * position/orientation with velocities/accelerations.
   * @param value The new value for deadReckoning.
   */
  void setDeadReckoning(const SFInt32 &value) { _deadReckoning = value; }

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
   * @brief Gets the value of detonateTime. AccessType: outputOnly
   * @details When were we detonated?.
   * @return SFTime The current value of detonateTime.
   */
  SFTime getDetonateTime() const { return _detonateTime; }

  /**
   * @brief Emit an output value on detonateTime. AccessType: outputOnly
   * @details When were we detonated?.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitDetonateTime(const SFTime &value) { _detonateTime = value; }

  /**
   * @brief Gets the value of detonationLocation. AccessType: inputOutput
   * @details World coordinates for detonationLocation.
   * @return SFVec3f The current value of detonationLocation.
   */
  SFVec3f getDetonationLocation() const { return _detonationLocation; }

  /**
   * @brief Sets the value of detonationLocation. AccessType: inputOutput
   * @details World coordinates for detonationLocation.
   * @param value The new value for detonationLocation.
   */
  void setDetonationLocation(const SFVec3f &value) {

    _detonationLocation = value;
  }

  void setDetonationLocation(SFVec3f &&value) {

    _detonationLocation = std::move(value);
  }

  /**
   * @brief Gets the value of detonationRelativeLocation. AccessType:
   * inputOutput
   * @details Relative coordinates for detonation location.
   * @return SFVec3f The current value of detonationRelativeLocation.
   */
  SFVec3f getDetonationRelativeLocation() const {
    return _detonationRelativeLocation;
  }

  /**
   * @brief Sets the value of detonationRelativeLocation. AccessType:
   * inputOutput
   * @details Relative coordinates for detonation location.
   * @param value The new value for detonationRelativeLocation.
   */
  void setDetonationRelativeLocation(const SFVec3f &value) {

    _detonationRelativeLocation = value;
  }

  void setDetonationRelativeLocation(SFVec3f &&value) {

    _detonationRelativeLocation = std::move(value);
  }

  /**
   * @brief Gets the value of detonationResult. AccessType: inputOutput
   * @details Integer enumeration for type of detonation and result that
   * occurred.
   * @return SFInt32 The current value of detonationResult.
   */
  SFInt32 getDetonationResult() const { return _detonationResult; }

  /**
   * @brief Sets the value of detonationResult. AccessType: inputOutput
   * @details Integer enumeration for type of detonation and result that
   * occurred.
   * @param value The new value for detonationResult.
   */
  void setDetonationResult(const SFInt32 &value) { _detonationResult = value; }

  /**
   * @brief Gets the value of enabled. AccessType: inputOutput
   * @details Enables/disables the sensor node.
   * @return SFBool The current value of enabled.
   */
  SFBool getEnabled() const { return _enabled; }

  /**
   * @brief Sets the value of enabled. AccessType: inputOutput
   * @details Enables/disables the sensor node.
   * @param value The new value for enabled.
   */
  void setEnabled(const SFBool &value) { _enabled = value; }

  /**
   * @brief Gets the value of entityCategory. AccessType: inputOutput
   * @details Integer enumerations value for main category that describes the
   * entity, semantics of each code varies according to domain.
   * @return SFInt32 The current value of entityCategory.
   */
  SFInt32 getEntityCategory() const { return _entityCategory; }

  /**
   * @brief Sets the value of entityCategory. AccessType: inputOutput
   * @details Integer enumerations value for main category that describes the
   * entity, semantics of each code varies according to domain.
   * @param value The new value for entityCategory.
   */
  void setEntityCategory(const SFInt32 &value) { _entityCategory = value; }

  /**
   * @brief Gets the value of entityCountry. AccessType: inputOutput
   * @details Integer enumerations value for country to which the design of the
   * entity or its design specification is attributed.
   * @return SFInt32 The current value of entityCountry.
   */
  SFInt32 getEntityCountry() const { return _entityCountry; }

  /**
   * @brief Sets the value of entityCountry. AccessType: inputOutput
   * @details Integer enumerations value for country to which the design of the
   * entity or its design specification is attributed.
   * @param value The new value for entityCountry.
   */
  void setEntityCountry(const SFInt32 &value) { _entityCountry = value; }

  /**
   * @brief Gets the value of entityDomain. AccessType: inputOutput
   * @details Integer enumerations value for domain in which the entity
   * operates: LAND, AIR, SURFACE, SUBSURFACE, SPACE or OTHER.
   * @return SFInt32 The current value of entityDomain.
   */
  SFInt32 getEntityDomain() const { return _entityDomain; }

  /**
   * @brief Sets the value of entityDomain. AccessType: inputOutput
   * @details Integer enumerations value for domain in which the entity
   * operates: LAND, AIR, SURFACE, SUBSURFACE, SPACE or OTHER.
   * @param value The new value for entityDomain.
   */
  void setEntityDomain(const SFInt32 &value) { _entityDomain = value; }

  /**
   * @brief Gets the value of entityExtra. AccessType: inputOutput
   * @details Any extra information required to describe a particular entity.
   * @return SFInt32 The current value of entityExtra.
   */
  SFInt32 getEntityExtra() const { return _entityExtra; }

  /**
   * @brief Sets the value of entityExtra. AccessType: inputOutput
   * @details Any extra information required to describe a particular entity.
   * @param value The new value for entityExtra.
   */
  void setEntityExtra(const SFInt32 &value) { _entityExtra = value; }

  /**
   * @brief Gets the value of entityID. AccessType: inputOutput
   * @details Simulation/exercise entityID is a unique ID for a single entity
   * within that application.
   * @return SFInt32 The current value of entityID.
   */
  SFInt32 getEntityID() const { return _entityID; }

  /**
   * @brief Sets the value of entityID. AccessType: inputOutput
   * @details Simulation/exercise entityID is a unique ID for a single entity
   * within that application.
   * @param value The new value for entityID.
   */
  void setEntityID(const SFInt32 &value) { _entityID = value; }

  /**
   * @brief Gets the value of entityKind. AccessType: inputOutput
   * @details Integer enumerations value for whether entity is a PLATFORM,
   * MUNITION, LIFE_FORM, ENVIRONMENTAL, CULTURAL_FEATURE, SUPPLY, RADIO,
   * EXPENDABLE, SENSOR_EMITTER or OTHER.
   * @return SFInt32 The current value of entityKind.
   */
  SFInt32 getEntityKind() const { return _entityKind; }

  /**
   * @brief Sets the value of entityKind. AccessType: inputOutput
   * @details Integer enumerations value for whether entity is a PLATFORM,
   * MUNITION, LIFE_FORM, ENVIRONMENTAL, CULTURAL_FEATURE, SUPPLY, RADIO,
   * EXPENDABLE, SENSOR_EMITTER or OTHER.
   * @param value The new value for entityKind.
   */
  void setEntityKind(const SFInt32 &value) { _entityKind = value; }

  /**
   * @brief Gets the value of entitySpecific. AccessType: inputOutput
   * @details Specific information about an entity based on the Subcategory
   * field.
   * @return SFInt32 The current value of entitySpecific.
   */
  SFInt32 getEntitySpecific() const { return _entitySpecific; }

  /**
   * @brief Sets the value of entitySpecific. AccessType: inputOutput
   * @details Specific information about an entity based on the Subcategory
   * field.
   * @param value The new value for entitySpecific.
   */
  void setEntitySpecific(const SFInt32 &value) { _entitySpecific = value; }

  /**
   * @brief Gets the value of entitySubcategory. AccessType: inputOutput
   * @details Integer enumerations value for particular subcategory to which an
   * entity belongs based on the category field.
   * @return SFInt32 The current value of entitySubcategory.
   */
  SFInt32 getEntitySubcategory() const { return _entitySubcategory; }

  /**
   * @brief Sets the value of entitySubcategory. AccessType: inputOutput
   * @details Integer enumerations value for particular subcategory to which an
   * entity belongs based on the category field.
   * @param value The new value for entitySubcategory.
   */
  void setEntitySubcategory(const SFInt32 &value) {

    _entitySubcategory = value;
  }

  /**
   * @brief Gets the value of eventApplicationID. AccessType: inputOutput
   * @details Simulation/exercise eventApplicationID is unique for events
   * generated from application at that site.
   * @return SFInt32 The current value of eventApplicationID.
   */
  SFInt32 getEventApplicationID() const { return _eventApplicationID; }

  /**
   * @brief Sets the value of eventApplicationID. AccessType: inputOutput
   * @details Simulation/exercise eventApplicationID is unique for events
   * generated from application at that site.
   * @param value The new value for eventApplicationID.
   */
  void setEventApplicationID(const SFInt32 &value) {

    _eventApplicationID = value;
  }

  /**
   * @brief Gets the value of eventEntityID. AccessType: inputOutput
   * @details For a given event, simulation/exercise entityID is a unique ID for
   * a single entity within that application.
   * @return SFInt32 The current value of eventEntityID.
   */
  SFInt32 getEventEntityID() const { return _eventEntityID; }

  /**
   * @brief Sets the value of eventEntityID. AccessType: inputOutput
   * @details For a given event, simulation/exercise entityID is a unique ID for
   * a single entity within that application.
   * @param value The new value for eventEntityID.
   */
  void setEventEntityID(const SFInt32 &value) { _eventEntityID = value; }

  /**
   * @brief Gets the value of eventNumber. AccessType: inputOutput
   * @details Sequential number of each event issued by an application.
   * @return SFInt32 The current value of eventNumber.
   */
  SFInt32 getEventNumber() const { return _eventNumber; }

  /**
   * @brief Sets the value of eventNumber. AccessType: inputOutput
   * @details Sequential number of each event issued by an application.
   * @param value The new value for eventNumber.
   */
  void setEventNumber(const SFInt32 &value) { _eventNumber = value; }

  /**
   * @brief Gets the value of eventSiteID. AccessType: inputOutput
   * @details Simulation/exercise siteID of the participating LAN or
   * organization.
   * @return SFInt32 The current value of eventSiteID.
   */
  SFInt32 getEventSiteID() const { return _eventSiteID; }

  /**
   * @brief Sets the value of eventSiteID. AccessType: inputOutput
   * @details Simulation/exercise siteID of the participating LAN or
   * organization.
   * @param value The new value for eventSiteID.
   */
  void setEventSiteID(const SFInt32 &value) { _eventSiteID = value; }

  /**
   * @brief Gets the value of fired1. AccessType: inputOutput
   * @details Has the primary weapon (Fire PDU) been fired?.
   * @return SFBool The current value of fired1.
   */
  SFBool getFired1() const { return _fired1; }

  /**
   * @brief Sets the value of fired1. AccessType: inputOutput
   * @details Has the primary weapon (Fire PDU) been fired?.
   * @param value The new value for fired1.
   */
  void setFired1(const SFBool &value) { _fired1 = value; }

  /**
   * @brief Gets the value of fired2. AccessType: inputOutput
   * @details Has the secondary weapon (Fire PDU) been fired?.
   * @return SFBool The current value of fired2.
   */
  SFBool getFired2() const { return _fired2; }

  /**
   * @brief Sets the value of fired2. AccessType: inputOutput
   * @details Has the secondary weapon (Fire PDU) been fired?.
   * @param value The new value for fired2.
   */
  void setFired2(const SFBool &value) { _fired2 = value; }

  /**
   * @brief Gets the value of firedTime. AccessType: outputOnly
   * @details When did we shoot a weapon (Fire PDU)? Warning: it is an error to
   * define this transient outputOnly field in an X3D file, instead only use it
   * a source for ROUTE events.
   * @return SFTime The current value of firedTime.
   */
  SFTime getFiredTime() const { return _firedTime; }

  /**
   * @brief Emit an output value on firedTime. AccessType: outputOnly
   * @details When did we shoot a weapon (Fire PDU)? Warning: it is an error to
   * define this transient outputOnly field in an X3D file, instead only use it
   * a source for ROUTE events. outputOnly fields have no author-facing setter;
   * a node's behavior or the runtime calls this to produce an output event. The
   * event cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitFiredTime(const SFTime &value) { _firedTime = value; }

  /**
   * @brief Gets the value of fireMissionIndex. AccessType: inputOutput
   * @details .
   * @return SFInt32 The current value of fireMissionIndex.
   */
  SFInt32 getFireMissionIndex() const { return _fireMissionIndex; }

  /**
   * @brief Sets the value of fireMissionIndex. AccessType: inputOutput
   * @details .
   * @param value The new value for fireMissionIndex.
   */
  void setFireMissionIndex(const SFInt32 &value) { _fireMissionIndex = value; }

  /**
   * @brief Gets the value of firingRange. AccessType: inputOutput
   * @details Range (three dimension, straight-line distance) that the firing
   * entity's fire control system has assumed for computing the fire control
   * solution if a weapon and if the value is known.
   * @return SFFloat The current value of firingRange.
   */
  SFFloat getFiringRange() const { return _firingRange; }

  /**
   * @brief Sets the value of firingRange. AccessType: inputOutput
   * @details Range (three dimension, straight-line distance) that the firing
   * entity's fire control system has assumed for computing the fire control
   * solution if a weapon and if the value is known.
   * @param value The new value for firingRange.
   */
  void setFiringRange(const SFFloat &value) { _firingRange = value; }

  /**
   * @brief Gets the value of firingRate. AccessType: inputOutput
   * @details Rate at which munitions are fired.
   * @return SFInt32 The current value of firingRate.
   */
  SFInt32 getFiringRate() const { return _firingRate; }

  /**
   * @brief Sets the value of firingRate. AccessType: inputOutput
   * @details Rate at which munitions are fired.
   * @param value The new value for firingRate.
   */
  void setFiringRate(const SFInt32 &value) { _firingRate = value; }

  /**
   * @brief Gets the value of forceID. AccessType: inputOutput
   * @details forceID determines the team membership of the issuing entity, and
   * whether FRIENDLY OPPOSING or NEUTRAL or OTHER.
   * @return SFInt32 The current value of forceID.
   */
  SFInt32 getForceID() const { return _forceID; }

  /**
   * @brief Sets the value of forceID. AccessType: inputOutput
   * @details forceID determines the team membership of the issuing entity, and
   * whether FRIENDLY OPPOSING or NEUTRAL or OTHER.
   * @param value The new value for forceID.
   */
  void setForceID(const SFInt32 &value) { _forceID = value; }

  /**
   * @brief Gets the value of fuse. AccessType: inputOutput
   * @details Integer enumerations value for type of fuse on the munition.
   * @return SFInt32 The current value of fuse.
   */
  SFInt32 getFuse() const { return _fuse; }

  /**
   * @brief Sets the value of fuse. AccessType: inputOutput
   * @details Integer enumerations value for type of fuse on the munition.
   * @param value The new value for fuse.
   */
  void setFuse(const SFInt32 &value) { _fuse = value; }

  /**
   * @brief Gets the value of geoCoords. AccessType: inputOutput
   * @details Geographic location (specified in current geoSystem coordinates)
   * for children geometry (specified in relative coordinate system, in meters).
   * @return SFVec3d The current value of geoCoords.
   */
  SFVec3d getGeoCoords() const { return _geoCoords; }

  /**
   * @brief Sets the value of geoCoords. AccessType: inputOutput
   * @details Geographic location (specified in current geoSystem coordinates)
   * for children geometry (specified in relative coordinate system, in meters).
   * @param value The new value for geoCoords.
   */
  void setGeoCoords(const SFVec3d &value) { _geoCoords = value; }

  void setGeoCoords(SFVec3d &&value) { _geoCoords = std::move(value); }

  /**
   * @brief Gets the value of geoSystem. AccessType: initializeOnly
   * @details Identifies spatial reference frame: Geodetic (GD), Geocentric
   * (GC), Universal Transverse Mercator (UTM).
   * @return MFString The current value of geoSystem.
   */
  MFString getGeoSystem() const { return _geoSystem; }
  /**
   * @brief Data-layer write of geoSystem (reader/init ingest path).
   * @details geoSystem is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setGeoSystem().
   */
  void setGeoSystemUnchecked(const MFString &value) { _geoSystem = value; }

  /**
   * @brief Gets the value of isActive. AccessType: outputOnly
   * @details Have we received a network update recently? Warning: it is an
   * error to define this transient outputOnly field in an X3D file, instead
   * only use it a source for ROUTE events.
   * @return SFBool The current value of isActive.
   */
  SFBool getIsActive() const { return _isActive; }

  /**
   * @brief Emit an output value on isActive. AccessType: outputOnly
   * @details Have we received a network update recently? Warning: it is an
   * error to define this transient outputOnly field in an X3D file, instead
   * only use it a source for ROUTE events. outputOnly fields have no
   * author-facing setter; a node's behavior or the runtime calls this to
   * produce an output event. The event cascade reaches it through the reflected
   * field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIsActive(const SFBool &value) { _isActive = value; }

  /**
   * @brief Gets the value of isCollided. AccessType: outputOnly
   * @details Has a matching CollisionPDU reported a collision? Warning: it is
   * an error to define this transient outputOnly field in an X3D file, instead
   * only use it a source for ROUTE events.
   * @return SFBool The current value of isCollided.
   */
  SFBool getIsCollided() const { return _isCollided; }

  /**
   * @brief Emit an output value on isCollided. AccessType: outputOnly
   * @details Has a matching CollisionPDU reported a collision? Warning: it is
   * an error to define this transient outputOnly field in an X3D file, instead
   * only use it a source for ROUTE events. outputOnly fields have no
   * author-facing setter; a node's behavior or the runtime calls this to
   * produce an output event. The event cascade reaches it through the reflected
   * field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIsCollided(const SFBool &value) { _isCollided = value; }

  /**
   * @brief Gets the value of isDetonated. AccessType: outputOnly
   * @details Has a matching DetonationPDU reported a detonation? Warning: it is
   * an error to define this transient outputOnly field in an X3D file, instead
   * only use it a source for ROUTE events.
   * @return SFBool The current value of isDetonated.
   */
  SFBool getIsDetonated() const { return _isDetonated; }

  /**
   * @brief Emit an output value on isDetonated. AccessType: outputOnly
   * @details Has a matching DetonationPDU reported a detonation? Warning: it is
   * an error to define this transient outputOnly field in an X3D file, instead
   * only use it a source for ROUTE events. outputOnly fields have no
   * author-facing setter; a node's behavior or the runtime calls this to
   * produce an output event. The event cascade reaches it through the reflected
   * field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIsDetonated(const SFBool &value) { _isDetonated = value; }

  /**
   * @brief Gets the value of isNetworkReader. AccessType: outputOnly
   * @details Whether networkMode='remote' (listen to network as copy of remote
   * entity).
   * @return SFBool The current value of isNetworkReader.
   */
  SFBool getIsNetworkReader() const { return _isNetworkReader; }

  /**
   * @brief Emit an output value on isNetworkReader. AccessType: outputOnly
   * @details Whether networkMode='remote' (listen to network as copy of remote
   * entity). outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIsNetworkReader(const SFBool &value) { _isNetworkReader = value; }

  /**
   * @brief Gets the value of isNetworkWriter. AccessType: outputOnly
   * @details Whether networkMode='master' (output to network as master entity
   * at writeInterval).
   * @return SFBool The current value of isNetworkWriter.
   */
  SFBool getIsNetworkWriter() const { return _isNetworkWriter; }

  /**
   * @brief Emit an output value on isNetworkWriter. AccessType: outputOnly
   * @details Whether networkMode='master' (output to network as master entity
   * at writeInterval). outputOnly fields have no author-facing setter; a node's
   * behavior or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIsNetworkWriter(const SFBool &value) { _isNetworkWriter = value; }

  /**
   * @brief Gets the value of isRtpHeaderHeard. AccessType: outputOnly
   * @details Whether incoming DIS packets have an RTP header prepended.
   * @return SFBool The current value of isRtpHeaderHeard.
   */
  SFBool getIsRtpHeaderHeard() const { return _isRtpHeaderHeard; }

  /**
   * @brief Emit an output value on isRtpHeaderHeard. AccessType: outputOnly
   * @details Whether incoming DIS packets have an RTP header prepended.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIsRtpHeaderHeard(const SFBool &value) { _isRtpHeaderHeard = value; }

  /**
   * @brief Gets the value of isStandAlone. AccessType: outputOnly
   * @details Whether networkMode='local' (ignore network but still respond to
   * local events).
   * @return SFBool The current value of isStandAlone.
   */
  SFBool getIsStandAlone() const { return _isStandAlone; }

  /**
   * @brief Emit an output value on isStandAlone. AccessType: outputOnly
   * @details Whether networkMode='local' (ignore network but still respond to
   * local events). outputOnly fields have no author-facing setter; a node's
   * behavior or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIsStandAlone(const SFBool &value) { _isStandAlone = value; }

  /**
   * @brief Gets the value of linearAcceleration. AccessType: inputOutput
   * @details Acceleration of the entity relative to the rotating Earth in
   * either world or entity coordinates, depending on the dead reckoning
   * algorithm used.
   * @return SFVec3f The current value of linearAcceleration.
   */
  SFVec3f getLinearAcceleration() const { return _linearAcceleration; }

  /**
   * @brief Sets the value of linearAcceleration. AccessType: inputOutput
   * @details Acceleration of the entity relative to the rotating Earth in
   * either world or entity coordinates, depending on the dead reckoning
   * algorithm used.
   * @param value The new value for linearAcceleration.
   */
  void setLinearAcceleration(const SFVec3f &value) {

    _linearAcceleration = value;
  }

  void setLinearAcceleration(SFVec3f &&value) {

    _linearAcceleration = std::move(value);
  }

  /**
   * @brief Gets the value of linearVelocity. AccessType: inputOutput
   * @details Velocity of the entity relative to the rotating Earth in either
   * world or entity coordinates, depending on the dead reckoning algorithm
   * used.
   * @return SFVec3f The current value of linearVelocity.
   */
  SFVec3f getLinearVelocity() const { return _linearVelocity; }

  /**
   * @brief Sets the value of linearVelocity. AccessType: inputOutput
   * @details Velocity of the entity relative to the rotating Earth in either
   * world or entity coordinates, depending on the dead reckoning algorithm
   * used.
   * @param value The new value for linearVelocity.
   */
  void setLinearVelocity(const SFVec3f &value) { _linearVelocity = value; }

  void setLinearVelocity(SFVec3f &&value) {

    _linearVelocity = std::move(value);
  }

  /**
   * @brief Gets the value of marking. AccessType: inputOutput
   * @details Maximum of 11 characters for simple entity label.
   * @return SFString The current value of marking.
   */
  SFString getMarking() const { return _marking; }

  /**
   * @brief Sets the value of marking. AccessType: inputOutput
   * @details Maximum of 11 characters for simple entity label.
   * @param value The new value for marking.
   */
  void setMarking(const SFString &value) { _marking = value; }

  void setMarking(SFString &&value) { _marking = std::move(value); }

  /**
   * @brief Gets the value of multicastRelayHost. AccessType: inputOutput
   * @details Fallback server address if multicast not available locally.
   * @return SFString The current value of multicastRelayHost.
   */
  SFString getMulticastRelayHost() const { return _multicastRelayHost; }

  /**
   * @brief Sets the value of multicastRelayHost. AccessType: inputOutput
   * @details Fallback server address if multicast not available locally.
   * @param value The new value for multicastRelayHost.
   */
  void setMulticastRelayHost(const SFString &value) {

    _multicastRelayHost = value;
  }

  void setMulticastRelayHost(SFString &&value) {

    _multicastRelayHost = std::move(value);
  }

  /**
   * @brief Gets the value of multicastRelayPort. AccessType: inputOutput
   * @details Fallback server port if multicast not available locally.
   * @return SFInt32 The current value of multicastRelayPort.
   */
  SFInt32 getMulticastRelayPort() const { return _multicastRelayPort; }

  /**
   * @brief Sets the value of multicastRelayPort. AccessType: inputOutput
   * @details Fallback server port if multicast not available locally.
   * @param value The new value for multicastRelayPort.
   */
  void setMulticastRelayPort(const SFInt32 &value) {

    _multicastRelayPort = value;
  }

  /**
   * @brief Gets the value of munitionApplicationID. AccessType: inputOutput
   * @details munitionApplicationID, unique for application at that site.
   * @return SFInt32 The current value of munitionApplicationID.
   */
  SFInt32 getMunitionApplicationID() const { return _munitionApplicationID; }

  /**
   * @brief Sets the value of munitionApplicationID. AccessType: inputOutput
   * @details munitionApplicationID, unique for application at that site.
   * @param value The new value for munitionApplicationID.
   */
  void setMunitionApplicationID(const SFInt32 &value) {

    _munitionApplicationID = value;
  }

  /**
   * @brief Gets the value of munitionEndPoint. AccessType: inputOutput
   * @details Final point of the munition path from firing weapon to detonation
   * or impact, in exercise coordinates.
   * @return SFVec3f The current value of munitionEndPoint.
   */
  SFVec3f getMunitionEndPoint() const { return _munitionEndPoint; }

  /**
   * @brief Sets the value of munitionEndPoint. AccessType: inputOutput
   * @details Final point of the munition path from firing weapon to detonation
   * or impact, in exercise coordinates.
   * @param value The new value for munitionEndPoint.
   */
  void setMunitionEndPoint(const SFVec3f &value) { _munitionEndPoint = value; }

  void setMunitionEndPoint(SFVec3f &&value) {

    _munitionEndPoint = std::move(value);
  }

  /**
   * @brief Gets the value of munitionEntityID. AccessType: inputOutput
   * @details munitionEntityID is unique ID for entity firing munition within
   * that application.
   * @return SFInt32 The current value of munitionEntityID.
   */
  SFInt32 getMunitionEntityID() const { return _munitionEntityID; }

  /**
   * @brief Sets the value of munitionEntityID. AccessType: inputOutput
   * @details munitionEntityID is unique ID for entity firing munition within
   * that application.
   * @param value The new value for munitionEntityID.
   */
  void setMunitionEntityID(const SFInt32 &value) { _munitionEntityID = value; }

  /**
   * @brief Gets the value of munitionQuantity. AccessType: inputOutput
   * @details Quantity of munitions fired.
   * @return SFInt32 The current value of munitionQuantity.
   */
  SFInt32 getMunitionQuantity() const { return _munitionQuantity; }

  /**
   * @brief Sets the value of munitionQuantity. AccessType: inputOutput
   * @details Quantity of munitions fired.
   * @param value The new value for munitionQuantity.
   */
  void setMunitionQuantity(const SFInt32 &value) { _munitionQuantity = value; }

  /**
   * @brief Gets the value of munitionSiteID. AccessType: inputOutput
   * @details Munition siteID of the participating LAN or organization.
   * @return SFInt32 The current value of munitionSiteID.
   */
  SFInt32 getMunitionSiteID() const { return _munitionSiteID; }

  /**
   * @brief Sets the value of munitionSiteID. AccessType: inputOutput
   * @details Munition siteID of the participating LAN or organization.
   * @param value The new value for munitionSiteID.
   */
  void setMunitionSiteID(const SFInt32 &value) { _munitionSiteID = value; }

  /**
   * @brief Gets the value of munitionStartPoint. AccessType: inputOutput
   * @details Initial point of the munition path from firing weapon to
   * detonation or impact, in exercise coordinates.
   * @return SFVec3f The current value of munitionStartPoint.
   */
  SFVec3f getMunitionStartPoint() const { return _munitionStartPoint; }

  /**
   * @brief Sets the value of munitionStartPoint. AccessType: inputOutput
   * @details Initial point of the munition path from firing weapon to
   * detonation or impact, in exercise coordinates.
   * @param value The new value for munitionStartPoint.
   */
  void setMunitionStartPoint(const SFVec3f &value) {

    _munitionStartPoint = value;
  }

  void setMunitionStartPoint(SFVec3f &&value) {

    _munitionStartPoint = std::move(value);
  }

  /**
   * @brief Gets the value of networkMode. AccessType: inputOutput
   * @details Whether this entity is ignoring the network, sending DIS packets
   * to the network, or receiving DIS packets from the network.
   * @return NetworkModeChoices The current value of networkMode.
   */
  NetworkModeChoices getNetworkMode() const { return _networkMode; }

  /**
   * @brief Sets the value of networkMode. AccessType: inputOutput
   * @details Whether this entity is ignoring the network, sending DIS packets
   * to the network, or receiving DIS packets from the network.
   * @param value The new value for networkMode.
   */
  void setNetworkMode(const NetworkModeChoices &value) { _networkMode = value; }

  /**
   * @brief Gets the value of port. AccessType: inputOutput
   * @details Network connection port number (EXAMPLE 3000) for sending or
   * receiving DIS messages.
   * @return SFInt32 The current value of port.
   */
  SFInt32 getPort() const { return _port; }

  /**
   * @brief Sets the value of port. AccessType: inputOutput
   * @details Network connection port number (EXAMPLE 3000) for sending or
   * receiving DIS messages.
   * @param value The new value for port.
   */
  void setPort(const SFInt32 &value) { _port = value; }

  /**
   * @brief Gets the value of readInterval. AccessType: inputOutput
   * @details Seconds between read updates, 0 means no reading.
   * @return SFTime The current value of readInterval.
   */
  SFTime getReadInterval() const { return _readInterval; }

  /**
   * @brief Sets the value of readInterval. AccessType: inputOutput
   * @details Seconds between read updates, 0 means no reading.
   * @param value The new value for readInterval.
   */
  void setReadInterval(const SFTime &value) {

    validateReadInterval(value);

    _readInterval = value;
  }

  void setReadInterval(SFTime &&value) {

    validateReadInterval(value);

    _readInterval = std::move(value);
  }

  /**
   * @brief Non-validating write of readInterval (runtime/reader ingest path).
   * @details Assigns without the range check setReadInterval() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setReadInterval() stays the
   *          enforcement point for programmatic callers.
   */
  void setReadIntervalUnchecked(const SFTime &value) { _readInterval = value; }

  /**
   * @brief Gets the value of rotation. AccessType: inputOutput
   * @details Orientation of children relative to local coordinate system,
   * usually read from (or written to) remote, networked EspduTransform nodes.
   * @return SFRotation The current value of rotation.
   */
  SFRotation getRotation() const { return _rotation; }

  /**
   * @brief Sets the value of rotation. AccessType: inputOutput
   * @details Orientation of children relative to local coordinate system,
   * usually read from (or written to) remote, networked EspduTransform nodes.
   * @param value The new value for rotation.
   */
  void setRotation(const SFRotation &value) { _rotation = value; }

  void setRotation(SFRotation &&value) { _rotation = std::move(value); }

  /**
   * @brief Gets the value of rtpHeaderExpected. AccessType: inputOutput
   * @details Whether RTP headers are prepended to DIS PDUs.
   * @return SFBool The current value of rtpHeaderExpected.
   */
  SFBool getRtpHeaderExpected() const { return _rtpHeaderExpected; }

  /**
   * @brief Sets the value of rtpHeaderExpected. AccessType: inputOutput
   * @details Whether RTP headers are prepended to DIS PDUs.
   * @param value The new value for rtpHeaderExpected.
   */
  void setRtpHeaderExpected(const SFBool &value) { _rtpHeaderExpected = value; }

  /**
   * @brief Gets the value of scale. AccessType: inputOutput
   * @details Non-uniform x-y-z scale of child coordinate system, adjusted by
   * center and scaleOrientation.
   * @return SFVec3f The current value of scale.
   */
  SFVec3f getScale() const { return _scale; }

  /**
   * @brief Sets the value of scale. AccessType: inputOutput
   * @details Non-uniform x-y-z scale of child coordinate system, adjusted by
   * center and scaleOrientation.
   * @param value The new value for scale.
   */
  void setScale(const SFVec3f &value) { _scale = value; }

  void setScale(SFVec3f &&value) { _scale = std::move(value); }

  /**
   * @brief Gets the value of scaleOrientation. AccessType: inputOutput
   * @details Preliminary rotation of coordinate system before scaling (to allow
   * scaling around arbitrary orientations).
   * @return SFRotation The current value of scaleOrientation.
   */
  SFRotation getScaleOrientation() const { return _scaleOrientation; }

  /**
   * @brief Sets the value of scaleOrientation. AccessType: inputOutput
   * @details Preliminary rotation of coordinate system before scaling (to allow
   * scaling around arbitrary orientations).
   * @param value The new value for scaleOrientation.
   */
  void setScaleOrientation(const SFRotation &value) {

    _scaleOrientation = value;
  }

  void setScaleOrientation(SFRotation &&value) {

    _scaleOrientation = std::move(value);
  }

  /**
   * @brief Event handler invoked when an event is received on
   * set_articulationParameterValue0. AccessType: inputOnly
   * @details Set element of user-defined payload array.
   *          Dispatches to the handler registered via
   * setOnSet_articulationParameterValue0Handler(); a no-op if none is set. The
   * event cascade reaches this through the node's reflected field table, so
   * routing is node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_articulationParameterValue0(const SFFloat &value) {
    if (_onSet_articulationParameterValue0Handler)
      _onSet_articulationParameterValue0Handler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * set_articulationParameterValue0.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnSet_articulationParameterValue0Handler(
      std::function<void(const SFFloat &)> handler) {
    _onSet_articulationParameterValue0Handler = std::move(handler);
  }

  /**
   * @brief Event handler invoked when an event is received on
   * set_articulationParameterValue1. AccessType: inputOnly
   * @details Set element of user-defined payload array.
   *          Dispatches to the handler registered via
   * setOnSet_articulationParameterValue1Handler(); a no-op if none is set. The
   * event cascade reaches this through the node's reflected field table, so
   * routing is node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_articulationParameterValue1(const SFFloat &value) {
    if (_onSet_articulationParameterValue1Handler)
      _onSet_articulationParameterValue1Handler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * set_articulationParameterValue1.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnSet_articulationParameterValue1Handler(
      std::function<void(const SFFloat &)> handler) {
    _onSet_articulationParameterValue1Handler = std::move(handler);
  }

  /**
   * @brief Event handler invoked when an event is received on
   * set_articulationParameterValue2. AccessType: inputOnly
   * @details Set element of user-defined payload array.
   *          Dispatches to the handler registered via
   * setOnSet_articulationParameterValue2Handler(); a no-op if none is set. The
   * event cascade reaches this through the node's reflected field table, so
   * routing is node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_articulationParameterValue2(const SFFloat &value) {
    if (_onSet_articulationParameterValue2Handler)
      _onSet_articulationParameterValue2Handler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * set_articulationParameterValue2.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnSet_articulationParameterValue2Handler(
      std::function<void(const SFFloat &)> handler) {
    _onSet_articulationParameterValue2Handler = std::move(handler);
  }

  /**
   * @brief Event handler invoked when an event is received on
   * set_articulationParameterValue3. AccessType: inputOnly
   * @details Set element of user-defined payload array.
   *          Dispatches to the handler registered via
   * setOnSet_articulationParameterValue3Handler(); a no-op if none is set. The
   * event cascade reaches this through the node's reflected field table, so
   * routing is node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_articulationParameterValue3(const SFFloat &value) {
    if (_onSet_articulationParameterValue3Handler)
      _onSet_articulationParameterValue3Handler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * set_articulationParameterValue3.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnSet_articulationParameterValue3Handler(
      std::function<void(const SFFloat &)> handler) {
    _onSet_articulationParameterValue3Handler = std::move(handler);
  }

  /**
   * @brief Event handler invoked when an event is received on
   * set_articulationParameterValue4. AccessType: inputOnly
   * @details Set element of user-defined payload array.
   *          Dispatches to the handler registered via
   * setOnSet_articulationParameterValue4Handler(); a no-op if none is set. The
   * event cascade reaches this through the node's reflected field table, so
   * routing is node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_articulationParameterValue4(const SFFloat &value) {
    if (_onSet_articulationParameterValue4Handler)
      _onSet_articulationParameterValue4Handler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * set_articulationParameterValue4.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnSet_articulationParameterValue4Handler(
      std::function<void(const SFFloat &)> handler) {
    _onSet_articulationParameterValue4Handler = std::move(handler);
  }

  /**
   * @brief Event handler invoked when an event is received on
   * set_articulationParameterValue5. AccessType: inputOnly
   * @details Set element of user-defined payload array.
   *          Dispatches to the handler registered via
   * setOnSet_articulationParameterValue5Handler(); a no-op if none is set. The
   * event cascade reaches this through the node's reflected field table, so
   * routing is node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_articulationParameterValue5(const SFFloat &value) {
    if (_onSet_articulationParameterValue5Handler)
      _onSet_articulationParameterValue5Handler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * set_articulationParameterValue5.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnSet_articulationParameterValue5Handler(
      std::function<void(const SFFloat &)> handler) {
    _onSet_articulationParameterValue5Handler = std::move(handler);
  }

  /**
   * @brief Event handler invoked when an event is received on
   * set_articulationParameterValue6. AccessType: inputOnly
   * @details Set element of user-defined payload array.
   *          Dispatches to the handler registered via
   * setOnSet_articulationParameterValue6Handler(); a no-op if none is set. The
   * event cascade reaches this through the node's reflected field table, so
   * routing is node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_articulationParameterValue6(const SFFloat &value) {
    if (_onSet_articulationParameterValue6Handler)
      _onSet_articulationParameterValue6Handler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * set_articulationParameterValue6.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnSet_articulationParameterValue6Handler(
      std::function<void(const SFFloat &)> handler) {
    _onSet_articulationParameterValue6Handler = std::move(handler);
  }

  /**
   * @brief Event handler invoked when an event is received on
   * set_articulationParameterValue7. AccessType: inputOnly
   * @details Set element of user-defined payload array.
   *          Dispatches to the handler registered via
   * setOnSet_articulationParameterValue7Handler(); a no-op if none is set. The
   * event cascade reaches this through the node's reflected field table, so
   * routing is node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_articulationParameterValue7(const SFFloat &value) {
    if (_onSet_articulationParameterValue7Handler)
      _onSet_articulationParameterValue7Handler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * set_articulationParameterValue7.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnSet_articulationParameterValue7Handler(
      std::function<void(const SFFloat &)> handler) {
    _onSet_articulationParameterValue7Handler = std::move(handler);
  }

  /**
   * @brief Gets the value of siteID. AccessType: inputOutput
   * @details Simulation/exercise siteID of the participating LAN or
   * organization.
   * @return SFInt32 The current value of siteID.
   */
  SFInt32 getSiteID() const { return _siteID; }

  /**
   * @brief Sets the value of siteID. AccessType: inputOutput
   * @details Simulation/exercise siteID of the participating LAN or
   * organization.
   * @param value The new value for siteID.
   */
  void setSiteID(const SFInt32 &value) { _siteID = value; }

  /**
   * @brief Gets the value of timestamp. AccessType: outputOnly
   * @details DIS timestamp received from latest PDU update, converted to X3D
   * SFTime units.
   * @return SFTime The current value of timestamp.
   */
  SFTime getTimestamp() const { return _timestamp; }

  /**
   * @brief Emit an output value on timestamp. AccessType: outputOnly
   * @details DIS timestamp received from latest PDU update, converted to X3D
   * SFTime units. outputOnly fields have no author-facing setter; a node's
   * behavior or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitTimestamp(const SFTime &value) { _timestamp = value; }

  /**
   * @brief Gets the value of translation. AccessType: inputOutput
   * @details Position of children relative to local coordinate system, usually
   * read from (or written to) remote, networked EspduTransform nodes.
   * @return SFVec3f The current value of translation.
   */
  SFVec3f getTranslation() const { return _translation; }

  /**
   * @brief Sets the value of translation. AccessType: inputOutput
   * @details Position of children relative to local coordinate system, usually
   * read from (or written to) remote, networked EspduTransform nodes.
   * @param value The new value for translation.
   */
  void setTranslation(const SFVec3f &value) { _translation = value; }

  void setTranslation(SFVec3f &&value) { _translation = std::move(value); }

  /**
   * @brief Gets the value of warhead. AccessType: inputOutput
   * @details Integer enumerations value for type of warhead on the munition.
   * @return SFInt32 The current value of warhead.
   */
  SFInt32 getWarhead() const { return _warhead; }

  /**
   * @brief Sets the value of warhead. AccessType: inputOutput
   * @details Integer enumerations value for type of warhead on the munition.
   * @param value The new value for warhead.
   */
  void setWarhead(const SFInt32 &value) { _warhead = value; }

  /**
   * @brief Gets the value of writeInterval. AccessType: inputOutput
   * @details Seconds between write updates, 0 means no writing (sending).
   * @return SFTime The current value of writeInterval.
   */
  SFTime getWriteInterval() const { return _writeInterval; }

  /**
   * @brief Sets the value of writeInterval. AccessType: inputOutput
   * @details Seconds between write updates, 0 means no writing (sending).
   * @param value The new value for writeInterval.
   */
  void setWriteInterval(const SFTime &value) {

    validateWriteInterval(value);

    _writeInterval = value;
  }

  void setWriteInterval(SFTime &&value) {

    validateWriteInterval(value);

    _writeInterval = std::move(value);
  }

  /**
   * @brief Non-validating write of writeInterval (runtime/reader ingest path).
   * @details Assigns without the range check setWriteInterval() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setWriteInterval() stays the
   *          enforcement point for programmatic callers.
   */
  void setWriteIntervalUnchecked(const SFTime &value) {
    _writeInterval = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "EspduTransform").
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
  static void checkRangesReadInterval(const SFTime &value,
                                      const std::string &nodeType,
                                      const std::string &defName,
                                      std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesWriteInterval(const SFTime &value,
                                       const std::string &nodeType,
                                       const std::string &defName,
                                       std::vector<RangeDiagnostic> &out);

private:
  static void validateReadInterval(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("readInterval below minimum of 0");
  }

  static void validateWriteInterval(const SFTime &value) {

    if (value < 0)
      throw std::out_of_range("writeInterval below minimum of 0");
  }

  /**
   * @brief Member variable for address.
   */

  SFString _address{"localhost"};

  /**
   * @brief Member variable for applicationID.
   */

  SFInt32 _applicationID{0};

  /**
   * @brief Member variable for articulationParameterArray.
   */

  MFFloat _articulationParameterArray{};

  /**
   * @brief Member variable for articulationParameterChangeIndicatorArray.
   */

  MFInt32 _articulationParameterChangeIndicatorArray{};

  /**
   * @brief Member variable for articulationParameterCount.
   */

  SFInt32 _articulationParameterCount{0};

  /**
   * @brief Member variable for articulationParameterDesignatorArray.
   */

  MFInt32 _articulationParameterDesignatorArray{};

  /**
   * @brief Member variable for articulationParameterIdPartAttachedToArray.
   */

  MFInt32 _articulationParameterIdPartAttachedToArray{};

  /**
   * @brief Member variable for articulationParameterTypeArray.
   */

  MFInt32 _articulationParameterTypeArray{};

  /**
   * @brief Member variable for articulationParameterValue0_changed.
   */

  SFFloat _articulationParameterValue0_changed{};

  /**
   * @brief Member variable for articulationParameterValue1_changed.
   */

  SFFloat _articulationParameterValue1_changed{};

  /**
   * @brief Member variable for articulationParameterValue2_changed.
   */

  SFFloat _articulationParameterValue2_changed{};

  /**
   * @brief Member variable for articulationParameterValue3_changed.
   */

  SFFloat _articulationParameterValue3_changed{};

  /**
   * @brief Member variable for articulationParameterValue4_changed.
   */

  SFFloat _articulationParameterValue4_changed{};

  /**
   * @brief Member variable for articulationParameterValue5_changed.
   */

  SFFloat _articulationParameterValue5_changed{};

  /**
   * @brief Member variable for articulationParameterValue6_changed.
   */

  SFFloat _articulationParameterValue6_changed{};

  /**
   * @brief Member variable for articulationParameterValue7_changed.
   */

  SFFloat _articulationParameterValue7_changed{};

  /**
   * @brief Member variable for center.
   */

  SFVec3f _center{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for collideTime.
   */

  SFTime _collideTime{};

  /**
   * @brief Member variable for collisionType.
   */

  SFInt32 _collisionType{0};

  /**
   * @brief Member variable for deadReckoning.
   */

  SFInt32 _deadReckoning{0};

  /**
   * @brief Member variable for description.
   */

  SFString _description{};

  /**
   * @brief Member variable for detonateTime.
   */

  SFTime _detonateTime{};

  /**
   * @brief Member variable for detonationLocation.
   */

  SFVec3f _detonationLocation{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for detonationRelativeLocation.
   */

  SFVec3f _detonationRelativeLocation{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for detonationResult.
   */

  SFInt32 _detonationResult{0};

  /**
   * @brief Member variable for enabled.
   */

  SFBool _enabled{true};

  /**
   * @brief Member variable for entityCategory.
   */

  SFInt32 _entityCategory{0};

  /**
   * @brief Member variable for entityCountry.
   */

  SFInt32 _entityCountry{0};

  /**
   * @brief Member variable for entityDomain.
   */

  SFInt32 _entityDomain{0};

  /**
   * @brief Member variable for entityExtra.
   */

  SFInt32 _entityExtra{0};

  /**
   * @brief Member variable for entityID.
   */

  SFInt32 _entityID{0};

  /**
   * @brief Member variable for entityKind.
   */

  SFInt32 _entityKind{0};

  /**
   * @brief Member variable for entitySpecific.
   */

  SFInt32 _entitySpecific{0};

  /**
   * @brief Member variable for entitySubcategory.
   */

  SFInt32 _entitySubcategory{0};

  /**
   * @brief Member variable for eventApplicationID.
   */

  SFInt32 _eventApplicationID{0};

  /**
   * @brief Member variable for eventEntityID.
   */

  SFInt32 _eventEntityID{0};

  /**
   * @brief Member variable for eventNumber.
   */

  SFInt32 _eventNumber{0};

  /**
   * @brief Member variable for eventSiteID.
   */

  SFInt32 _eventSiteID{0};

  /**
   * @brief Member variable for fired1.
   */

  SFBool _fired1{false};

  /**
   * @brief Member variable for fired2.
   */

  SFBool _fired2{false};

  /**
   * @brief Member variable for firedTime.
   */

  SFTime _firedTime{};

  /**
   * @brief Member variable for fireMissionIndex.
   */

  SFInt32 _fireMissionIndex{0};

  /**
   * @brief Member variable for firingRange.
   */

  SFFloat _firingRange{0};

  /**
   * @brief Member variable for firingRate.
   */

  SFInt32 _firingRate{0};

  /**
   * @brief Member variable for forceID.
   */

  SFInt32 _forceID{0};

  /**
   * @brief Member variable for fuse.
   */

  SFInt32 _fuse{0};

  /**
   * @brief Member variable for geoCoords.
   */

  SFVec3d _geoCoords{SFVec3d{0, 0, 0}};

  /**
   * @brief Member variable for geoSystem.
   */

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};

  /**
   * @brief Member variable for isActive.
   */

  SFBool _isActive{};

  /**
   * @brief Member variable for isCollided.
   */

  SFBool _isCollided{};

  /**
   * @brief Member variable for isDetonated.
   */

  SFBool _isDetonated{};

  /**
   * @brief Member variable for isNetworkReader.
   */

  SFBool _isNetworkReader{};

  /**
   * @brief Member variable for isNetworkWriter.
   */

  SFBool _isNetworkWriter{};

  /**
   * @brief Member variable for isRtpHeaderHeard.
   */

  SFBool _isRtpHeaderHeard{};

  /**
   * @brief Member variable for isStandAlone.
   */

  SFBool _isStandAlone{};

  /**
   * @brief Member variable for linearAcceleration.
   */

  SFVec3f _linearAcceleration{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for linearVelocity.
   */

  SFVec3f _linearVelocity{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for marking.
   */

  SFString _marking{};

  /**
   * @brief Member variable for multicastRelayHost.
   */

  SFString _multicastRelayHost{};

  /**
   * @brief Member variable for multicastRelayPort.
   */

  SFInt32 _multicastRelayPort{0};

  /**
   * @brief Member variable for munitionApplicationID.
   */

  SFInt32 _munitionApplicationID{0};

  /**
   * @brief Member variable for munitionEndPoint.
   */

  SFVec3f _munitionEndPoint{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for munitionEntityID.
   */

  SFInt32 _munitionEntityID{0};

  /**
   * @brief Member variable for munitionQuantity.
   */

  SFInt32 _munitionQuantity{0};

  /**
   * @brief Member variable for munitionSiteID.
   */

  SFInt32 _munitionSiteID{0};

  /**
   * @brief Member variable for munitionStartPoint.
   */

  SFVec3f _munitionStartPoint{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for networkMode.
   */

  NetworkModeChoices _networkMode{NetworkModeChoices::STANDALONE};

  /**
   * @brief Member variable for port.
   */

  SFInt32 _port{0};

  /**
   * @brief Member variable for readInterval.
   */

  SFTime _readInterval{0.1};

  /**
   * @brief Member variable for rotation.
   */

  SFRotation _rotation{SFRotation{0, 0, 1, 0}};

  /**
   * @brief Member variable for rtpHeaderExpected.
   */

  SFBool _rtpHeaderExpected{false};

  /**
   * @brief Member variable for scale.
   */

  SFVec3f _scale{SFVec3f{1, 1, 1}};

  /**
   * @brief Member variable for scaleOrientation.
   */

  SFRotation _scaleOrientation{SFRotation{0, 0, 1, 0}};

  /**
   * @brief Registered event handler for set_articulationParameterValue0
   * (inputOnly); empty until set.
   */
  std::function<void(const SFFloat &)>
      _onSet_articulationParameterValue0Handler{};

  /**
   * @brief Registered event handler for set_articulationParameterValue1
   * (inputOnly); empty until set.
   */
  std::function<void(const SFFloat &)>
      _onSet_articulationParameterValue1Handler{};

  /**
   * @brief Registered event handler for set_articulationParameterValue2
   * (inputOnly); empty until set.
   */
  std::function<void(const SFFloat &)>
      _onSet_articulationParameterValue2Handler{};

  /**
   * @brief Registered event handler for set_articulationParameterValue3
   * (inputOnly); empty until set.
   */
  std::function<void(const SFFloat &)>
      _onSet_articulationParameterValue3Handler{};

  /**
   * @brief Registered event handler for set_articulationParameterValue4
   * (inputOnly); empty until set.
   */
  std::function<void(const SFFloat &)>
      _onSet_articulationParameterValue4Handler{};

  /**
   * @brief Registered event handler for set_articulationParameterValue5
   * (inputOnly); empty until set.
   */
  std::function<void(const SFFloat &)>
      _onSet_articulationParameterValue5Handler{};

  /**
   * @brief Registered event handler for set_articulationParameterValue6
   * (inputOnly); empty until set.
   */
  std::function<void(const SFFloat &)>
      _onSet_articulationParameterValue6Handler{};

  /**
   * @brief Registered event handler for set_articulationParameterValue7
   * (inputOnly); empty until set.
   */
  std::function<void(const SFFloat &)>
      _onSet_articulationParameterValue7Handler{};

  /**
   * @brief Member variable for siteID.
   */

  SFInt32 _siteID{0};

  /**
   * @brief Member variable for timestamp.
   */

  SFTime _timestamp{};

  /**
   * @brief Member variable for translation.
   */

  SFVec3f _translation{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for warhead.
   */

  SFInt32 _warhead{0};

  /**
   * @brief Member variable for writeInterval.
   */

  SFTime _writeInterval{1.0};
};

#endif // ESPDUTRANSFORM_HPP