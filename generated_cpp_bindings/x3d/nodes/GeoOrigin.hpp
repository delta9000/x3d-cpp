#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class GeoOrigin
 * @brief GeoOrigin is deprecated and discouraged (but nevertheless allowed) in
 * X3D version 3.3. GeoOrigin is restored in X3D version 4.0 for special use on
 * devices with limited floating-point resolution.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geospatial.html#GeoOrigin
 */
class GeoOrigin : public virtual X3DNode {
public:
  GeoOrigin() = default;

  ~GeoOrigin() = default;

  static SFVec3d getDefaultGeoCoords() { return SFVec3d{0, 0, 0}; }

  static MFString getDefaultGeoSystem() {
    return std::vector<std::string>{"GD", "WE"};
  }

  static SFBool getDefaultRotateYUp() { return false; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geoOrigin"; }

  static std::string componentName() { return "Geospatial"; }

  static int componentLevel() { return 1; }

  /**
   * SFVec3d; access: inputOutput. Borrows the stored value.
   * @details Defines absolute geographic location (and implicit local
   * coordinate frame).
   */
  const SFVec3d &getGeoCoords() const { return _geoCoords; }

  /**
   * Access: inputOutput.
   * @details Defines absolute geographic location (and implicit local
   * coordinate frame).
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
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details Whether to rotate coordinates of nodes using this GeoOrigin so
   * that local-up direction aligns with VRML Y axis rotateYUp false means local
   * up-direction is relative to planet surface rotateYUp true allows proper
   * operation of NavigationInfo modes FLY, WALK.
   */
  const SFBool &getRotateYUp() const { return _rotateYUp; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setRotateYUpUnchecked(const SFBool &value) { _rotateYUp = value; }

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

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};

  SFBool _rotateYUp{false};
};

} // namespace x3d::nodes
