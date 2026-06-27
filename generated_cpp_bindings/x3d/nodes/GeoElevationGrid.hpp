// GeoElevationGrid.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DGeometryNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class GeoElevationGrid
 * @brief GeoElevationGrid is a geometry node defining a rectangular height
 * field, with default values for a 1m by 1m square at height 0.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geospatial.html#GeoElevationGrid
 */
class GeoElevationGrid : public virtual X3DGeometryNode {
public:
  /**
   * @brief Default constructor for GeoElevationGrid
   */
  GeoElevationGrid() = default;

  /**
   * @brief Destructor for GeoElevationGrid
   */
  ~GeoElevationGrid() = default;

  /**
   * @brief Get the default value for ccw
   * @return SFBool The default value
   */
  static SFBool getDefaultCcw() { return true; }

  /**
   * @brief Get the default value for color
   * @return SFNode The default value
   */
  static SFNode getDefaultColor() { return nullptr; }

  /**
   * @brief Get the default value for colorPerVertex
   * @return SFBool The default value
   */
  static SFBool getDefaultColorPerVertex() { return true; }

  /**
   * @brief Get the default value for creaseAngle
   * @return SFDouble The default value
   */
  static SFDouble getDefaultCreaseAngle() { return 0; }

  /**
   * @brief Get the default value for geoGridOrigin
   * @return SFVec3d The default value
   */
  static SFVec3d getDefaultGeoGridOrigin() { return SFVec3d{0, 0, 0}; }

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
   * @brief Get the default value for height
   * @return MFDouble The default value
   */
  static MFDouble getDefaultHeight() { return std::vector<double>{0, 0}; }

  /**
   * @brief Get the default value for normal
   * @return SFNode The default value
   */
  static SFNode getDefaultNormal() { return nullptr; }

  /**
   * @brief Get the default value for normalPerVertex
   * @return SFBool The default value
   */
  static SFBool getDefaultNormalPerVertex() { return true; }

  /**
   * @brief Get the default value for solid
   * @return SFBool The default value
   */
  static SFBool getDefaultSolid() { return true; }

  /**
   * @brief Get the default value for texCoord
   * @return SFNode The default value
   */
  static SFNode getDefaultTexCoord() { return nullptr; }

  /**
   * @brief Get the default value for xDimension
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultXDimension() { return 0; }

  /**
   * @brief Get the default value for xSpacing
   * @return SFDouble The default value
   */
  static SFDouble getDefaultXSpacing() { return 1.0; }

  /**
   * @brief Get the default value for yScale
   * @return SFFloat The default value
   */
  static SFFloat getDefaultYScale() { return 1; }

  /**
   * @brief Get the default value for zDimension
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultZDimension() { return 0; }

  /**
   * @brief Get the default value for zSpacing
   * @return SFDouble The default value
   */
  static SFDouble getDefaultZSpacing() { return 1.0; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "geometry"; }

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
   * @brief Gets the value of ccw. AccessType: initializeOnly
   * @details ccw defines clockwise/counterclockwise ordering of vertex
   * coordinates, which in turn defines front/back orientation of polygon
   * normals according to Right-Hand Rule (RHR).
   * @return SFBool The current value of ccw.
   */
  SFBool getCcw() const { return _ccw; }
  /**
   * @brief Data-layer write of ccw (reader/init ingest path).
   * @details ccw is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setCcw().
   */
  void setCcwUnchecked(const SFBool &value) { _ccw = value; }

  /**
   * @brief Gets the value of color. AccessType: inputOutput
   * @details Single contained Color or ColorRGBA node that can specify color
   * values applied to corresponding vertices according to colorPerVertex field.
   * @return SFNode The current value of color.
   */
  SFNode getColor() const { return _color; }

  /**
   * @brief Acceptable node types for the color field.
   * @details Permitted X3D node types: X3DColorNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableColorNodeTypes() {
    static const std::vector<std::string> types = {"X3DColorNode"};
    return types;
  }

  /**
   * @brief Sets the value of color. AccessType: inputOutput
   * @details Single contained Color or ColorRGBA node that can specify color
   * values applied to corresponding vertices according to colorPerVertex field.
   * @param value The new value for color.
   */
  void setColor(const SFNode &value) { _color = value; }

  void setColor(SFNode &&value) { _color = std::move(value); }

  /**
   * @brief Gets the value of colorPerVertex. AccessType: initializeOnly
   * @details Whether Color node color values are applied to each point vertex
   * (true) or per quadrilateral (false).
   * @return SFBool The current value of colorPerVertex.
   */
  SFBool getColorPerVertex() const { return _colorPerVertex; }
  /**
   * @brief Data-layer write of colorPerVertex (reader/init ingest path).
   * @details colorPerVertex is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setColorPerVertex().
   */
  void setColorPerVertexUnchecked(const SFBool &value) {
    _colorPerVertex = value;
  }

