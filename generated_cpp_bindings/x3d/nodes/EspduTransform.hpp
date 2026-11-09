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

#include "x3d/nodes/X3DGroupingNode.hpp"

#include "x3d/nodes/X3DSensorNode.hpp"

#include "x3d/nodes/X3DNetworkSensorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

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
  EspduTransform() = default;

  ~EspduTransform() = default;

  static SFString getDefaultAddress() { return "localhost"; }

  static SFInt32 getDefaultApplicationID() { return 0; }

  static SFInt32 getDefaultArticulationParameterCount() { return 0; }

  static SFVec3f getDefaultCenter() { return SFVec3f{0, 0, 0}; }

  static SFInt32 getDefaultCollisionType() { return 0; }

  static SFInt32 getDefaultDeadReckoning() { return 0; }

  static SFVec3f getDefaultDetonationLocation() { return SFVec3f{0, 0, 0}; }

  static SFVec3f getDefaultDetonationRelativeLocation() {
    return SFVec3f{0, 0, 0};
  }

  static SFInt32 getDefaultDetonationResult() { return 0; }

  static SFBool getDefaultEnabled() { return true; }

  static SFInt32 getDefaultEntityCategory() { return 0; }

  static SFInt32 getDefaultEntityCountry() { return 0; }

  static SFInt32 getDefaultEntityDomain() { return 0; }

  static SFInt32 getDefaultEntityExtra() { return 0; }

  static SFInt32 getDefaultEntityID() { return 0; }

  static SFInt32 getDefaultEntityKind() { return 0; }

  static SFInt32 getDefaultEntitySpecific() { return 0; }

  static SFInt32 getDefaultEntitySubcategory() { return 0; }

  static SFInt32 getDefaultEventApplicationID() { return 0; }

  static SFInt32 getDefaultEventEntityID() { return 0; }

  static SFInt32 getDefaultEventNumber() { return 0; }

  static SFInt32 getDefaultEventSiteID() { return 0; }

  static SFBool getDefaultFired1() { return false; }

  static SFBool getDefaultFired2() { return false; }

  static SFInt32 getDefaultFireMissionIndex() { return 0; }

  static SFFloat getDefaultFiringRange() { return 0; }

  static SFInt32 getDefaultFiringRate() { return 0; }

  static SFInt32 getDefaultForceID() { return 0; }

  static SFInt32 getDefaultFuse() { return 0; }

  static SFVec3d getDefaultGeoCoords() { return SFVec3d{0, 0, 0}; }

  static MFString getDefaultGeoSystem() {
    return std::vector<std::string>{"GD", "WE"};
  }

  static SFVec3f getDefaultLinearAcceleration() { return SFVec3f{0, 0, 0}; }

  static SFVec3f getDefaultLinearVelocity() { return SFVec3f{0, 0, 0}; }

  static SFInt32 getDefaultMulticastRelayPort() { return 0; }

  static SFInt32 getDefaultMunitionApplicationID() { return 0; }

  static SFVec3f getDefaultMunitionEndPoint() { return SFVec3f{0, 0, 0}; }

  static SFInt32 getDefaultMunitionEntityID() { return 0; }

  static SFInt32 getDefaultMunitionQuantity() { return 0; }

  static SFInt32 getDefaultMunitionSiteID() { return 0; }

  static SFVec3f getDefaultMunitionStartPoint() { return SFVec3f{0, 0, 0}; }

  static NetworkModeChoices getDefaultNetworkMode() {
    return NetworkModeChoices::STANDALONE;
  }

  static SFInt32 getDefaultPort() { return 0; }

  static SFTime getDefaultReadInterval() { return 0.1; }

  static SFRotation getDefaultRotation() { return SFRotation{0, 0, 1, 0}; }

  static SFBool getDefaultRtpHeaderExpected() { return false; }

  static SFVec3f getDefaultScale() { return SFVec3f{1, 1, 1}; }

  static SFRotation getDefaultScaleOrientation() {
    return SFRotation{0, 0, 1, 0};
  }

  static SFInt32 getDefaultSiteID() { return 0; }

  static SFVec3f getDefaultTranslation() { return SFVec3f{0, 0, 0}; }

  static SFInt32 getDefaultWarhead() { return 0; }

  static SFTime getDefaultWriteInterval() { return 1.0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "DIS"; }

  static int componentLevel() { return 1; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Multicast network address, or else 'localhost'; Example: 224.
   */
  const SFString &getAddress() const { return _address; }

  /**
   * Access: inputOutput.
   * @details Multicast network address, or else 'localhost'; Example: 224.
   */
  void setAddress(const SFString &value) { _address = value; }

  void setAddress(SFString &&value) { _address = std::move(value); }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Simulation/exercise applicationID is unique for application at
   * that site.
   */
  const SFInt32 &getApplicationID() const { return _applicationID; }

  /**
   * Access: inputOutput.
   * @details Simulation/exercise applicationID is unique for application at
   * that site.
   */
  void setApplicationID(const SFInt32 &value) { _applicationID = value; }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details Information required for representation of the entity's visual
   * appearance and position of its articulated parts.
   */
  const MFFloat &getArticulationParameterArray() const {
    return _articulationParameterArray;
  }

  /**
   * Access: inputOutput.
   * @details Information required for representation of the entity's visual
   * appearance and position of its articulated parts.
   */
  void setArticulationParameterArray(const MFFloat &value) {

    _articulationParameterArray = value;
  }

  void setArticulationParameterArray(MFFloat &&value) {

    _articulationParameterArray = std::move(value);
  }

  /**
   * MFInt32; access: inputOutput. Borrows the stored value.
   * @details Array of change counters, each incremented when an articulated
   * parameter is updated.
   */
  const MFInt32 &getArticulationParameterChangeIndicatorArray() const {
    return _articulationParameterChangeIndicatorArray;
  }

  /**
   * Access: inputOutput.
   * @details Array of change counters, each incremented when an articulated
   * parameter is updated.
   */
  void setArticulationParameterChangeIndicatorArray(const MFInt32 &value) {

    _articulationParameterChangeIndicatorArray = value;
  }

  void setArticulationParameterChangeIndicatorArray(MFInt32 &&value) {

    _articulationParameterChangeIndicatorArray = std::move(value);
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Number of articulated parameters attached to this entity state
   * PDU.
   */
  const SFInt32 &getArticulationParameterCount() const {
    return _articulationParameterCount;
  }

  /**
   * Access: inputOutput.
   * @details Number of articulated parameters attached to this entity state
   * PDU.
   */
  void setArticulationParameterCount(const SFInt32 &value) {

    _articulationParameterCount = value;
  }

  /**
   * MFInt32; access: inputOutput. Borrows the stored value.
   * @details Array of designators for each articulated parameter.
   */
  const MFInt32 &getArticulationParameterDesignatorArray() const {
    return _articulationParameterDesignatorArray;
  }

  /**
   * Access: inputOutput.
   * @details Array of designators for each articulated parameter.
   */
  void setArticulationParameterDesignatorArray(const MFInt32 &value) {

    _articulationParameterDesignatorArray = value;
  }

  void setArticulationParameterDesignatorArray(MFInt32 &&value) {

    _articulationParameterDesignatorArray = std::move(value);
  }

  /**
   * MFInt32; access: inputOutput. Borrows the stored value.
   * @details Array of ID parts that each articulated parameter is attached to.
   */
  const MFInt32 &getArticulationParameterIdPartAttachedToArray() const {
    return _articulationParameterIdPartAttachedToArray;
  }

  /**
   * Access: inputOutput.
   * @details Array of ID parts that each articulated parameter is attached to.
   */
  void setArticulationParameterIdPartAttachedToArray(const MFInt32 &value) {

    _articulationParameterIdPartAttachedToArray = value;
  }

  void setArticulationParameterIdPartAttachedToArray(MFInt32 &&value) {

    _articulationParameterIdPartAttachedToArray = std::move(value);
  }

  /**
   * MFInt32; access: inputOutput. Borrows the stored value.
   * @details Array of type enumerations for each articulated parameter element.
   */
  const MFInt32 &getArticulationParameterTypeArray() const {
    return _articulationParameterTypeArray;
  }

  /**
   * Access: inputOutput.
   * @details Array of type enumerations for each articulated parameter element.
   */
  void setArticulationParameterTypeArray(const MFInt32 &value) {

    _articulationParameterTypeArray = value;
  }

  void setArticulationParameterTypeArray(MFInt32 &&value) {

    _articulationParameterTypeArray = std::move(value);
  }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details Get element of user-defined payload array.
   */
  const SFFloat &getArticulationParameterValue0_changed() const {
    return _articulationParameterValue0_changed;
  }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Get element of user-defined payload array.
   */
  void emitArticulationParameterValue0_changed(const SFFloat &value) {
    _articulationParameterValue0_changed = value;
  }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details Get element of user-defined payload array.
   */
  const SFFloat &getArticulationParameterValue1_changed() const {
    return _articulationParameterValue1_changed;
  }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Get element of user-defined payload array.
   */
  void emitArticulationParameterValue1_changed(const SFFloat &value) {
    _articulationParameterValue1_changed = value;
  }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details Get element of user-defined payload array.
   */
  const SFFloat &getArticulationParameterValue2_changed() const {
    return _articulationParameterValue2_changed;
  }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Get element of user-defined payload array.
   */
  void emitArticulationParameterValue2_changed(const SFFloat &value) {
    _articulationParameterValue2_changed = value;
  }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details Get element of user-defined payload array.
   */
  const SFFloat &getArticulationParameterValue3_changed() const {
    return _articulationParameterValue3_changed;
  }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Get element of user-defined payload array.
   */
  void emitArticulationParameterValue3_changed(const SFFloat &value) {
    _articulationParameterValue3_changed = value;
  }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details Get element of user-defined payload array.
   */
  const SFFloat &getArticulationParameterValue4_changed() const {
    return _articulationParameterValue4_changed;
  }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Get element of user-defined payload array.
   */
  void emitArticulationParameterValue4_changed(const SFFloat &value) {
    _articulationParameterValue4_changed = value;
  }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details Get element of user-defined payload array.
   */
  const SFFloat &getArticulationParameterValue5_changed() const {
    return _articulationParameterValue5_changed;
  }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Get element of user-defined payload array.
   */
  void emitArticulationParameterValue5_changed(const SFFloat &value) {
    _articulationParameterValue5_changed = value;
  }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details Get element of user-defined payload array.
   */
  const SFFloat &getArticulationParameterValue6_changed() const {
    return _articulationParameterValue6_changed;
  }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Get element of user-defined payload array.
   */
  void emitArticulationParameterValue6_changed(const SFFloat &value) {
    _articulationParameterValue6_changed = value;
  }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details Get element of user-defined payload array.
   */
  const SFFloat &getArticulationParameterValue7_changed() const {
    return _articulationParameterValue7_changed;
  }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Get element of user-defined payload array.
   */
  void emitArticulationParameterValue7_changed(const SFFloat &value) {
    _articulationParameterValue7_changed = value;
  }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Translation offset from origin of local coordinate system.
   */
  const SFVec3f &getCenter() const { return _center; }

  /**
   * Access: inputOutput.
   * @details Translation offset from origin of local coordinate system.
   */
  void setCenter(const SFVec3f &value) { _center = value; }

  void setCenter(SFVec3f &&value) { _center = std::move(value); }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details When were we collided with? Warning: it is an error to define this
   * transient outputOnly field in an X3D file, instead only use it a source for
   * ROUTE events.
   */
  const SFTime &getCollideTime() const { return _collideTime; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details When were we collided with? Warning: it is an error to define this
   * transient outputOnly field in an X3D file, instead only use it a source for
   * ROUTE events.
   */
  void emitCollideTime(const SFTime &value) { _collideTime = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Integer enumeration for type of collision: ELASTIC or INELASTIC.
   */
  const SFInt32 &getCollisionType() const { return _collisionType; }

  /**
   * Access: inputOutput.
   * @details Integer enumeration for type of collision: ELASTIC or INELASTIC.
   */
  void setCollisionType(const SFInt32 &value) { _collisionType = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Dead reckoning algorithm being used to project
   * position/orientation with velocities/accelerations.
   */
  const SFInt32 &getDeadReckoning() const { return _deadReckoning; }

  /**
   * Access: inputOutput.
   * @details Dead reckoning algorithm being used to project
   * position/orientation with velocities/accelerations.
   */
  void setDeadReckoning(const SFInt32 &value) { _deadReckoning = value; }

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
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details When were we detonated?.
   */
  const SFTime &getDetonateTime() const { return _detonateTime; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details When were we detonated?.
   */
  void emitDetonateTime(const SFTime &value) { _detonateTime = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details World coordinates for detonationLocation.
   */
  const SFVec3f &getDetonationLocation() const { return _detonationLocation; }

  /**
   * Access: inputOutput.
   * @details World coordinates for detonationLocation.
   */
  void setDetonationLocation(const SFVec3f &value) {

    _detonationLocation = value;
  }

  void setDetonationLocation(SFVec3f &&value) {

    _detonationLocation = std::move(value);
  }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Relative coordinates for detonation location.
   */
  const SFVec3f &getDetonationRelativeLocation() const {
    return _detonationRelativeLocation;
  }

  /**
   * Access: inputOutput.
   * @details Relative coordinates for detonation location.
   */
  void setDetonationRelativeLocation(const SFVec3f &value) {

    _detonationRelativeLocation = value;
  }

  void setDetonationRelativeLocation(SFVec3f &&value) {

    _detonationRelativeLocation = std::move(value);
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Integer enumeration for type of detonation and result that
   * occurred.
   */
  const SFInt32 &getDetonationResult() const { return _detonationResult; }

  /**
   * Access: inputOutput.
   * @details Integer enumeration for type of detonation and result that
   * occurred.
   */
  void setDetonationResult(const SFInt32 &value) { _detonationResult = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Enables/disables the sensor node.
   */
  const SFBool &getEnabled() const { return _enabled; }

  /**
   * Access: inputOutput.
   * @details Enables/disables the sensor node.
   */
  void setEnabled(const SFBool &value) { _enabled = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Integer enumerations value for main category that describes the
   * entity, semantics of each code varies according to domain.
   */
  const SFInt32 &getEntityCategory() const { return _entityCategory; }

  /**
   * Access: inputOutput.
   * @details Integer enumerations value for main category that describes the
   * entity, semantics of each code varies according to domain.
   */
  void setEntityCategory(const SFInt32 &value) { _entityCategory = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Integer enumerations value for country to which the design of the
   * entity or its design specification is attributed.
   */
  const SFInt32 &getEntityCountry() const { return _entityCountry; }

  /**
   * Access: inputOutput.
   * @details Integer enumerations value for country to which the design of the
   * entity or its design specification is attributed.
   */
  void setEntityCountry(const SFInt32 &value) { _entityCountry = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Integer enumerations value for domain in which the entity
   * operates: LAND, AIR, SURFACE, SUBSURFACE, SPACE or OTHER.
   */
  const SFInt32 &getEntityDomain() const { return _entityDomain; }

  /**
   * Access: inputOutput.
   * @details Integer enumerations value for domain in which the entity
   * operates: LAND, AIR, SURFACE, SUBSURFACE, SPACE or OTHER.
   */
  void setEntityDomain(const SFInt32 &value) { _entityDomain = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Any extra information required to describe a particular entity.
   */
  const SFInt32 &getEntityExtra() const { return _entityExtra; }

  /**
   * Access: inputOutput.
   * @details Any extra information required to describe a particular entity.
   */
  void setEntityExtra(const SFInt32 &value) { _entityExtra = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Simulation/exercise entityID is a unique ID for a single entity
   * within that application.
   */
  const SFInt32 &getEntityID() const { return _entityID; }

  /**
   * Access: inputOutput.
   * @details Simulation/exercise entityID is a unique ID for a single entity
   * within that application.
   */
  void setEntityID(const SFInt32 &value) { _entityID = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Integer enumerations value for whether entity is a PLATFORM,
   * MUNITION, LIFE_FORM, ENVIRONMENTAL, CULTURAL_FEATURE, SUPPLY, RADIO,
   * EXPENDABLE, SENSOR_EMITTER or OTHER.
   */
  const SFInt32 &getEntityKind() const { return _entityKind; }

  /**
   * Access: inputOutput.
   * @details Integer enumerations value for whether entity is a PLATFORM,
   * MUNITION, LIFE_FORM, ENVIRONMENTAL, CULTURAL_FEATURE, SUPPLY, RADIO,
   * EXPENDABLE, SENSOR_EMITTER or OTHER.
   */
  void setEntityKind(const SFInt32 &value) { _entityKind = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Specific information about an entity based on the Subcategory
   * field.
   */
  const SFInt32 &getEntitySpecific() const { return _entitySpecific; }

  /**
   * Access: inputOutput.
   * @details Specific information about an entity based on the Subcategory
   * field.
   */
  void setEntitySpecific(const SFInt32 &value) { _entitySpecific = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Integer enumerations value for particular subcategory to which an
   * entity belongs based on the category field.
   */
  const SFInt32 &getEntitySubcategory() const { return _entitySubcategory; }

  /**
   * Access: inputOutput.
   * @details Integer enumerations value for particular subcategory to which an
   * entity belongs based on the category field.
   */
  void setEntitySubcategory(const SFInt32 &value) {

    _entitySubcategory = value;
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Simulation/exercise eventApplicationID is unique for events
   * generated from application at that site.
   */
  const SFInt32 &getEventApplicationID() const { return _eventApplicationID; }

  /**
   * Access: inputOutput.
   * @details Simulation/exercise eventApplicationID is unique for events
   * generated from application at that site.
   */
  void setEventApplicationID(const SFInt32 &value) {

    _eventApplicationID = value;
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details For a given event, simulation/exercise entityID is a unique ID for
   * a single entity within that application.
   */
  const SFInt32 &getEventEntityID() const { return _eventEntityID; }

  /**
   * Access: inputOutput.
   * @details For a given event, simulation/exercise entityID is a unique ID for
   * a single entity within that application.
   */
  void setEventEntityID(const SFInt32 &value) { _eventEntityID = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Sequential number of each event issued by an application.
   */
  const SFInt32 &getEventNumber() const { return _eventNumber; }

  /**
   * Access: inputOutput.
   * @details Sequential number of each event issued by an application.
   */
  void setEventNumber(const SFInt32 &value) { _eventNumber = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Simulation/exercise siteID of the participating LAN or
   * organization.
   */
  const SFInt32 &getEventSiteID() const { return _eventSiteID; }

  /**
   * Access: inputOutput.
   * @details Simulation/exercise siteID of the participating LAN or
   * organization.
   */
  void setEventSiteID(const SFInt32 &value) { _eventSiteID = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Has the primary weapon (Fire PDU) been fired?.
   */
  const SFBool &getFired1() const { return _fired1; }

  /**
   * Access: inputOutput.
   * @details Has the primary weapon (Fire PDU) been fired?.
   */
  void setFired1(const SFBool &value) { _fired1 = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Has the secondary weapon (Fire PDU) been fired?.
   */
  const SFBool &getFired2() const { return _fired2; }

  /**
   * Access: inputOutput.
   * @details Has the secondary weapon (Fire PDU) been fired?.
   */
  void setFired2(const SFBool &value) { _fired2 = value; }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details When did we shoot a weapon (Fire PDU)? Warning: it is an error to
   * define this transient outputOnly field in an X3D file, instead only use it
   * a source for ROUTE events.
   */
  const SFTime &getFiredTime() const { return _firedTime; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details When did we shoot a weapon (Fire PDU)? Warning: it is an error to
   * define this transient outputOnly field in an X3D file, instead only use it
   * a source for ROUTE events.
   */
  void emitFiredTime(const SFTime &value) { _firedTime = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details .
   */
  const SFInt32 &getFireMissionIndex() const { return _fireMissionIndex; }

  /**
   * Access: inputOutput.
   * @details .
   */
  void setFireMissionIndex(const SFInt32 &value) { _fireMissionIndex = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Range (three dimension, straight-line distance) that the firing
   * entity's fire control system has assumed for computing the fire control
   * solution if a weapon and if the value is known.
   */
  const SFFloat &getFiringRange() const { return _firingRange; }

  /**
   * Access: inputOutput.
   * @details Range (three dimension, straight-line distance) that the firing
   * entity's fire control system has assumed for computing the fire control
   * solution if a weapon and if the value is known.
   */
  void setFiringRange(const SFFloat &value) { _firingRange = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Rate at which munitions are fired.
   */
  const SFInt32 &getFiringRate() const { return _firingRate; }

  /**
   * Access: inputOutput.
   * @details Rate at which munitions are fired.
   */
  void setFiringRate(const SFInt32 &value) { _firingRate = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details forceID determines the team membership of the issuing entity, and
   * whether FRIENDLY OPPOSING or NEUTRAL or OTHER.
   */
  const SFInt32 &getForceID() const { return _forceID; }

  /**
   * Access: inputOutput.
   * @details forceID determines the team membership of the issuing entity, and
   * whether FRIENDLY OPPOSING or NEUTRAL or OTHER.
   */
  void setForceID(const SFInt32 &value) { _forceID = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Integer enumerations value for type of fuse on the munition.
   */
  const SFInt32 &getFuse() const { return _fuse; }

  /**
   * Access: inputOutput.
   * @details Integer enumerations value for type of fuse on the munition.
   */
  void setFuse(const SFInt32 &value) { _fuse = value; }

  /**
   * SFVec3d; access: inputOutput. Borrows the stored value.
   * @details Geographic location (specified in current geoSystem coordinates)
   * for children geometry (specified in relative coordinate system, in meters).
   */
  const SFVec3d &getGeoCoords() const { return _geoCoords; }

  /**
   * Access: inputOutput.
   * @details Geographic location (specified in current geoSystem coordinates)
   * for children geometry (specified in relative coordinate system, in meters).
   */
  void setGeoCoords(const SFVec3d &value) { _geoCoords = value; }

  void setGeoCoords(SFVec3d &&value) { _geoCoords = std::move(value); }

  /**
   * MFString; access: initializeOnly. Borrows the stored value.
   * @details Identifies spatial reference frame: Geodetic (GD), Geocentric
   * (GC), Universal Transverse Mercator (UTM).
   */
  const MFString &getGeoSystem() const { return _geoSystem; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setGeoSystemUnchecked(const MFString &value) { _geoSystem = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details Have we received a network update recently? Warning: it is an
   * error to define this transient outputOnly field in an X3D file, instead
   * only use it a source for ROUTE events.
   */
  const SFBool &getIsActive() const { return _isActive; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Have we received a network update recently? Warning: it is an
   * error to define this transient outputOnly field in an X3D file, instead
   * only use it a source for ROUTE events.
   */
  void emitIsActive(const SFBool &value) { _isActive = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details Has a matching CollisionPDU reported a collision? Warning: it is
   * an error to define this transient outputOnly field in an X3D file, instead
   * only use it a source for ROUTE events.
   */
  const SFBool &getIsCollided() const { return _isCollided; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Has a matching CollisionPDU reported a collision? Warning: it is
   * an error to define this transient outputOnly field in an X3D file, instead
   * only use it a source for ROUTE events.
   */
  void emitIsCollided(const SFBool &value) { _isCollided = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details Has a matching DetonationPDU reported a detonation? Warning: it is
   * an error to define this transient outputOnly field in an X3D file, instead
   * only use it a source for ROUTE events.
   */
  const SFBool &getIsDetonated() const { return _isDetonated; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Has a matching DetonationPDU reported a detonation? Warning: it is
   * an error to define this transient outputOnly field in an X3D file, instead
   * only use it a source for ROUTE events.
   */
  void emitIsDetonated(const SFBool &value) { _isDetonated = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details Whether networkMode='remote' (listen to network as copy of remote
   * entity).
   */
  const SFBool &getIsNetworkReader() const { return _isNetworkReader; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Whether networkMode='remote' (listen to network as copy of remote
   * entity).
   */
  void emitIsNetworkReader(const SFBool &value) { _isNetworkReader = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details Whether networkMode='master' (output to network as master entity
   * at writeInterval).
   */
  const SFBool &getIsNetworkWriter() const { return _isNetworkWriter; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Whether networkMode='master' (output to network as master entity
   * at writeInterval).
   */
  void emitIsNetworkWriter(const SFBool &value) { _isNetworkWriter = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details Whether incoming DIS packets have an RTP header prepended.
   */
  const SFBool &getIsRtpHeaderHeard() const { return _isRtpHeaderHeard; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Whether incoming DIS packets have an RTP header prepended.
   */
  void emitIsRtpHeaderHeard(const SFBool &value) { _isRtpHeaderHeard = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details Whether networkMode='local' (ignore network but still respond to
   * local events).
   */
  const SFBool &getIsStandAlone() const { return _isStandAlone; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Whether networkMode='local' (ignore network but still respond to
   * local events).
   */
  void emitIsStandAlone(const SFBool &value) { _isStandAlone = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Acceleration of the entity relative to the rotating Earth in
   * either world or entity coordinates, depending on the dead reckoning
   * algorithm used.
   */
  const SFVec3f &getLinearAcceleration() const { return _linearAcceleration; }

  /**
   * Access: inputOutput.
   * @details Acceleration of the entity relative to the rotating Earth in
   * either world or entity coordinates, depending on the dead reckoning
   * algorithm used.
   */
  void setLinearAcceleration(const SFVec3f &value) {

    _linearAcceleration = value;
  }

  void setLinearAcceleration(SFVec3f &&value) {

    _linearAcceleration = std::move(value);
  }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Velocity of the entity relative to the rotating Earth in either
   * world or entity coordinates, depending on the dead reckoning algorithm
   * used.
   */
  const SFVec3f &getLinearVelocity() const { return _linearVelocity; }

  /**
   * Access: inputOutput.
   * @details Velocity of the entity relative to the rotating Earth in either
   * world or entity coordinates, depending on the dead reckoning algorithm
   * used.
   */
  void setLinearVelocity(const SFVec3f &value) { _linearVelocity = value; }

  void setLinearVelocity(SFVec3f &&value) {

    _linearVelocity = std::move(value);
  }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Maximum of 11 characters for simple entity label.
   */
  const SFString &getMarking() const { return _marking; }

  /**
   * Access: inputOutput.
   * @details Maximum of 11 characters for simple entity label.
   */
  void setMarking(const SFString &value) { _marking = value; }

  void setMarking(SFString &&value) { _marking = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Fallback server address if multicast not available locally.
   */
  const SFString &getMulticastRelayHost() const { return _multicastRelayHost; }

  /**
   * Access: inputOutput.
   * @details Fallback server address if multicast not available locally.
   */
  void setMulticastRelayHost(const SFString &value) {

    _multicastRelayHost = value;
  }

  void setMulticastRelayHost(SFString &&value) {

    _multicastRelayHost = std::move(value);
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Fallback server port if multicast not available locally.
   */
  const SFInt32 &getMulticastRelayPort() const { return _multicastRelayPort; }

  /**
   * Access: inputOutput.
   * @details Fallback server port if multicast not available locally.
   */
  void setMulticastRelayPort(const SFInt32 &value) {

    _multicastRelayPort = value;
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details munitionApplicationID, unique for application at that site.
   */
  const SFInt32 &getMunitionApplicationID() const {
    return _munitionApplicationID;
  }

  /**
   * Access: inputOutput.
   * @details munitionApplicationID, unique for application at that site.
   */
  void setMunitionApplicationID(const SFInt32 &value) {

    _munitionApplicationID = value;
  }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Final point of the munition path from firing weapon to detonation
   * or impact, in exercise coordinates.
   */
  const SFVec3f &getMunitionEndPoint() const { return _munitionEndPoint; }

  /**
   * Access: inputOutput.
   * @details Final point of the munition path from firing weapon to detonation
   * or impact, in exercise coordinates.
   */
  void setMunitionEndPoint(const SFVec3f &value) { _munitionEndPoint = value; }

  void setMunitionEndPoint(SFVec3f &&value) {

    _munitionEndPoint = std::move(value);
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details munitionEntityID is unique ID for entity firing munition within
   * that application.
   */
  const SFInt32 &getMunitionEntityID() const { return _munitionEntityID; }

  /**
   * Access: inputOutput.
   * @details munitionEntityID is unique ID for entity firing munition within
   * that application.
   */
  void setMunitionEntityID(const SFInt32 &value) { _munitionEntityID = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Quantity of munitions fired.
   */
  const SFInt32 &getMunitionQuantity() const { return _munitionQuantity; }

  /**
   * Access: inputOutput.
   * @details Quantity of munitions fired.
   */
  void setMunitionQuantity(const SFInt32 &value) { _munitionQuantity = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Munition siteID of the participating LAN or organization.
   */
  const SFInt32 &getMunitionSiteID() const { return _munitionSiteID; }

  /**
   * Access: inputOutput.
   * @details Munition siteID of the participating LAN or organization.
   */
  void setMunitionSiteID(const SFInt32 &value) { _munitionSiteID = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Initial point of the munition path from firing weapon to
   * detonation or impact, in exercise coordinates.
   */
  const SFVec3f &getMunitionStartPoint() const { return _munitionStartPoint; }

  /**
   * Access: inputOutput.
   * @details Initial point of the munition path from firing weapon to
   * detonation or impact, in exercise coordinates.
   */
  void setMunitionStartPoint(const SFVec3f &value) {

    _munitionStartPoint = value;
  }

  void setMunitionStartPoint(SFVec3f &&value) {

    _munitionStartPoint = std::move(value);
  }

  /**
   * NetworkModeChoices; access: inputOutput. Borrows the stored value.
   * @details Whether this entity is ignoring the network, sending DIS packets
   * to the network, or receiving DIS packets from the network.
   */
  const NetworkModeChoices &getNetworkMode() const { return _networkMode; }

  /**
   * Access: inputOutput.
   * @details Whether this entity is ignoring the network, sending DIS packets
   * to the network, or receiving DIS packets from the network.
   */
  void setNetworkMode(const NetworkModeChoices &value) { _networkMode = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Network connection port number (EXAMPLE 3000) for sending or
   * receiving DIS messages.
   */
  const SFInt32 &getPort() const { return _port; }

  /**
   * Access: inputOutput.
   * @details Network connection port number (EXAMPLE 3000) for sending or
   * receiving DIS messages.
   */
  void setPort(const SFInt32 &value) { _port = value; }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details Seconds between read updates, 0 means no reading.
   */
  const SFTime &getReadInterval() const { return _readInterval; }

  /**
   * Access: inputOutput.
   * @details Seconds between read updates, 0 means no reading.
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
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setReadInterval() for range validation.
   */
  void setReadIntervalUnchecked(const SFTime &value) { _readInterval = value; }

  /**
   * SFRotation; access: inputOutput. Borrows the stored value.
   * @details Orientation of children relative to local coordinate system,
   * usually read from (or written to) remote, networked EspduTransform nodes.
   */
  const SFRotation &getRotation() const { return _rotation; }

  /**
   * Access: inputOutput.
   * @details Orientation of children relative to local coordinate system,
   * usually read from (or written to) remote, networked EspduTransform nodes.
   */
  void setRotation(const SFRotation &value) { _rotation = value; }

  void setRotation(SFRotation &&value) { _rotation = std::move(value); }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Whether RTP headers are prepended to DIS PDUs.
   */
  const SFBool &getRtpHeaderExpected() const { return _rtpHeaderExpected; }

  /**
   * Access: inputOutput.
   * @details Whether RTP headers are prepended to DIS PDUs.
   */
  void setRtpHeaderExpected(const SFBool &value) { _rtpHeaderExpected = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Non-uniform x-y-z scale of child coordinate system, adjusted by
   * center and scaleOrientation.
   */
  const SFVec3f &getScale() const { return _scale; }

  /**
   * Access: inputOutput.
   * @details Non-uniform x-y-z scale of child coordinate system, adjusted by
   * center and scaleOrientation.
   */
  void setScale(const SFVec3f &value) { _scale = value; }

  void setScale(SFVec3f &&value) { _scale = std::move(value); }

  /**
   * SFRotation; access: inputOutput. Borrows the stored value.
   * @details Preliminary rotation of coordinate system before scaling (to allow
   * scaling around arbitrary orientations).
   */
  const SFRotation &getScaleOrientation() const { return _scaleOrientation; }

  /**
   * Access: inputOutput.
   * @details Preliminary rotation of coordinate system before scaling (to allow
   * scaling around arbitrary orientations).
   */
  void setScaleOrientation(const SFRotation &value) {

    _scaleOrientation = value;
  }

  void setScaleOrientation(SFRotation &&value) {

    _scaleOrientation = std::move(value);
  }

  /**
   * SFFloat; access: inputOnly.
   * @details Set element of user-defined payload array.
   *          Dispatches to the handler registered via
   * setOnSet_articulationParameterValue0Handler(); a no-op if none is set.
   */
  void onSet_articulationParameterValue0(const SFFloat &value) {
    if (_onSet_articulationParameterValue0Handler)
      _onSet_articulationParameterValue0Handler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_articulationParameterValue0Handler(
      std::function<void(const SFFloat &)> handler) {
    _onSet_articulationParameterValue0Handler = std::move(handler);
  }

  /**
   * SFFloat; access: inputOnly.
   * @details Set element of user-defined payload array.
   *          Dispatches to the handler registered via
   * setOnSet_articulationParameterValue1Handler(); a no-op if none is set.
   */
  void onSet_articulationParameterValue1(const SFFloat &value) {
    if (_onSet_articulationParameterValue1Handler)
      _onSet_articulationParameterValue1Handler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_articulationParameterValue1Handler(
      std::function<void(const SFFloat &)> handler) {
    _onSet_articulationParameterValue1Handler = std::move(handler);
  }

  /**
   * SFFloat; access: inputOnly.
   * @details Set element of user-defined payload array.
   *          Dispatches to the handler registered via
   * setOnSet_articulationParameterValue2Handler(); a no-op if none is set.
   */
  void onSet_articulationParameterValue2(const SFFloat &value) {
    if (_onSet_articulationParameterValue2Handler)
      _onSet_articulationParameterValue2Handler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_articulationParameterValue2Handler(
      std::function<void(const SFFloat &)> handler) {
    _onSet_articulationParameterValue2Handler = std::move(handler);
  }

  /**
   * SFFloat; access: inputOnly.
   * @details Set element of user-defined payload array.
   *          Dispatches to the handler registered via
   * setOnSet_articulationParameterValue3Handler(); a no-op if none is set.
   */
  void onSet_articulationParameterValue3(const SFFloat &value) {
    if (_onSet_articulationParameterValue3Handler)
      _onSet_articulationParameterValue3Handler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_articulationParameterValue3Handler(
      std::function<void(const SFFloat &)> handler) {
    _onSet_articulationParameterValue3Handler = std::move(handler);
  }

  /**
   * SFFloat; access: inputOnly.
   * @details Set element of user-defined payload array.
   *          Dispatches to the handler registered via
   * setOnSet_articulationParameterValue4Handler(); a no-op if none is set.
   */
  void onSet_articulationParameterValue4(const SFFloat &value) {
    if (_onSet_articulationParameterValue4Handler)
      _onSet_articulationParameterValue4Handler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_articulationParameterValue4Handler(
      std::function<void(const SFFloat &)> handler) {
    _onSet_articulationParameterValue4Handler = std::move(handler);
  }

  /**
   * SFFloat; access: inputOnly.
   * @details Set element of user-defined payload array.
   *          Dispatches to the handler registered via
   * setOnSet_articulationParameterValue5Handler(); a no-op if none is set.
   */
  void onSet_articulationParameterValue5(const SFFloat &value) {
    if (_onSet_articulationParameterValue5Handler)
      _onSet_articulationParameterValue5Handler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_articulationParameterValue5Handler(
      std::function<void(const SFFloat &)> handler) {
    _onSet_articulationParameterValue5Handler = std::move(handler);
  }

  /**
   * SFFloat; access: inputOnly.
   * @details Set element of user-defined payload array.
   *          Dispatches to the handler registered via
   * setOnSet_articulationParameterValue6Handler(); a no-op if none is set.
   */
  void onSet_articulationParameterValue6(const SFFloat &value) {
    if (_onSet_articulationParameterValue6Handler)
      _onSet_articulationParameterValue6Handler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_articulationParameterValue6Handler(
      std::function<void(const SFFloat &)> handler) {
    _onSet_articulationParameterValue6Handler = std::move(handler);
  }

  /**
   * SFFloat; access: inputOnly.
   * @details Set element of user-defined payload array.
   *          Dispatches to the handler registered via
   * setOnSet_articulationParameterValue7Handler(); a no-op if none is set.
   */
  void onSet_articulationParameterValue7(const SFFloat &value) {
    if (_onSet_articulationParameterValue7Handler)
      _onSet_articulationParameterValue7Handler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_articulationParameterValue7Handler(
      std::function<void(const SFFloat &)> handler) {
    _onSet_articulationParameterValue7Handler = std::move(handler);
  }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Simulation/exercise siteID of the participating LAN or
   * organization.
   */
  const SFInt32 &getSiteID() const { return _siteID; }

  /**
   * Access: inputOutput.
   * @details Simulation/exercise siteID of the participating LAN or
   * organization.
   */
  void setSiteID(const SFInt32 &value) { _siteID = value; }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details DIS timestamp received from latest PDU update, converted to X3D
   * SFTime units.
   */
  const SFTime &getTimestamp() const { return _timestamp; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details DIS timestamp received from latest PDU update, converted to X3D
   * SFTime units.
   */
  void emitTimestamp(const SFTime &value) { _timestamp = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Position of children relative to local coordinate system, usually
   * read from (or written to) remote, networked EspduTransform nodes.
   */
  const SFVec3f &getTranslation() const { return _translation; }

  /**
   * Access: inputOutput.
   * @details Position of children relative to local coordinate system, usually
   * read from (or written to) remote, networked EspduTransform nodes.
   */
  void setTranslation(const SFVec3f &value) { _translation = value; }

  void setTranslation(SFVec3f &&value) { _translation = std::move(value); }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Integer enumerations value for type of warhead on the munition.
   */
  const SFInt32 &getWarhead() const { return _warhead; }

  /**
   * Access: inputOutput.
   * @details Integer enumerations value for type of warhead on the munition.
   */
  void setWarhead(const SFInt32 &value) { _warhead = value; }

  /**
   * SFTime; access: inputOutput. Borrows the stored value.
   * @details Seconds between write updates, 0 means no writing (sending).
   */
  const SFTime &getWriteInterval() const { return _writeInterval; }

  /**
   * Access: inputOutput.
   * @details Seconds between write updates, 0 means no writing (sending).
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
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setWriteInterval() for range validation.
   */
  void setWriteIntervalUnchecked(const SFTime &value) {
    _writeInterval = value;
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

  SFString _address{"localhost"};

  SFInt32 _applicationID{0};

  MFFloat _articulationParameterArray{};

  MFInt32 _articulationParameterChangeIndicatorArray{};

  SFInt32 _articulationParameterCount{0};

  MFInt32 _articulationParameterDesignatorArray{};

  MFInt32 _articulationParameterIdPartAttachedToArray{};

  MFInt32 _articulationParameterTypeArray{};

  SFFloat _articulationParameterValue0_changed{};

  SFFloat _articulationParameterValue1_changed{};

  SFFloat _articulationParameterValue2_changed{};

  SFFloat _articulationParameterValue3_changed{};

  SFFloat _articulationParameterValue4_changed{};

  SFFloat _articulationParameterValue5_changed{};

  SFFloat _articulationParameterValue6_changed{};

  SFFloat _articulationParameterValue7_changed{};

  SFVec3f _center{SFVec3f{0, 0, 0}};

  SFTime _collideTime{};

  SFInt32 _collisionType{0};

  SFInt32 _deadReckoning{0};

  SFString _description{};

  SFTime _detonateTime{};

  SFVec3f _detonationLocation{SFVec3f{0, 0, 0}};

  SFVec3f _detonationRelativeLocation{SFVec3f{0, 0, 0}};

  SFInt32 _detonationResult{0};

  SFBool _enabled{true};

  SFInt32 _entityCategory{0};

  SFInt32 _entityCountry{0};

  SFInt32 _entityDomain{0};

  SFInt32 _entityExtra{0};

  SFInt32 _entityID{0};

  SFInt32 _entityKind{0};

  SFInt32 _entitySpecific{0};

  SFInt32 _entitySubcategory{0};

  SFInt32 _eventApplicationID{0};

  SFInt32 _eventEntityID{0};

  SFInt32 _eventNumber{0};

  SFInt32 _eventSiteID{0};

  SFBool _fired1{false};

  SFBool _fired2{false};

  SFTime _firedTime{};

  SFInt32 _fireMissionIndex{0};

  SFFloat _firingRange{0};

  SFInt32 _firingRate{0};

  SFInt32 _forceID{0};

  SFInt32 _fuse{0};

  SFVec3d _geoCoords{SFVec3d{0, 0, 0}};

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};

  SFBool _isActive{};

  SFBool _isCollided{};

  SFBool _isDetonated{};

  SFBool _isNetworkReader{};

  SFBool _isNetworkWriter{};

  SFBool _isRtpHeaderHeard{};

  SFBool _isStandAlone{};

  SFVec3f _linearAcceleration{SFVec3f{0, 0, 0}};

  SFVec3f _linearVelocity{SFVec3f{0, 0, 0}};

  SFString _marking{};

  SFString _multicastRelayHost{};

  SFInt32 _multicastRelayPort{0};

  SFInt32 _munitionApplicationID{0};

  SFVec3f _munitionEndPoint{SFVec3f{0, 0, 0}};

  SFInt32 _munitionEntityID{0};

  SFInt32 _munitionQuantity{0};

  SFInt32 _munitionSiteID{0};

  SFVec3f _munitionStartPoint{SFVec3f{0, 0, 0}};

  NetworkModeChoices _networkMode{NetworkModeChoices::STANDALONE};

  SFInt32 _port{0};

  SFTime _readInterval{0.1};

  SFRotation _rotation{SFRotation{0, 0, 1, 0}};

  SFBool _rtpHeaderExpected{false};

  SFVec3f _scale{SFVec3f{1, 1, 1}};

  SFRotation _scaleOrientation{SFRotation{0, 0, 1, 0}};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFFloat &)>
      _onSet_articulationParameterValue0Handler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFFloat &)>
      _onSet_articulationParameterValue1Handler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFFloat &)>
      _onSet_articulationParameterValue2Handler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFFloat &)>
      _onSet_articulationParameterValue3Handler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFFloat &)>
      _onSet_articulationParameterValue4Handler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFFloat &)>
      _onSet_articulationParameterValue5Handler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFFloat &)>
      _onSet_articulationParameterValue6Handler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFFloat &)>
      _onSet_articulationParameterValue7Handler{};

  SFInt32 _siteID{0};

  SFTime _timestamp{};

  SFVec3f _translation{SFVec3f{0, 0, 0}};

  SFInt32 _warhead{0};

  SFTime _writeInterval{1.0};
};

} // namespace x3d::nodes
