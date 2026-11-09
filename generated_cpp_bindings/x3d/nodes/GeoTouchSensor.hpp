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

#include "x3d/nodes/X3DPointingDeviceSensorNode.hpp"

#include "x3d/nodes/X3DTouchSensorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class GeoTouchSensor
 * @brief GeoTouchSensor returns geographic coordinates for the object being
 * selected.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geospatial.html#GeoTouchSensor
 */
class GeoTouchSensor : public virtual X3DTouchSensorNode {
public:
  GeoTouchSensor() = default;

  ~GeoTouchSensor() = default;

  static SFNode getDefaultGeoOrigin() { return nullptr; }

  static MFString getDefaultGeoSystem() {
    return std::vector<std::string>{"GD", "WE"};
  }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Geospatial"; }

  static int componentLevel() { return 1; }

  /**
   * SFNode; access: initializeOnly. Borrows the stored value.
   * @details Single contained GeoOrigin node that can specify a local
   * coordinate frame for extended precision.
   */
  const SFNode &getGeoOrigin() const { return _geoOrigin; }

  /**
   * @details Permitted X3D node types: GeoOrigin
   */
  static const std::vector<std::string> &acceptableGeoOriginNodeTypes() {
    static const std::vector<std::string> types = {"GeoOrigin"};
    return types;
  }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setGeoOriginUnchecked(const SFNode &value) { _geoOrigin = value; }

  /**
   * MFString; access: initializeOnly. Borrows the stored value.
   * @details Identifies spatial reference frame: Geodetic (G D), Geocentric
   * (GC), Universal Transverse Mercator (UTM).
   */
  const MFString &getGeoSystem() const { return _geoSystem; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setGeoSystemUnchecked(const MFString &value) { _geoSystem = value; }

  /**
   * SFVec3d; access: outputOnly. Borrows the stored value.
   * @details Output event containing 3D point on surface of underlying
   * geometry, given in GeoTouchSensor's local coordinate system.
   */
  const SFVec3d &getHitGeoCoord_changed() const { return _hitGeoCoord_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Output event containing 3D point on surface of underlying
   * geometry, given in GeoTouchSensor's local coordinate system.
   */
  void emitHitGeoCoord_changed(const SFVec3d &value) {
    _hitGeoCoord_changed = value;
  }

  /**
   * SFVec3f; access: outputOnly. Borrows the stored value.
   * @details Output event containing surface normal vector at the
   * hitGeoCoordinate.
   */
  const SFVec3f &getHitNormal_changed() const { return _hitNormal_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Output event containing surface normal vector at the
   * hitGeoCoordinate.
   */
  void emitHitNormal_changed(const SFVec3f &value) {
    _hitNormal_changed = value;
  }

  /**
   * SFVec3f; access: outputOnly. Borrows the stored value.
   * @details Output event containing 3D point on surface of underlying
   * geometry, given in geometry coordinates (not geographic coordinates).
   */
  const SFVec3f &getHitPoint_changed() const { return _hitPoint_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Output event containing 3D point on surface of underlying
   * geometry, given in geometry coordinates (not geographic coordinates).
   */
  void emitHitPoint_changed(const SFVec3f &value) { _hitPoint_changed = value; }

  /**
   * SFVec2f; access: outputOnly. Borrows the stored value.
   * @details Output event containing texture coordinates of surface at the
   * hitGeoCoordinate.
   */
  const SFVec2f &getHitTexCoord_changed() const { return _hitTexCoord_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Output event containing texture coordinates of surface at the
   * hitGeoCoordinate.
   */
  void emitHitTexCoord_changed(const SFVec2f &value) {
    _hitTexCoord_changed = value;
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

private:
  SFNode _geoOrigin{nullptr};

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};

  SFVec3d _hitGeoCoord_changed{};

  SFVec3f _hitNormal_changed{};

  SFVec3f _hitPoint_changed{};

  SFVec2f _hitTexCoord_changed{};
};

} // namespace x3d::nodes
