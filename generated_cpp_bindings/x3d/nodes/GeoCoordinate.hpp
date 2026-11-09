#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DGeometricPropertyNode.hpp"

#include "x3d/nodes/X3DCoordinateNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class GeoCoordinate
 * @brief GeoCoordinate builds geometry as a set of geographic 3D coordinates.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geospatial.html#GeoCoordinate
 */
class GeoCoordinate : public virtual X3DCoordinateNode {
public:
  GeoCoordinate() = default;

  ~GeoCoordinate() = default;

  static SFNode getDefaultGeoOrigin() { return nullptr; }

  static MFString getDefaultGeoSystem() {
    return std::vector<std::string>{"GD", "WE"};
  }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "coord"; }

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
   * MFVec3d; access: inputOutput. Borrows the stored value.
   * @details point contains a set of actual 3D geographic coordinates, provided
   * in geoSystem format can split strings if desired: "x1 y1 z1 x2 y2 z2" or
   * "x1 y1 z1", "x2 y2 z2".
   */
  const MFVec3d &getPoint() const { return _point; }

  /**
   * Access: inputOutput.
   * @details point contains a set of actual 3D geographic coordinates, provided
   * in geoSystem format can split strings if desired: "x1 y1 z1 x2 y2 z2" or
   * "x1 y1 z1", "x2 y2 z2".
   */
  void setPoint(const MFVec3d &value) { _point = value; }

  void setPoint(MFVec3d &&value) { _point = std::move(value); }

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

  MFVec3d _point{};
};

} // namespace x3d::nodes
