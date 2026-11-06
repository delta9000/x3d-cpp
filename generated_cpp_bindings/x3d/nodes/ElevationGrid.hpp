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
 * @class ElevationGrid
 * @brief ElevationGrid is a geometry node defining a rectangular height field,
 * with default values for a 1m by 1m square at height 0.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry3D.html#ElevationGrid
 */
class ElevationGrid : public virtual X3DGeometryNode {
public:
  ElevationGrid() = default;

  ~ElevationGrid() = default;

  static SFBool getDefaultCcw() { return true; }

  static SFNode getDefaultColor() { return nullptr; }

  static SFBool getDefaultColorPerVertex() { return true; }

  static SFFloat getDefaultCreaseAngle() { return 0; }

  static SFNode getDefaultFogCoord() { return nullptr; }

  static SFNode getDefaultNormal() { return nullptr; }

  static SFBool getDefaultNormalPerVertex() { return true; }

  static SFBool getDefaultSolid() { return true; }

  static SFNode getDefaultTexCoord() { return nullptr; }

  static SFInt32 getDefaultXDimension() { return 0; }

  static SFFloat getDefaultXSpacing() { return 1.0; }

  static SFInt32 getDefaultZDimension() { return 0; }

  static SFFloat getDefaultZSpacing() { return 1.0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "Geometry3D"; }

  static int componentLevel() { return 3; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained FloatVertexAttribute node that can specify list
   * of per-vertex attribute information for programmable shaders.
   */
  const MFNode &getAttrib() const { return _attrib; }

  /**
   * @details Permitted X3D node types: X3DVertexAttributeNode
   */
  static const std::vector<std::string> &acceptableAttribNodeTypes() {
    static const std::vector<std::string> types = {"X3DVertexAttributeNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained FloatVertexAttribute node that can specify list
   * of per-vertex attribute information for programmable shaders.
   */
  void setAttrib(const MFNode &value) { _attrib = value; }

  void setAttrib(MFNode &&value) { _attrib = std::move(value); }

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
   * SFFloat; access: initializeOnly. Borrows the stored value.
   * @details creaseAngle defines angle (in radians) for determining whether
   * adjacent polygons are drawn with sharp edges or smooth shading.
   */
  const SFFloat &getCreaseAngle() const { return _creaseAngle; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setCreaseAngleUnchecked(const SFFloat &value) { _creaseAngle = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained FogCoordinate node that can specify depth
   * parameters for fog in corresponding geometry.
   */
  const SFNode &getFogCoord() const { return _fogCoord; }

  /**
   * @details Permitted X3D node types: FogCoordinate
   */
  static const std::vector<std::string> &acceptableFogCoordNodeTypes() {
    static const std::vector<std::string> types = {"FogCoordinate"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained FogCoordinate node that can specify depth
   * parameters for fog in corresponding geometry.
   */
  void setFogCoord(const SFNode &value) { _fogCoord = value; }

  void setFogCoord(SFNode &&value) { _fogCoord = std::move(value); }

  /**
   * MFFloat; access: initializeOnly. Borrows the stored value.
   * @details Grid array of height vertices with upward direction along +Y axis,
   * with xDimension rows and zDimension columns.
   */
  const MFFloat &getHeight() const { return _height; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setHeightUnchecked(const MFFloat &value) { _height = value; }

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
   * MFFloat; access: inputOnly.
   * @details Grid array of height vertices with upward direction along +Y axis,
   * with xDimension rows and zDimension columns. Dispatches to the handler
   * registered via setOnSet_heightHandler(); a no-op if none is set.
   */
  void onSet_height(const MFFloat &value) {
    if (_onSet_heightHandler)
      _onSet_heightHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_heightHandler(std::function<void(const MFFloat &)> handler) {
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
   * @details Number of elements in the height array along X direction.
   */
  const SFInt32 &getXDimension() const { return _xDimension; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setXDimensionUnchecked(const SFInt32 &value) { _xDimension = value; }

  /**
   * SFFloat; access: initializeOnly. Borrows the stored value.
   * @details Meters distance between grid-array vertices along X direction.
   */
  const SFFloat &getXSpacing() const { return _xSpacing; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setXSpacingUnchecked(const SFFloat &value) { _xSpacing = value; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details Number of elements in the height array along Z direction.
   */
  const SFInt32 &getZDimension() const { return _zDimension; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setZDimensionUnchecked(const SFInt32 &value) { _zDimension = value; }

  /**
   * SFFloat; access: initializeOnly. Borrows the stored value.
   * @details Meters distance between grid-array vertices along Z direction.
   */
  const SFFloat &getZSpacing() const { return _zSpacing; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setZSpacingUnchecked(const SFFloat &value) { _zSpacing = value; }

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
  static void checkRangesCreaseAngle(const SFFloat &value,
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
  static void checkRangesZDimension(const SFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

private:
  MFNode _attrib{};

  SFBool _ccw{true};

  SFNode _color{nullptr};

  SFBool _colorPerVertex{true};

  SFFloat _creaseAngle{0};

  SFNode _fogCoord{nullptr};

  MFFloat _height{};

  SFNode _normal{nullptr};

  SFBool _normalPerVertex{true};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFFloat &)> _onSet_heightHandler{};

  SFBool _solid{true};

  SFNode _texCoord{nullptr};

  SFInt32 _xDimension{0};

  SFFloat _xSpacing{1.0};

  SFInt32 _zDimension{0};

  SFFloat _zSpacing{1.0};
};

} // namespace x3d::nodes
