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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class GeoLocation
 * @brief GeoLocation positions a regular X3D model onto earth's surface.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geospatial.html#GeoLocation
 */
class GeoLocation : public virtual X3DGroupingNode {
public:
  GeoLocation() = default;

  ~GeoLocation() = default;

  static SFVec3d getDefaultGeoCoords() { return SFVec3d{0, 0, 0}; }

  static SFNode getDefaultGeoOrigin() { return nullptr; }

  static MFString getDefaultGeoSystem() {
    return std::vector<std::string>{"GD", "WE"};
  }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Geospatial"; }

  static int componentLevel() { return 1; }

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
  SFVec3d _geoCoords{SFVec3d{0, 0, 0}};

  SFNode _geoOrigin{nullptr};

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};
};

} // namespace x3d::nodes
