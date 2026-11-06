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
 * @class GeoTransform
 * @brief GeoTransform is a Grouping node that can contain most nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geospatial.html#GeoTransform
 */
class GeoTransform : public virtual X3DGroupingNode {
public:
  GeoTransform() = default;

  ~GeoTransform() = default;

  static SFVec3d getDefaultGeoCenter() { return SFVec3d{0, 0, 0}; }

  static SFNode getDefaultGeoOrigin() { return nullptr; }

  static MFString getDefaultGeoSystem() {
    return std::vector<std::string>{"GD", "WE"};
  }

  static SFRotation getDefaultRotation() { return SFRotation{0, 0, 1, 0}; }

  static SFVec3f getDefaultScale() { return SFVec3f{1, 1, 1}; }

  static SFRotation getDefaultScaleOrientation() {
    return SFRotation{0, 0, 1, 0};
  }

  static SFVec3f getDefaultTranslation() { return SFVec3f{0, 0, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Geospatial"; }

  static int componentLevel() { return 2; }

  /**
   * SFVec3d; access: inputOutput. Borrows the stored value.
   * @details Translation offset from origin of local coordinate system, applied
   * prior to rotation or scaling.
   */
  const SFVec3d &getGeoCenter() const { return _geoCenter; }

  /**
   * Access: inputOutput.
   * @details Translation offset from origin of local coordinate system, applied
   * prior to rotation or scaling.
   */
  void setGeoCenter(const SFVec3d &value) { _geoCenter = value; }

  void setGeoCenter(SFVec3d &&value) { _geoCenter = std::move(value); }

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
   * SFRotation; access: inputOutput. Borrows the stored value.
   * @details Orientation (axis, angle in radians) of children relative to local
   * coordinate system.
   */
  const SFRotation &getRotation() const { return _rotation; }

  /**
   * Access: inputOutput.
   * @details Orientation (axis, angle in radians) of children relative to local
   * coordinate system.
   */
  void setRotation(const SFRotation &value) { _rotation = value; }

  void setRotation(SFRotation &&value) { _rotation = std::move(value); }

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
   * @details Preliminary rotation of coordinate sys tem before scaling (to
   * allow scaling around arbitrary orientations).
   */
  const SFRotation &getScaleOrientation() const { return _scaleOrientation; }

  /**
   * Access: inputOutput.
   * @details Preliminary rotation of coordinate sys tem before scaling (to
   * allow scaling around arbitrary orientations).
   */
  void setScaleOrientation(const SFRotation &value) {

    _scaleOrientation = value;
  }

  void setScaleOrientation(SFRotation &&value) {

    _scaleOrientation = std::move(value);
  }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Position (x, y, z in meters) of children relative to local
   * coordinate system.
   */
  const SFVec3f &getTranslation() const { return _translation; }

  /**
   * Access: inputOutput.
   * @details Position (x, y, z in meters) of children relative to local
   * coordinate system.
   */
  void setTranslation(const SFVec3f &value) { _translation = value; }

  void setTranslation(SFVec3f &&value) { _translation = std::move(value); }

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
  SFVec3d _geoCenter{SFVec3d{0, 0, 0}};

  SFNode _geoOrigin{nullptr};

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};

  SFRotation _rotation{SFRotation{0, 0, 1, 0}};

  SFVec3f _scale{SFVec3f{1, 1, 1}};

  SFRotation _scaleOrientation{SFRotation{0, 0, 1, 0}};

  SFVec3f _translation{SFVec3f{0, 0, 0}};
};

} // namespace x3d::nodes
