// GeoProximitySensor.hpp
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

#include "x3d/nodes/X3DSensorNode.hpp"

#include "x3d/nodes/X3DEnvironmentalSensorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class GeoProximitySensor
 * @brief GeoProximitySensor generates events when the viewer enters, exits and
 * moves within a region of space (defined by a box).
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geospatial.html#GeoProximitySensor
 */
class GeoProximitySensor : public virtual X3DEnvironmentalSensorNode {
public:
  /**
   * @brief Default constructor for GeoProximitySensor
   */
  GeoProximitySensor() = default;

  /**
   * @brief Destructor for GeoProximitySensor
   */
  ~GeoProximitySensor() = default;

  /**
   * @brief Get the default value for center
   * @return SFVec3d The default value
   */
  static SFVec3d getDefaultCenter() { return SFVec3d{0, 0, 0}; }

  /**
   * @brief Get the default value for geoCenter
   * @return SFVec3d The default value
   */
  static SFVec3d getDefaultGeoCenter() { return SFVec3d{0, 0, 0}; }

  /**
   * @brief Get the default value for geoOrigin
   * @return SFNode The default value
   */
  static SFNode getDefaultGeoOrigin() { return nullptr; }

  /**
   * @brief Get the default value for geoSystem
   * @return MFString The default value
   */
  static MFString getDefaultGeoSystem() {
    return std::vector<std::string>{"GD", "WE"};
  }

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
  static std::string componentName() { return "Geospatial"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of center. AccessType: inputOutput
   * @details Position offset from origin of local coordinate system.
   * @return SFVec3d The current value of center.
   */
  SFVec3d getCenter() const { return _center; }

  /**
   * @brief Sets the value of center. AccessType: inputOutput
   * @details Position offset from origin of local coordinate system.
   * @param value The new value for center.
   */
  void setCenter(const SFVec3d &value) { _center = value; }

  void setCenter(SFVec3d &&value) { _center = std::move(value); }

  /**
   * @brief Gets the value of centerOfRotation_changed. AccessType: outputOnly
   * @details Sends changed centerOfRotation values, likely caused by user
   * interaction.
   * @return SFVec3f The current value of centerOfRotation_changed.
   */
  SFVec3f getCenterOfRotation_changed() const {
    return _centerOfRotation_changed;
  }

  /**
   * @brief Emit an output value on centerOfRotation_changed. AccessType:
   * outputOnly
   * @details Sends changed centerOfRotation values, likely caused by user
   * interaction. outputOnly fields have no author-facing setter; a node's
   * behavior or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitCenterOfRotation_changed(const SFVec3f &value) {
    _centerOfRotation_changed = value;
  }

  /**
   * @brief Gets the value of enterTime. AccessType: outputOnly
   * @details Time event generated when user's camera enters the box.
   * @return SFTime The current value of enterTime.
   */
  SFTime getEnterTime() const { return _enterTime; }

  /**
   * @brief Emit an output value on enterTime. AccessType: outputOnly
   * @details Time event generated when user's camera enters the box.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitEnterTime(const SFTime &value) { _enterTime = value; }

  /**
   * @brief Gets the value of exitTime. AccessType: outputOnly
   * @details Time event generated when user's camera exits the box.
   * @return SFTime The current value of exitTime.
   */
  SFTime getExitTime() const { return _exitTime; }

  /**
   * @brief Emit an output value on exitTime. AccessType: outputOnly
   * @details Time event generated when user's camera exits the box.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitExitTime(const SFTime &value) { _exitTime = value; }

  /**
   * @brief Gets the value of geoCenter. AccessType: inputOutput
   * @details Position offset from origin of local coordinate system.
   * @return SFVec3d The current value of geoCenter.
   */
  SFVec3d getGeoCenter() const { return _geoCenter; }

  /**
   * @brief Sets the value of geoCenter. AccessType: inputOutput
   * @details Position offset from origin of local coordinate system.
   * @param value The new value for geoCenter.
   */
  void setGeoCenter(const SFVec3d &value) { _geoCenter = value; }

  void setGeoCenter(SFVec3d &&value) { _geoCenter = std::move(value); }

  /**
   * @brief Gets the value of geoCoord_changed. AccessType: outputOnly
   * @details Sends geospatial coordinates of viewer's position corresponding to
   * world position returned by position_changed.
   * @return SFVec3d The current value of geoCoord_changed.
   */
  SFVec3d getGeoCoord_changed() const { return _geoCoord_changed; }

  /**
   * @brief Emit an output value on geoCoord_changed. AccessType: outputOnly
   * @details Sends geospatial coordinates of viewer's position corresponding to
   * world position returned by position_changed. outputOnly fields have no
   * author-facing setter; a node's behavior or the runtime calls this to
   * produce an output event. The event cascade reaches it through the reflected
   * field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitGeoCoord_changed(const SFVec3d &value) { _geoCoord_changed = value; }

  /**
   * @brief Gets the value of geoOrigin. AccessType: initializeOnly
   * @details Single contained GeoOrigin node that can specify a local
   * coordinate frame for extended precision.
   * @return SFNode The current value of geoOrigin.
   */
  SFNode getGeoOrigin() const { return _geoOrigin; }

  /**
   * @brief Acceptable node types for the geoOrigin field.
   * @details Permitted X3D node types: GeoOrigin
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableGeoOriginNodeTypes() {
    static const std::vector<std::string> types = {"GeoOrigin"};
    return types;
  }
  /**
   * @brief Data-layer write of geoOrigin (reader/init ingest path).
   * @details geoOrigin is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setGeoOrigin().
   */
  void setGeoOriginUnchecked(const SFNode &value) { _geoOrigin = value; }

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
   * @brief Gets the value of orientation_changed. AccessType: outputOnly
   * @details Sends rotation event relative to center.
   * @return SFRotation The current value of orientation_changed.
   */
  SFRotation getOrientation_changed() const { return _orientation_changed; }

  /**
   * @brief Emit an output value on orientation_changed. AccessType: outputOnly
   * @details Sends rotation event relative to center.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitOrientation_changed(const SFRotation &value) {
    _orientation_changed = value;
  }

  /**
   * @brief Gets the value of position_changed. AccessType: outputOnly
   * @details Sends translation event relative to center.
   * @return SFVec3f The current value of position_changed.
   */
  SFVec3f getPosition_changed() const { return _position_changed; }

  /**
   * @brief Emit an output value on position_changed. AccessType: outputOnly
   * @details Sends translation event relative to center.
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitPosition_changed(const SFVec3f &value) { _position_changed = value; }

  /**
   * @brief The X3D type name of this node (e.g. "GeoProximitySensor").
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

private:
  /**
   * @brief Member variable for center.
   */

  SFVec3d _center{SFVec3d{0, 0, 0}};

  /**
   * @brief Member variable for centerOfRotation_changed.
   */

  SFVec3f _centerOfRotation_changed{};

  /**
   * @brief Member variable for enterTime.
   */

  SFTime _enterTime{};

  /**
   * @brief Member variable for exitTime.
   */

  SFTime _exitTime{};

  /**
   * @brief Member variable for geoCenter.
   */

  SFVec3d _geoCenter{SFVec3d{0, 0, 0}};

  /**
   * @brief Member variable for geoCoord_changed.
   */

  SFVec3d _geoCoord_changed{};

  /**
   * @brief Member variable for geoOrigin.
   */

  SFNode _geoOrigin{nullptr};

  /**
   * @brief Member variable for geoSystem.
   */

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};

  /**
   * @brief Member variable for orientation_changed.
   */

  SFRotation _orientation_changed{};

  /**
   * @brief Member variable for position_changed.
   */

  SFVec3f _position_changed{};
};

} // namespace x3d::nodes
