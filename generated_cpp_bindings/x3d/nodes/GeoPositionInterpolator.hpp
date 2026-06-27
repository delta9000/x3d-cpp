// GeoPositionInterpolator.hpp
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

#include "x3d/nodes/X3DInterpolatorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class GeoPositionInterpolator
 * @brief GeoPositionInterpolator animates objects within a geographic
 * coordinate system.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geospatial.html#GeoPositionInterpolator
 */
class GeoPositionInterpolator : public virtual X3DInterpolatorNode {
public:
  /**
   * @brief Default constructor for GeoPositionInterpolator
   */
  GeoPositionInterpolator() = default;

  /**
   * @brief Destructor for GeoPositionInterpolator
   */
  ~GeoPositionInterpolator() = default;

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
   * @brief Gets the value of geovalue_changed. AccessType: outputOnly
   * @details interpolated coordinate in the geographic coordinate system
   * specified by geoSystem Hint: X3D for Advanced Modeling (X3D4AM) slideset
   * https://x3dgraphics.
   * @return SFVec3d The current value of geovalue_changed.
   */
  SFVec3d getGeovalue_changed() const { return _geovalue_changed; }

  /**
   * @brief Emit an output value on geovalue_changed. AccessType: outputOnly
   * @details interpolated coordinate in the geographic coordinate system
   * specified by geoSystem Hint: X3D for Advanced Modeling (X3D4AM) slideset
   * https://x3dgraphics. outputOnly fields have no author-facing setter; a
   * node's behavior or the runtime calls this to produce an output event. The
   * event cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitGeovalue_changed(const SFVec3d &value) { _geovalue_changed = value; }

  /**
   * @brief Gets the value of keyValue. AccessType: inputOutput
   * @details Output values for linear interpolation, each corresponding to an
   * input-fraction value in the key array.
   * @return MFVec3d The current value of keyValue.
   */
  MFVec3d getKeyValue() const { return _keyValue; }

  /**
   * @brief Sets the value of keyValue. AccessType: inputOutput
   * @details Output values for linear interpolation, each corresponding to an
   * input-fraction value in the key array.
   * @param value The new value for keyValue.
   */
  void setKeyValue(const MFVec3d &value) { _keyValue = value; }

  void setKeyValue(MFVec3d &&value) { _keyValue = std::move(value); }

  /**
   * @brief Gets the value of value_changed. AccessType: outputOnly
   * @details Linearly interpolated output value determined by current key time
   * and corresponding keyValue pair.
   * @return SFVec3f The current value of value_changed.
   */
  SFVec3f getValue_changed() const { return _value_changed; }

  /**
   * @brief Emit an output value on value_changed. AccessType: outputOnly
   * @details Linearly interpolated output value determined by current key time
   * and corresponding keyValue pair. outputOnly fields have no author-facing
   * setter; a node's behavior or the runtime calls this to produce an output
   * event. The event cascade reaches it through the reflected field table so
   * producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitValue_changed(const SFVec3f &value) { _value_changed = value; }

  /**
   * @brief The X3D type name of this node (e.g. "GeoPositionInterpolator").
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
   * @brief Member variable for geovalue_changed.
   */

  SFVec3d _geovalue_changed{};

  /**
   * @brief Member variable for keyValue.
   */

  MFVec3d _keyValue{};

  /**
   * @brief Member variable for value_changed.
   */

  SFVec3f _value_changed{};
};

} // namespace x3d::nodes
