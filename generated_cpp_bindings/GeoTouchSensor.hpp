// GeoTouchSensor.hpp
#ifndef GEOTOUCHSENSOR_HPP
#define GEOTOUCHSENSOR_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DSensorNode.hpp"

#include "X3DPointingDeviceSensorNode.hpp"

#include "X3DTouchSensorNode.hpp"

/**
 * @class GeoTouchSensor
 * @brief GeoTouchSensor returns geographic coordinates for the object being
 * selected.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geospatial.html#GeoTouchSensor
 */
class GeoTouchSensor : public virtual X3DTouchSensorNode {
public:
  /**
   * @brief Default constructor for GeoTouchSensor
   */
  GeoTouchSensor() = default;

  /**
   * @brief Destructor for GeoTouchSensor
   */
  ~GeoTouchSensor() = default;

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
  static int componentLevel() { return 1; }

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
   * @details Identifies spatial reference frame: Geodetic (G D), Geocentric
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
   * @brief Gets the value of hitGeoCoord_changed. AccessType: outputOnly
   * @details Output event containing 3D point on surface of underlying
   * geometry, given in GeoTouchSensor's local coordinate system.
   * @return SFVec3d The current value of hitGeoCoord_changed.
   */
  SFVec3d getHitGeoCoord_changed() const { return _hitGeoCoord_changed; }

  /**
   * @brief Emit an output value on hitGeoCoord_changed. AccessType: outputOnly
   * @details Output event containing 3D point on surface of underlying
   * geometry, given in GeoTouchSensor's local coordinate system. outputOnly
   * fields have no author-facing setter; a node's behavior or the runtime calls
   * this to produce an output event. The event cascade reaches it through the
   * reflected field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitHitGeoCoord_changed(const SFVec3d &value) {
    _hitGeoCoord_changed = value;
  }

  /**
   * @brief Gets the value of hitNormal_changed. AccessType: outputOnly
   * @details Output event containing surface normal vector at the
   * hitGeoCoordinate.
   * @return SFVec3f The current value of hitNormal_changed.
   */
  SFVec3f getHitNormal_changed() const { return _hitNormal_changed; }

  /**
   * @brief Emit an output value on hitNormal_changed. AccessType: outputOnly
   * @details Output event containing surface normal vector at the
   * hitGeoCoordinate. outputOnly fields have no author-facing setter; a node's
   * behavior or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitHitNormal_changed(const SFVec3f &value) {
    _hitNormal_changed = value;
  }

  /**
   * @brief Gets the value of hitPoint_changed. AccessType: outputOnly
   * @details Output event containing 3D point on surface of underlying
   * geometry, given in geometry coordinates (not geographic coordinates).
   * @return SFVec3f The current value of hitPoint_changed.
   */
  SFVec3f getHitPoint_changed() const { return _hitPoint_changed; }

  /**
   * @brief Emit an output value on hitPoint_changed. AccessType: outputOnly
   * @details Output event containing 3D point on surface of underlying
   * geometry, given in geometry coordinates (not geographic coordinates).
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitHitPoint_changed(const SFVec3f &value) { _hitPoint_changed = value; }

  /**
   * @brief Gets the value of hitTexCoord_changed. AccessType: outputOnly
   * @details Output event containing texture coordinates of surface at the
   * hitGeoCoordinate.
   * @return SFVec2f The current value of hitTexCoord_changed.
   */
  SFVec2f getHitTexCoord_changed() const { return _hitTexCoord_changed; }

  /**
   * @brief Emit an output value on hitTexCoord_changed. AccessType: outputOnly
   * @details Output event containing texture coordinates of surface at the
   * hitGeoCoordinate. outputOnly fields have no author-facing setter; a node's
   * behavior or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitHitTexCoord_changed(const SFVec2f &value) {
    _hitTexCoord_changed = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "GeoTouchSensor").
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

private:
  /**
   * @brief Member variable for geoOrigin.
   */

  SFNode _geoOrigin{nullptr};

  /**
   * @brief Member variable for geoSystem.
   */

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};

  /**
   * @brief Member variable for hitGeoCoord_changed.
   */

  SFVec3d _hitGeoCoord_changed{};

  /**
   * @brief Member variable for hitNormal_changed.
   */

  SFVec3f _hitNormal_changed{};

  /**
   * @brief Member variable for hitPoint_changed.
   */

  SFVec3f _hitPoint_changed{};

  /**
   * @brief Member variable for hitTexCoord_changed.
   */

  SFVec2f _hitTexCoord_changed{};
};

#endif // GEOTOUCHSENSOR_HPP