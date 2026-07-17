// LineSet.hpp
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
 * @class LineSet
 * @brief LineSet is a geometry node that can contain a
 * Coordinate|CoordinateDouble node and optionally a Color|ColorRGBA node.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rendering.html#LineSet
 */
class LineSet : public virtual X3DGeometryNode {
public:
  /**
   * @brief Default constructor for LineSet
   */
  LineSet() = default;

  /**
   * @brief Destructor for LineSet
   */
  ~LineSet() = default;

  /**
   * @brief Get the default value for color
   * @return SFNode The default value
   */
  static SFNode getDefaultColor() { return nullptr; }

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
   * @brief Gets the value of vertexCount. AccessType: inputOutput
   * @details vertexCount describes how many vertices are used in each
   * individual polyline segment from the Coordinate point values.
   * @return MFInt32 The current value of vertexCount.
   */
  MFInt32 getVertexCount() const { return _vertexCount; }

  /**
   * @brief Sets the value of vertexCount. AccessType: inputOutput
   * @details vertexCount describes how many vertices are used in each
   * individual polyline segment from the Coordinate point values.
   * @param value The new value for vertexCount.
   */
  void setVertexCount(const MFInt32 &value) {

    validateVertexCount(value);

    _vertexCount = value;
  }

  void setVertexCount(MFInt32 &&value) {

    validateVertexCount(value);

    _vertexCount = std::move(value);
  }

  /**
   * @brief Non-validating write of vertexCount (runtime/reader ingest path).
   * @details Assigns without the range check setVertexCount() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setVertexCount() stays the
   *          enforcement point for programmatic callers.
   */
  void setVertexCountUnchecked(const MFInt32 &value) { _vertexCount = value; }

  /**
   * @brief The X3D type name of this node (e.g. "LineSet").
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
  static void checkRangesVertexCount(const MFInt32 &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

private:
  static void validateVertexCount(const MFInt32 &value) {

    for (const auto &v : value) {

      if (v < 2)
        throw std::out_of_range("vertexCount below minimum of 2");
    }
  }

  /**
   * @brief Member variable for attrib.
   */

  MFNode _attrib{};

  /**
   * @brief Member variable for color.
   */

  SFNode _color{nullptr};

  /**
   * @brief Member variable for coord.
   */

  SFNode _coord{nullptr};

  /**
   * @brief Member variable for fogCoord.
   */

  SFNode _fogCoord{nullptr};

  /**
   * @brief Member variable for normal.
   */

  SFNode _normal{nullptr};

  /**
   * @brief Member variable for vertexCount.
   */

  MFInt32 _vertexCount{};
};

} // namespace x3d::nodes
