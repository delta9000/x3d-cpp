// IndexedLineSet.hpp
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
 * @class IndexedLineSet
 * @brief IndexedLineSet defines polyline segments using index lists
 * corresponding to vertex coordinates.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rendering.html#IndexedLineSet
 */
class IndexedLineSet : public virtual X3DGeometryNode {
public:
  /**
   * @brief Default constructor for IndexedLineSet
   */
  IndexedLineSet() = default;

  /**
   * @brief Destructor for IndexedLineSet
   */
  ~IndexedLineSet() = default;

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
   * @brief Get the default value for coord
   * @return SFNode The default value
   */
  static SFNode getDefaultCoord() { return nullptr; }

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
  static std::string componentName() { return "Rendering"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of attrib. AccessType: inputOutput
   * @details Single contained FloatVertexAttribute node that can specify list
   * of per-vertex attribute information for programmable shaders.
   * @return const MFNode& The current value of attrib.
   */
  const MFNode &getAttrib() const { return _attrib; }

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
   * @brief Gets the value of color. AccessType: inputOutput
   * @details Single contained Color or ColorRGBA node that can specify color
   * values applied to corresponding vertices according to colorIndex and
   * colorPerVertex fields.
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
   * values applied to corresponding vertices according to colorIndex and
   * colorPerVertex fields.
   * @param value The new value for color.
   */
  void setColor(const SFNode &value) { _color = value; }

  void setColor(SFNode &&value) { _color = std::move(value); }

  /**
   * @brief Gets the value of colorIndex. AccessType: initializeOnly
   * @details colorIndex values define the order in which Color|ColorRGBA values
   * are applied to polygons (or vertices).
   * @return MFInt32 The current value of colorIndex.
   */
  MFInt32 getColorIndex() const { return _colorIndex; }
  /**
   * @brief Data-layer write of colorIndex (reader/init ingest path).
   * @details colorIndex is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setColorIndex().
   */
  void setColorIndexUnchecked(const MFInt32 &value) { _colorIndex = value; }

  /**
   * @brief Gets the value of colorPerVertex. AccessType: initializeOnly
   * @details Whether Color node color values are applied to each point vertex
   * (true) or per polyline (false).
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
   * @brief Gets the value of coord. AccessType: inputOutput
   * @details Single contained Coordinate or CoordinateDouble node that can
   * specify a list of vertex values.
   * @return SFNode The current value of coord.
   */
  SFNode getCoord() const { return _coord; }

  /**
   * @brief Acceptable node types for the coord field.
   * @details Permitted X3D node types: X3DCoordinateNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableCoordNodeTypes() {
    static const std::vector<std::string> types = {"X3DCoordinateNode"};
    return types;
  }

  /**
   * @brief Sets the value of coord. AccessType: inputOutput
   * @details Single contained Coordinate or CoordinateDouble node that can
   * specify a list of vertex values.
   * @param value The new value for coord.
   */
  void setCoord(const SFNode &value) { _coord = value; }

  void setCoord(SFNode &&value) { _coord = std::move(value); }

  /**
   * @brief Gets the value of coordIndex. AccessType: initializeOnly
   * @details coordIndex indices provide the order in which coordinates are
   * applied to construct each polygon face.
   * @return MFInt32 The current value of coordIndex.
   */
  MFInt32 getCoordIndex() const { return _coordIndex; }
  /**
   * @brief Data-layer write of coordIndex (reader/init ingest path).
   * @details coordIndex is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setCoordIndex().
   */
  void setCoordIndexUnchecked(const MFInt32 &value) { _coordIndex = value; }

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
   * @brief Event handler invoked when an event is received on set_colorIndex.
   * AccessType: inputOnly
   * @details colorIndex values define the order in which Color|ColorRGBA values
   * are applied to polygons (or vertices). Dispatches to the handler registered
   * via setOnSet_colorIndexHandler(); a no-op if none is set. The event cascade
   * reaches this through the node's reflected field table, so routing is
   * node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_colorIndex(const MFInt32 &value) {
    if (_onSet_colorIndexHandler)
      _onSet_colorIndexHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * set_colorIndex.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void
  setOnSet_colorIndexHandler(std::function<void(const MFInt32 &)> handler) {
    _onSet_colorIndexHandler = std::move(handler);
  }

  /**
   * @brief Event handler invoked when an event is received on set_coordIndex.
   * AccessType: inputOnly
   * @details coordIndex indices provide the order in which coordinates are
   * applied to construct each polyline. Dispatches to the handler registered
   * via setOnSet_coordIndexHandler(); a no-op if none is set. The event cascade
   * reaches this through the node's reflected field table, so routing is
   * node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_coordIndex(const MFInt32 &value) {
    if (_onSet_coordIndexHandler)
      _onSet_coordIndexHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * set_coordIndex.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void
  setOnSet_coordIndexHandler(std::function<void(const MFInt32 &)> handler) {
    _onSet_coordIndexHandler = std::move(handler);
  }

  /**
   * @brief The X3D type name of this node (e.g. "IndexedLineSet").
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
  static void checkRangesColorIndex(const MFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesCoordIndex(const MFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

private:
  /**
   * @brief Member variable for attrib.
   */

  MFNode _attrib{};

  /**
   * @brief Member variable for color.
   */

  SFNode _color{nullptr};

  /**
   * @brief Member variable for colorIndex.
   */

  MFInt32 _colorIndex{};

  /**
   * @brief Member variable for colorPerVertex.
   */

  SFBool _colorPerVertex{true};

  /**
   * @brief Member variable for coord.
   */

  SFNode _coord{nullptr};

  /**
   * @brief Member variable for coordIndex.
   */

  MFInt32 _coordIndex{};

  /**
   * @brief Member variable for fogCoord.
   */

  SFNode _fogCoord{nullptr};

  /**
   * @brief Member variable for normal.
   */

  SFNode _normal{nullptr};

  /**
   * @brief Registered event handler for set_colorIndex (inputOnly); empty until
   * set.
   */
  std::function<void(const MFInt32 &)> _onSet_colorIndexHandler{};

  /**
   * @brief Registered event handler for set_coordIndex (inputOnly); empty until
   * set.
   */
  std::function<void(const MFInt32 &)> _onSet_coordIndexHandler{};
};

} // namespace x3d::nodes
