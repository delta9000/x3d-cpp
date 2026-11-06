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

#include "x3d/nodes/X3DBindableNode.hpp"

#include "x3d/nodes/X3DViewpointNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class GeoViewpoint
 * @brief GeoViewpoint specifies viewpoints using geographic coordinates.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geospatial.html#GeoViewpoint
 */
class GeoViewpoint : public virtual X3DViewpointNode {
public:
  GeoViewpoint() = default;

  ~GeoViewpoint() = default;

  static SFVec3d getDefaultCenterOfRotation() { return SFVec3d{0, 0, 0}; }

  static SFFloat getDefaultFieldOfView() { return 0.7854; }

  static SFNode getDefaultGeoOrigin() { return nullptr; }

  static MFString getDefaultGeoSystem() {
    return std::vector<std::string>{"GD", "WE"};
  }

  static SFVec3d getDefaultPosition() { return SFVec3d{0, 0, 100000}; }

  static SFFloat getDefaultSpeedFactor() { return 1.0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Geospatial"; }

  static int componentLevel() { return 1; }

  /**
   * SFVec3d; access: inputOutput. Borrows the stored value.
   * @details centerOfRotation specifies center point about which to rotate
   * user's eyepoint when in EXAMINE or LOOKAT mode.
   */
  const SFVec3d &getCenterOfRotation() const { return _centerOfRotation; }

  /**
   * Access: inputOutput.
   * @details centerOfRotation specifies center point about which to rotate
   * user's eyepoint when in EXAMINE or LOOKAT mode.
   */
  void setCenterOfRotation(const SFVec3d &value) { _centerOfRotation = value; }

  void setCenterOfRotation(SFVec3d &&value) {

    _centerOfRotation = std::move(value);
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Preferred minimum viewing angle from this viewpoint in radians,
   * providing minimum height or minimum width (whichever is smaller).
   */
  const SFFloat &getFieldOfView() const { return _fieldOfView; }

  /**
   * Access: inputOutput.
   * @details Preferred minimum viewing angle from this viewpoint in radians,
   * providing minimum height or minimum width (whichever is smaller).
   */
  void setFieldOfView(const SFFloat &value) { _fieldOfView = value; }

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
   * SFVec3d; access: inputOutput. Borrows the stored value.
   * @details position relative to local georeferenced coordinate system, in
   * proper format.
   */
  const SFVec3d &getPosition() const { return _position; }

  /**
   * Access: inputOutput.
   * @details position relative to local georeferenced coordinate system, in
   * proper format.
   */
  void setPosition(const SFVec3d &value) { _position = value; }

  void setPosition(SFVec3d &&value) { _position = std::move(value); }

  /**
   * SFFloat; access: initializeOnly. Borrows the stored value.
   * @details speedFactor is a multiplier to modify the original elevation-based
   * speed that is set automatically by the browser.
   */
  const SFFloat &getSpeedFactor() const { return _speedFactor; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setSpeedFactorUnchecked(const SFFloat &value) { _speedFactor = value; }

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
  static void checkRangesSpeedFactor(const SFFloat &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

private:
  SFVec3d _centerOfRotation{SFVec3d{0, 0, 0}};

  SFFloat _fieldOfView{0.7854};

  SFNode _geoOrigin{nullptr};

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};

  SFVec3d _position{SFVec3d{0, 0, 100000}};

  SFFloat _speedFactor{1.0};
};

} // namespace x3d::nodes
