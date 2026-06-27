// GeoOrigin.hpp
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
  /**
   * @brief Default constructor for GeoOrigin
   */
  GeoOrigin() = default;

  /**
   * @brief Destructor for GeoOrigin
   */
  ~GeoOrigin() = default;

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
   * @brief Get the default value for rotateYUp
   * @return SFBool The default value
   */
  static SFBool getDefaultRotateYUp() { return false; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "geoOrigin"; }

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
   * @brief Gets the value of geoCoords. AccessType: inputOutput
   * @details Defines absolute geographic location (and implicit local
   * coordinate frame).
   * @return SFVec3d The current value of geoCoords.
   */
  SFVec3d getGeoCoords() const { return _geoCoords; }

  /**
   * @brief Sets the value of geoCoords. AccessType: inputOutput
   * @details Defines absolute geographic location (and implicit local
   * coordinate frame).
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
   * @brief Gets the value of rotateYUp. AccessType: initializeOnly
   * @details Whether to rotate coordinates of nodes using this GeoOrigin so
   * that local-up direction aligns with VRML Y axis rotateYUp false means local
   * up-direction is relative to planet surface rotateYUp true allows proper
   * operation of NavigationInfo modes FLY, WALK.
   * @return SFBool The current value of rotateYUp.
   */
  SFBool getRotateYUp() const { return _rotateYUp; }
  /**
   * @brief Data-layer write of rotateYUp (reader/init ingest path).
   * @details rotateYUp is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setRotateYUp().
   */
  void setRotateYUpUnchecked(const SFBool &value) { _rotateYUp = value; }

  /**
   * @brief The X3D type name of this node (e.g. "GeoOrigin").
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
   * @brief Member variable for geoCoords.
   */

  SFVec3d _geoCoords{SFVec3d{0, 0, 0}};

  /**
   * @brief Member variable for geoSystem.
   */

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};

  /**
   * @brief Member variable for rotateYUp.
   */

  SFBool _rotateYUp{false};
};

} // namespace x3d::nodes
