// ElevationGrid.hpp
#ifndef ELEVATIONGRID_HPP
#define ELEVATIONGRID_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DGeometryNode.hpp"

/**
 * @class ElevationGrid
 * @brief ElevationGrid is a geometry node defining a rectangular height field,
 * with default values for a 1m by 1m square at height 0.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry3D.html#ElevationGrid
 */
class ElevationGrid : public virtual X3DGeometryNode {
public:
  /**
   * @brief Default constructor for ElevationGrid
   */
  ElevationGrid() = default;

  /**
   * @brief Destructor for ElevationGrid
   */
  ~ElevationGrid() = default;

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
   * @return SFFloat The default value
   */
  static SFFloat getDefaultCreaseAngle() { return 0; }

  /**
   * @brief Get the default value for fogCoord
   * @return SFNode The default value
   */
  static SFNode getDefaultFogCoord() { return nullptr; }

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
   * @return SFFloat The default value
   */
  static SFFloat getDefaultXSpacing() { return 1.0; }

  /**
   * @brief Get the default value for zDimension
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultZDimension() { return 0; }

  /**
   * @brief Get the default value for zSpacing
   * @return SFFloat The default value
   */
  static SFFloat getDefaultZSpacing() { return 1.0; }

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
  static std::string componentName() { return "Geometry3D"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 3; }

  /**
   * @brief Gets the value of attrib. AccessType: inputOutput
   * @details Single contained FloatVertexAttribute node that can specify list
   * of per-vertex attribute information for programmable shaders.
   * @return MFNode The current value of attrib.
   */
  MFNode getAttrib() const { return _attrib; }

  /**
   * @brief Acceptable node types for the attrib field.
   * @details Permitted X3D node types: X3DVertexAttributeNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableAttribNodeTypes() {
    static const std::vector<std::string> types = {"X3DVertexAttributeNode"};
    return types;
  }

  /**
   * @brief Sets the value of attrib. AccessType: inputOutput
   * @details Single contained FloatVertexAttribute node that can specify list
   * of per-vertex attribute information for programmable shaders.
   * @param value The new value for attrib.
   */
  void setAttrib(const MFNode &value) { _attrib = value; }

  void setAttrib(MFNode &&value) { _attrib = std::move(value); }

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
   * @return SFFloat The current value of creaseAngle.
   */
  SFFloat getCreaseAngle() const { return _creaseAngle; }
  /**
   * @brief Data-layer write of creaseAngle (reader/init ingest path).
   * @details creaseAngle is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setCreaseAngle().
   */
  void setCreaseAngleUnchecked(const SFFloat &value) { _creaseAngle = value; }

  /**
   * @brief Gets the value of fogCoord. AccessType: inputOutput
   * @details Single contained FogCoordinate node that can specify depth
   * parameters for fog in corresponding geometry.
   * @return SFNode The current value of fogCoord.
   */
  SFNode getFogCoord() const { return _fogCoord; }

  /**
   * @brief Acceptable node types for the fogCoord field.
   * @details Permitted X3D node types: FogCoordinate
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableFogCoordNodeTypes() {
    static const std::vector<std::string> types = {"FogCoordinate"};
    return types;
  }

  /**
   * @brief Sets the value of fogCoord. AccessType: inputOutput
   * @details Single contained FogCoordinate node that can specify depth
   * parameters for fog in corresponding geometry.
   * @param value The new value for fogCoord.
   */
  void setFogCoord(const SFNode &value) { _fogCoord = value; }

  void setFogCoord(SFNode &&value) { _fogCoord = std::move(value); }

  /**
   * @brief Gets the value of height. AccessType: initializeOnly
   * @details Grid array of height vertices with upward direction along +Y axis,
   * with xDimension rows and zDimension columns.
   * @return MFFloat The current value of height.
   */
  MFFloat getHeight() const { return _height; }
  /**
   * @brief Data-layer write of height (reader/init ingest path).
   * @details height is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setHeight().
   */
  void setHeightUnchecked(const MFFloat &value) { _height = value; }

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
   * @details Grid array of height vertices with upward direction along +Y axis,
   * with xDimension rows and zDimension columns. Dispatches to the handler
   * registered via setOnSet_heightHandler(); a no-op if none is set. The event
   * cascade reaches this through the node's reflected field table, so routing
   * is node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_height(const MFFloat &value) {
    if (_onSet_heightHandler)
      _onSet_heightHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on set_height.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnSet_heightHandler(std::function<void(const MFFloat &)> handler) {
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
   * @details Number of elements in the height array along X direction.
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
   * @details Meters distance between grid-array vertices along X direction.
   * @return SFFloat The current value of xSpacing.
   */
  SFFloat getXSpacing() const { return _xSpacing; }
  /**
   * @brief Data-layer write of xSpacing (reader/init ingest path).
   * @details xSpacing is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setXSpacing().
   */
  void setXSpacingUnchecked(const SFFloat &value) { _xSpacing = value; }

  /**
   * @brief Gets the value of zDimension. AccessType: initializeOnly
   * @details Number of elements in the height array along Z direction.
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
   * @details Meters distance between grid-array vertices along Z direction.
   * @return SFFloat The current value of zSpacing.
   */
  SFFloat getZSpacing() const { return _zSpacing; }
  /**
   * @brief Data-layer write of zSpacing (reader/init ingest path).
   * @details zSpacing is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setZSpacing().
   */
  void setZSpacingUnchecked(const SFFloat &value) { _zSpacing = value; }

  /**
   * @brief The X3D type name of this node (e.g. "ElevationGrid").
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
   * @brief Member variable for attrib.
   */

  MFNode _attrib{};

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

  SFFloat _creaseAngle{0};

  /**
   * @brief Member variable for fogCoord.
   */

  SFNode _fogCoord{nullptr};

  /**
   * @brief Member variable for height.
   */

  MFFloat _height{};

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
  std::function<void(const MFFloat &)> _onSet_heightHandler{};

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

  SFFloat _xSpacing{1.0};

  /**
   * @brief Member variable for zDimension.
   */

  SFInt32 _zDimension{0};

  /**
   * @brief Member variable for zSpacing.
   */

  SFFloat _zSpacing{1.0};
};

#endif // ELEVATIONGRID_HPP