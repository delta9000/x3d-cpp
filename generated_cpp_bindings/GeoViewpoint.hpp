// GeoViewpoint.hpp
#ifndef GEOVIEWPOINT_HPP
#define GEOVIEWPOINT_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DBindableNode.hpp"

#include "X3DViewpointNode.hpp"

/**
 * @class GeoViewpoint
 * @brief GeoViewpoint specifies viewpoints using geographic coordinates.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geospatial.html#GeoViewpoint
 */
class GeoViewpoint : public virtual X3DViewpointNode {
public:
  /**
   * @brief Default constructor for GeoViewpoint
   */
  GeoViewpoint() = default;

  /**
   * @brief Destructor for GeoViewpoint
   */
  ~GeoViewpoint() = default;

  /**
   * @brief Get the default value for centerOfRotation
   * @return SFVec3d The default value
   */
  static SFVec3d getDefaultCenterOfRotation() { return SFVec3d{0, 0, 0}; }

  /**
   * @brief Get the default value for fieldOfView
   * @return SFFloat The default value
   */
  static SFFloat getDefaultFieldOfView() { return 0.7854; }

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
   * @brief Get the default value for position
   * @return SFVec3d The default value
   */
  static SFVec3d getDefaultPosition() { return SFVec3d{0, 0, 100000}; }

  /**
   * @brief Get the default value for speedFactor
   * @return SFFloat The default value
   */
  static SFFloat getDefaultSpeedFactor() { return 1.0; }

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
   * @brief Gets the value of centerOfRotation. AccessType: inputOutput
   * @details centerOfRotation specifies center point about which to rotate
   * user's eyepoint when in EXAMINE or LOOKAT mode.
   * @return SFVec3d The current value of centerOfRotation.
   */
  SFVec3d getCenterOfRotation() const { return _centerOfRotation; }

  /**
   * @brief Sets the value of centerOfRotation. AccessType: inputOutput
   * @details centerOfRotation specifies center point about which to rotate
   * user's eyepoint when in EXAMINE or LOOKAT mode.
   * @param value The new value for centerOfRotation.
   */
  void setCenterOfRotation(const SFVec3d &value) { _centerOfRotation = value; }

  void setCenterOfRotation(SFVec3d &&value) {

    _centerOfRotation = std::move(value);
  }

  /**
   * @brief Gets the value of fieldOfView. AccessType: inputOutput
   * @details Preferred minimum viewing angle from this viewpoint in radians,
   * providing minimum height or minimum width (whichever is smaller).
   * @return SFFloat The current value of fieldOfView.
   */
  SFFloat getFieldOfView() const { return _fieldOfView; }

  /**
   * @brief Sets the value of fieldOfView. AccessType: inputOutput
   * @details Preferred minimum viewing angle from this viewpoint in radians,
   * providing minimum height or minimum width (whichever is smaller).
   * @param value The new value for fieldOfView.
   */
  void setFieldOfView(const SFFloat &value) { _fieldOfView = value; }

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
   * @brief Gets the value of position. AccessType: inputOutput
   * @details position relative to local georeferenced coordinate system, in
   * proper format.
   * @return SFVec3d The current value of position.
   */
  SFVec3d getPosition() const { return _position; }

  /**
   * @brief Sets the value of position. AccessType: inputOutput
   * @details position relative to local georeferenced coordinate system, in
   * proper format.
   * @param value The new value for position.
   */
  void setPosition(const SFVec3d &value) { _position = value; }

  void setPosition(SFVec3d &&value) { _position = std::move(value); }

  /**
   * @brief Gets the value of speedFactor. AccessType: initializeOnly
   * @details speedFactor is a multiplier to modify the original elevation-based
   * speed that is set automatically by the browser.
   * @return SFFloat The current value of speedFactor.
   */
  SFFloat getSpeedFactor() const { return _speedFactor; }
  /**
   * @brief Data-layer write of speedFactor (reader/init ingest path).
   * @details speedFactor is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setSpeedFactor().
   */
  void setSpeedFactorUnchecked(const SFFloat &value) { _speedFactor = value; }

  /**
   * @brief The X3D type name of this node (e.g. "GeoViewpoint").
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
   * @brief Member variable for centerOfRotation.
   */

  SFVec3d _centerOfRotation{SFVec3d{0, 0, 0}};

  /**
   * @brief Member variable for fieldOfView.
   */

  SFFloat _fieldOfView{0.7854};

  /**
   * @brief Member variable for geoOrigin.
   */

  SFNode _geoOrigin{nullptr};

  /**
   * @brief Member variable for geoSystem.
   */

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};

  /**
   * @brief Member variable for position.
   */

  SFVec3d _position{SFVec3d{0, 0, 100000}};

  /**
   * @brief Member variable for speedFactor.
   */

  SFFloat _speedFactor{1.0};
};

#endif // GEOVIEWPOINT_HPP