  /**
   * @brief Gets the value of creaseAngle. AccessType: initializeOnly
   * @details creaseAngle defines angle (in radians) for determining whether
   * adjacent polygons are drawn with sharp edges or smooth shading.
   * @return SFDouble The current value of creaseAngle.
   */
  SFDouble getCreaseAngle() const { return _creaseAngle; }
  /**
   * @brief Data-layer write of creaseAngle (reader/init ingest path).
   * @details creaseAngle is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setCreaseAngle().
   */
  void setCreaseAngleUnchecked(const SFDouble &value) { _creaseAngle = value; }

  /**
   * @brief Gets the value of geoGridOrigin. AccessType: initializeOnly
   * @details Geographic coordinate for southwest (lower-left) corner of height
   * dataset.
   * @return SFVec3d The current value of geoGridOrigin.
   */
  SFVec3d getGeoGridOrigin() const { return _geoGridOrigin; }
  /**
   * @brief Data-layer write of geoGridOrigin (reader/init ingest path).
   * @details geoGridOrigin is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setGeoGridOrigin().
   */
  void setGeoGridOriginUnchecked(const SFVec3d &value) {
    _geoGridOrigin = value;
  }

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
   * @brief Gets the value of height. AccessType: initializeOnly
   * @details Contains xDimension rows * zDimension columns floating-point
   * values for elevation above ellipsoid.
   * @return MFDouble The current value of height.
   */
  MFDouble getHeight() const { return _height; }
  /**
   * @brief Data-layer write of height (reader/init ingest path).
   * @details height is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setHeight().
   */
  void setHeightUnchecked(const MFDouble &value) { _height = value; }

  /**
   * @brief Gets the value of normal. AccessType: inputOutput
   * @details Single contained Normal node that can specify perpendicular
   * vectors for corresponding vertices to support rendering computations,
   * applied according to the normalPerVertex field.
   * @return SFNode The current value of normal.
   */
  SFNode getNormal() const { return _normal; }

  /**
   * @brief Acceptable node types for the normal field.
   * @details Permitted X3D node types: X3DNormalNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableNormalNodeTypes() {
    static const std::vector<std::string> types = {"X3DNormalNode"};
    return types;
  }

  /**
   * @brief Sets the value of normal. AccessType: inputOutput
   * @details Single contained Normal node that can specify perpendicular
   * vectors for corresponding vertices to support rendering computations,
   * applied according to the normalPerVertex field.
   * @param value The new value for normal.
   */
  void setNormal(const SFNode &value) { _normal = value; }

  void setNormal(SFNode &&value) { _normal = std::move(value); }

  /**
   * @brief Gets the value of normalPerVertex. AccessType: initializeOnly
   * @details Whether Normal node vector values are applied to each point vertex
   * (true) or per quadrilateral (false).
   * @return SFBool The current value of normalPerVertex.
   */
  SFBool getNormalPerVertex() const { return _normalPerVertex; }
  /**
   * @brief Data-layer write of normalPerVertex (reader/init ingest path).
   * @details normalPerVertex is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setNormalPerVertex().
   */
  void setNormalPerVertexUnchecked(const SFBool &value) {
    _normalPerVertex = value;
  }

  /**
   * @brief Event handler invoked when an event is received on set_height.
   * AccessType: inputOnly
   * @details Contains xDimension rows * zDimension columns floating-point
   * values for elevation above ellipsoid. Dispatches to the handler registered
   * via setOnSet_heightHandler(); a no-op if none is set. The event cascade
   * reaches this through the node's reflected field table, so routing is
   * node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_height(const MFDouble &value) {
    if (_onSet_heightHandler)
      _onSet_heightHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on set_height.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnSet_heightHandler(std::function<void(const MFDouble &)> handler) {
    _onSet_heightHandler = std::move(handler);
  }

  /**
   * @brief Gets the value of solid. AccessType: initializeOnly
   * @details Setting solid true means draw only one side of polygons (backface
   * culling on), setting solid false means draw both sides of polygons
   * (backface culling off).
   * @return SFBool The current value of solid.
   */
  SFBool getSolid() const { return _solid; }
  /**
   * @brief Data-layer write of solid (reader/init ingest path).
   * @details solid is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setSolid().
   */
  void setSolidUnchecked(const SFBool &value) { _solid = value; }

  /**
   * @brief Gets the value of texCoord. AccessType: inputOutput
   * @details Single contained TextureCoordinate, TextureCoordinateGenerator or
   * MultiTextureCoordinate node that can specify coordinates for texture
   * mapping onto corresponding geometry.
   * @return SFNode The current value of texCoord.
   */
  SFNode getTexCoord() const { return _texCoord; }

  /**
   * @brief Acceptable node types for the texCoord field.
   * @details Permitted X3D node types: X3DSingleTextureCoordinateNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableTexCoordNodeTypes() {
    static const std::vector<std::string> types = {
        "X3DSingleTextureCoordinateNode"};
    return types;
  }

  /**
   * @brief Sets the value of texCoord. AccessType: inputOutput
   * @details Single contained TextureCoordinate, TextureCoordinateGenerator or
   * MultiTextureCoordinate node that can specify coordinates for texture
   * mapping onto corresponding geometry.
   * @param value The new value for texCoord.
   */
  void setTexCoord(const SFNode &value) { _texCoord = value; }

