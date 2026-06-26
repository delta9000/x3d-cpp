// GeoTransform.hpp
#ifndef GEOTRANSFORM_HPP
#define GEOTRANSFORM_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DBoundedObject.hpp"

#include "X3DGroupingNode.hpp"

/**
 * @class GeoTransform
 * @brief GeoTransform is a Grouping node that can contain most nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geospatial.html#GeoTransform
 */
class GeoTransform : public virtual X3DGroupingNode {
public:
  /**
   * @brief Default constructor for GeoTransform
   */
  GeoTransform() = default;

  /**
   * @brief Destructor for GeoTransform
   */
  ~GeoTransform() = default;

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
   * @brief Get the default value for rotation
   * @return SFRotation The default value
   */
  static SFRotation getDefaultRotation() { return SFRotation{0, 0, 1, 0}; }

  /**
   * @brief Get the default value for scale
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultScale() { return SFVec3f{1, 1, 1}; }

  /**
   * @brief Get the default value for scaleOrientation
   * @return SFRotation The default value
   */
  static SFRotation getDefaultScaleOrientation() {
    return SFRotation{0, 0, 1, 0};
  }

  /**
   * @brief Get the default value for translation
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultTranslation() { return SFVec3f{0, 0, 0}; }

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
   * @brief Gets the value of geoCenter. AccessType: inputOutput
   * @details Translation offset from origin of local coordinate system, applied
   * prior to rotation or scaling.
   * @return SFVec3d The current value of geoCenter.
   */
  SFVec3d getGeoCenter() const { return _geoCenter; }

  /**
   * @brief Sets the value of geoCenter. AccessType: inputOutput
   * @details Translation offset from origin of local coordinate system, applied
   * prior to rotation or scaling.
   * @param value The new value for geoCenter.
   */
  void setGeoCenter(const SFVec3d &value) { _geoCenter = value; }

  void setGeoCenter(SFVec3d &&value) { _geoCenter = std::move(value); }

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
   * @brief Gets the value of rotation. AccessType: inputOutput
   * @details Orientation (axis, angle in radians) of children relative to local
   * coordinate system.
   * @return SFRotation The current value of rotation.
   */
  SFRotation getRotation() const { return _rotation; }

  /**
   * @brief Sets the value of rotation. AccessType: inputOutput
   * @details Orientation (axis, angle in radians) of children relative to local
   * coordinate system.
   * @param value The new value for rotation.
   */
  void setRotation(const SFRotation &value) { _rotation = value; }

  void setRotation(SFRotation &&value) { _rotation = std::move(value); }

  /**
   * @brief Gets the value of scale. AccessType: inputOutput
   * @details Non-uniform x-y-z scale of child coordinate system, adjusted by
   * center and scaleOrientation.
   * @return SFVec3f The current value of scale.
   */
  SFVec3f getScale() const { return _scale; }

  /**
   * @brief Sets the value of scale. AccessType: inputOutput
   * @details Non-uniform x-y-z scale of child coordinate system, adjusted by
   * center and scaleOrientation.
   * @param value The new value for scale.
   */
  void setScale(const SFVec3f &value) { _scale = value; }

  void setScale(SFVec3f &&value) { _scale = std::move(value); }

  /**
   * @brief Gets the value of scaleOrientation. AccessType: inputOutput
   * @details Preliminary rotation of coordinate sys tem before scaling (to
   * allow scaling around arbitrary orientations).
   * @return SFRotation The current value of scaleOrientation.
   */
  SFRotation getScaleOrientation() const { return _scaleOrientation; }

  /**
   * @brief Sets the value of scaleOrientation. AccessType: inputOutput
   * @details Preliminary rotation of coordinate sys tem before scaling (to
   * allow scaling around arbitrary orientations).
   * @param value The new value for scaleOrientation.
   */
  void setScaleOrientation(const SFRotation &value) {

    _scaleOrientation = value;
  }

  void setScaleOrientation(SFRotation &&value) {

    _scaleOrientation = std::move(value);
  }

  /**
   * @brief Gets the value of translation. AccessType: inputOutput
   * @details Position (x, y, z in meters) of children relative to local
   * coordinate system.
   * @return SFVec3f The current value of translation.
   */
  SFVec3f getTranslation() const { return _translation; }

  /**
   * @brief Sets the value of translation. AccessType: inputOutput
   * @details Position (x, y, z in meters) of children relative to local
   * coordinate system.
   * @param value The new value for translation.
   */
  void setTranslation(const SFVec3f &value) { _translation = value; }

  void setTranslation(SFVec3f &&value) { _translation = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "GeoTransform").
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
   * @brief Member variable for geoCenter.
   */

  SFVec3d _geoCenter{SFVec3d{0, 0, 0}};

  /**
   * @brief Member variable for geoOrigin.
   */

  SFNode _geoOrigin{nullptr};

  /**
   * @brief Member variable for geoSystem.
   */

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};

  /**
   * @brief Member variable for rotation.
   */

  SFRotation _rotation{SFRotation{0, 0, 1, 0}};

  /**
   * @brief Member variable for scale.
   */

  SFVec3f _scale{SFVec3f{1, 1, 1}};

  /**
   * @brief Member variable for scaleOrientation.
   */

  SFRotation _scaleOrientation{SFRotation{0, 0, 1, 0}};

  /**
   * @brief Member variable for translation.
   */

  SFVec3f _translation{SFVec3f{0, 0, 0}};
};

#endif // GEOTRANSFORM_HPP