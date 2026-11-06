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
  LineSet() = default;

  ~LineSet() = default;

  static SFNode getDefaultColor() { return nullptr; }

  static SFNode getDefaultCoord() { return nullptr; }

  static SFNode getDefaultFogCoord() { return nullptr; }

  static SFNode getDefaultNormal() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "Rendering"; }

  static int componentLevel() { return 1; }

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
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained Color or ColorRGBA node that can specify color
   * values applied to corresponding vertices according to colorIndex and
   * colorPerVertex fields.
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
   * values applied to corresponding vertices according to colorIndex and
   * colorPerVertex fields.
   */
  void setColor(const SFNode &value) { _color = value; }

  void setColor(SFNode &&value) { _color = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained Coordinate or CoordinateDouble node that can
   * specify a list of vertex values.
   */
  const SFNode &getCoord() const { return _coord; }

  /**
   * @details Permitted X3D node types: X3DCoordinateNode
   */
  static const std::vector<std::string> &acceptableCoordNodeTypes() {
    static const std::vector<std::string> types = {"X3DCoordinateNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained Coordinate or CoordinateDouble node that can
   * specify a list of vertex values.
   */
  void setCoord(const SFNode &value) { _coord = value; }

  void setCoord(SFNode &&value) { _coord = std::move(value); }

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
   * MFInt32; access: inputOutput. Borrows the stored value.
   * @details vertexCount describes how many vertices are used in each
   * individual polyline segment from the Coordinate point values.
   */
  const MFInt32 &getVertexCount() const { return _vertexCount; }

  /**
   * Access: inputOutput.
   * @details vertexCount describes how many vertices are used in each
   * individual polyline segment from the Coordinate point values.
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
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setVertexCount() for range validation.
   */
  void setVertexCountUnchecked(const MFInt32 &value) { _vertexCount = value; }

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

  MFNode _attrib{};

  SFNode _color{nullptr};

  SFNode _coord{nullptr};

  SFNode _fogCoord{nullptr};

  SFNode _normal{nullptr};

  MFInt32 _vertexCount{};
};

} // namespace x3d::nodes