  void setTexCoord(SFNode &&value) { _texCoord = std::move(value); }

  /**
   * @brief Gets the value of xDimension. AccessType: initializeOnly
   * @details Number of elements in the height array along east-west X
   * direction.
   * @return SFInt32 The current value of xDimension.
   */
  SFInt32 getXDimension() const { return _xDimension; }
  /**
   * @brief Data-layer write of xDimension (reader/init ingest path).
   * @details xDimension is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setXDimension().
   */
  void setXDimensionUnchecked(const SFInt32 &value) { _xDimension = value; }

  /**
   * @brief Gets the value of xSpacing. AccessType: initializeOnly
   * @details Distance between grid-array vertices along east-west X direction.
   * @return SFDouble The current value of xSpacing.
   */
  SFDouble getXSpacing() const { return _xSpacing; }
  /**
   * @brief Data-layer write of xSpacing (reader/init ingest path).
   * @details xSpacing is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setXSpacing().
   */
  void setXSpacingUnchecked(const SFDouble &value) { _xSpacing = value; }

  /**
   * @brief Gets the value of yScale. AccessType: inputOutput
   * @details Vertical exaggeration of displayed data produced from the height
   * array.
   * @return SFFloat The current value of yScale.
   */
  SFFloat getYScale() const { return _yScale; }

  /**
   * @brief Sets the value of yScale. AccessType: inputOutput
   * @details Vertical exaggeration of displayed data produced from the height
   * array.
   * @param value The new value for yScale.
   */
  void setYScale(const SFFloat &value) {

    validateYScale(value);

    _yScale = value;
  }

  /**
   * @brief Non-validating write of yScale (runtime/reader ingest path).
   * @details Assigns without the range check setYScale() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setYScale() stays the
   *          enforcement point for programmatic callers.
   */
  void setYScaleUnchecked(const SFFloat &value) { _yScale = value; }

  /**
   * @brief Gets the value of zDimension. AccessType: initializeOnly
   * @details Number of elements in the height array along north-south Z
   * direction.
   * @return SFInt32 The current value of zDimension.
   */
  SFInt32 getZDimension() const { return _zDimension; }
  /**
   * @brief Data-layer write of zDimension (reader/init ingest path).
   * @details zDimension is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setZDimension().
   */
  void setZDimensionUnchecked(const SFInt32 &value) { _zDimension = value; }

  /**
   * @brief Gets the value of zSpacing. AccessType: initializeOnly
   * @details Distance between grid-array vertices along north-south Z
   * direction.
   * @return SFDouble The current value of zSpacing.
   */
  SFDouble getZSpacing() const { return _zSpacing; }
  /**
   * @brief Data-layer write of zSpacing (reader/init ingest path).
   * @details zSpacing is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setZSpacing().
   */
  void setZSpacingUnchecked(const SFDouble &value) { _zSpacing = value; }

  /**
   * @brief The X3D type name of this node (e.g. "GeoElevationGrid").
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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesYScale(const SFFloat &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

private:
  static void validateYScale(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("yScale below minimum of 0");
  }

  /**
   * @brief Member variable for ccw.
   */

  SFBool _ccw{true};

  /**
   * @brief Member variable for color.
   */

  SFNode _color{nullptr};

  /**
   * @brief Member variable for colorPerVertex.
   */

  SFBool _colorPerVertex{true};

  /**
   * @brief Member variable for creaseAngle.
   */

  SFDouble _creaseAngle{0};

  /**
   * @brief Member variable for geoGridOrigin.
   */

  SFVec3d _geoGridOrigin{SFVec3d{0, 0, 0}};

  /**
   * @brief Member variable for geoOrigin.
   */

  SFNode _geoOrigin{nullptr};

  /**
   * @brief Member variable for geoSystem.
   */

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};

  /**
   * @brief Member variable for height.
   */

  MFDouble _height{std::vector<double>{0, 0}};

  /**
   * @brief Member variable for normal.
   */

  SFNode _normal{nullptr};

  /**
   * @brief Member variable for normalPerVertex.
   */

  SFBool _normalPerVertex{true};

  /**
   * @brief Registered event handler for set_height (inputOnly); empty until
   * set.
   */
  std::function<void(const MFDouble &)> _onSet_heightHandler{};

  /**
   * @brief Member variable for solid.
   */

  SFBool _solid{true};

  /**
   * @brief Member variable for texCoord.
   */

  SFNode _texCoord{nullptr};

  /**
   * @brief Member variable for xDimension.
   */

  SFInt32 _xDimension{0};

  /**
   * @brief Member variable for xSpacing.
   */

  SFDouble _xSpacing{1.0};

  /**
   * @brief Member variable for yScale.
   */

  SFFloat _yScale{1};

  /**
   * @brief Member variable for zDimension.
   */

  SFInt32 _zDimension{0};

  /**
   * @brief Member variable for zSpacing.
   */

  SFDouble _zSpacing{1.0};
};

} // namespace x3d::nodes
