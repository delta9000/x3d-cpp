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
  GeoElevationGrid() = default;

  ~GeoElevationGrid() = default;

  static SFBool getDefaultCcw() { return true; }

  static SFNode getDefaultColor() { return nullptr; }

  static SFBool getDefaultColorPerVertex() { return true; }

  static SFDouble getDefaultCreaseAngle() { return 0; }

  static SFVec3d getDefaultGeoGridOrigin() { return SFVec3d{0, 0, 0}; }

  static SFNode getDefaultGeoOrigin() { return nullptr; }

  static MFString getDefaultGeoSystem() {
    return std::vector<std::string>{"GD", "WE"};
  }

  static MFDouble getDefaultHeight() { return std::vector<double>{0, 0}; }

  static SFNode getDefaultNormal() { return nullptr; }

  static SFBool getDefaultNormalPerVertex() { return true; }

  static SFBool getDefaultSolid() { return true; }

  static SFNode getDefaultTexCoord() { return nullptr; }

  static SFInt32 getDefaultXDimension() { return 0; }

  static SFDouble getDefaultXSpacing() { return 1.0; }

  static SFFloat getDefaultYScale() { return 1; }

  static SFInt32 getDefaultZDimension() { return 0; }

  static SFDouble getDefaultZSpacing() { return 1.0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "Geospatial"; }

  static int componentLevel() { return 1; }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details ccw defines clockwise/counterclockwise ordering of vertex
   * coordinates, which in turn defines front/back orientation of polygon
   * normals according to Right-Hand Rule (RHR).
   */
  const SFBool &getCcw() const { return _ccw; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setCcwUnchecked(const SFBool &value) { _ccw = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained Color or ColorRGBA node that can specify color
   * values applied to corresponding vertices according to colorPerVertex field.
   */
  const SFNode &getColor() const { return _color; }

  /**
   * @details Permitted X3D node types: X3DColorNode
   */
  static const std::vector<std::string> &acceptableColorNodeTypes() {
    static const std::vector<std::string> types = {"X3DColorNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained Color or ColorRGBA node that can specify color
   * values applied to corresponding vertices according to colorPerVertex field.
   */
  void setColor(const SFNode &value) { _color = value; }

  void setColor(SFNode &&value) { _color = std::move(value); }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details Whether Color node color values are applied to each point vertex
   * (true) or per quadrilateral (false).
   */
  const SFBool &getColorPerVertex() const { return _colorPerVertex; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setColorPerVertexUnchecked(const SFBool &value) {
    _colorPerVertex = value;
  }

  /**
   * SFDouble; access: initializeOnly. Borrows the stored value.
   * @details creaseAngle defines angle (in radians) for determining whether
   * adjacent polygons are drawn with sharp edges or smooth shading.
   */
  const SFDouble &getCreaseAngle() const { return _creaseAngle; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setCreaseAngleUnchecked(const SFDouble &value) { _creaseAngle = value; }

  /**
   * SFVec3d; access: initializeOnly. Borrows the stored value.
   * @details Geographic coordinate for southwest (lower-left) corner of height
   * dataset.
   */
  const SFVec3d &getGeoGridOrigin() const { return _geoGridOrigin; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setGeoGridOriginUnchecked(const SFVec3d &value) {
    _geoGridOrigin = value;
  }

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
   * MFDouble; access: initializeOnly. Borrows the stored value.
   * @details Contains xDimension rows * zDimension columns floating-point
   * values for elevation above ellipsoid.
   */
  const MFDouble &getHeight() const { return _height; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setHeightUnchecked(const MFDouble &value) { _height = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained Normal node that can specify perpendicular
   * vectors for corresponding vertices to support rendering computations,
   * applied according to the normalPerVertex field.
   */
  const SFNode &getNormal() const { return _normal; }

  /**
   * @details Permitted X3D node types: X3DNormalNode
   */
  static const std::vector<std::string> &acceptableNormalNodeTypes() {
    static const std::vector<std::string> types = {"X3DNormalNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained Normal node that can specify perpendicular
   * vectors for corresponding vertices to support rendering computations,
   * applied according to the normalPerVertex field.
   */
  void setNormal(const SFNode &value) { _normal = value; }

  void setNormal(SFNode &&value) { _normal = std::move(value); }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details Whether Normal node vector values are applied to each point vertex
   * (true) or per quadrilateral (false).
   */
  const SFBool &getNormalPerVertex() const { return _normalPerVertex; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setNormalPerVertexUnchecked(const SFBool &value) {
    _normalPerVertex = value;
  }

  /**
   * MFDouble; access: inputOnly.
   * @details Contains xDimension rows * zDimension columns floating-point
   * values for elevation above ellipsoid. Dispatches to the handler registered
   * via setOnSet_heightHandler(); a no-op if none is set.
   */
  void onSet_height(const MFDouble &value) {
    if (_onSet_heightHandler)
      _onSet_heightHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_heightHandler(std::function<void(const MFDouble &)> handler) {
    _onSet_heightHandler = std::move(handler);
  }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details Setting solid true means draw only one side of polygons (backface
   * culling on), setting solid false means draw both sides of polygons
   * (backface culling off).
   */
  const SFBool &getSolid() const { return _solid; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setSolidUnchecked(const SFBool &value) { _solid = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained TextureCoordinate, TextureCoordinateGenerator or
   * MultiTextureCoordinate node that can specify coordinates for texture
   * mapping onto corresponding geometry.
   */
  const SFNode &getTexCoord() const { return _texCoord; }

  /**
   * @details Permitted X3D node types: X3DSingleTextureCoordinateNode
   */
  static const std::vector<std::string> &acceptableTexCoordNodeTypes() {
    static const std::vector<std::string> types = {
        "X3DSingleTextureCoordinateNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained TextureCoordinate, TextureCoordinateGenerator or
   * MultiTextureCoordinate node that can specify coordinates for texture
   * mapping onto corresponding geometry.
   */
  void setTexCoord(const SFNode &value) { _texCoord = value; }

  void setTexCoord(SFNode &&value) { _texCoord = std::move(value); }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details Number of elements in the height array along east-west X
   * direction.
   */
  const SFInt32 &getXDimension() const { return _xDimension; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setXDimensionUnchecked(const SFInt32 &value) { _xDimension = value; }

  /**
   * SFDouble; access: initializeOnly. Borrows the stored value.
   * @details Distance between grid-array vertices along east-west X direction.
   */
  const SFDouble &getXSpacing() const { return _xSpacing; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setXSpacingUnchecked(const SFDouble &value) { _xSpacing = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Vertical exaggeration of displayed data produced from the height
   * array.
   */
  const SFFloat &getYScale() const { return _yScale; }

  /**
   * Access: inputOutput.
   * @details Vertical exaggeration of displayed data produced from the height
   * array.
   */
  void setYScale(const SFFloat &value) {

    validateYScale(value);

    _yScale = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setYScale() for range validation.
   */
  void setYScaleUnchecked(const SFFloat &value) { _yScale = value; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details Number of elements in the height array along north-south Z
   * direction.
   */
  const SFInt32 &getZDimension() const { return _zDimension; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setZDimensionUnchecked(const SFInt32 &value) { _zDimension = value; }

  /**
   * SFDouble; access: initializeOnly. Borrows the stored value.
   * @details Distance between grid-array vertices along north-south Z
   * direction.
   */
  const SFDouble &getZSpacing() const { return _zSpacing; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setZSpacingUnchecked(const SFDouble &value) { _zSpacing = value; }

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
  static void checkRangesCreaseAngle(const SFDouble &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesXDimension(const SFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesYScale(const SFFloat &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesZDimension(const SFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

private:
  static void validateYScale(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("yScale below minimum of 0");
  }

  SFBool _ccw{true};

  SFNode _color{nullptr};

  SFBool _colorPerVertex{true};

  SFDouble _creaseAngle{0};

  SFVec3d _geoGridOrigin{SFVec3d{0, 0, 0}};

  SFNode _geoOrigin{nullptr};

  MFString _geoSystem{std::vector<std::string>{"GD", "WE"}};

  MFDouble _height{std::vector<double>{0, 0}};

  SFNode _normal{nullptr};

  SFBool _normalPerVertex{true};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFDouble &)> _onSet_heightHandler{};

  SFBool _solid{true};

  SFNode _texCoord{nullptr};

  SFInt32 _xDimension{0};

  SFDouble _xSpacing{1.0};

  SFFloat _yScale{1};

  SFInt32 _zDimension{0};

  SFDouble _zSpacing{1.0};
};

} // namespace x3d::nodes
