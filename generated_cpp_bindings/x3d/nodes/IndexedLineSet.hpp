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
  IndexedLineSet() = default;

  ~IndexedLineSet() = default;

  static SFNode getDefaultColor() { return nullptr; }

  static SFBool getDefaultColorPerVertex() { return true; }

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
   * MFInt32; access: initializeOnly. Borrows the stored value.
   * @details colorIndex values define the order in which Color|ColorRGBA values
   * are applied to polygons (or vertices).
   */
  const MFInt32 &getColorIndex() const { return _colorIndex; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setColorIndexUnchecked(const MFInt32 &value) { _colorIndex = value; }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details Whether Color node color values are applied to each point vertex
   * (true) or per polyline (false).
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
   * MFInt32; access: initializeOnly. Borrows the stored value.
   * @details coordIndex indices provide the order in which coordinates are
   * applied to construct each polygon face.
   */
  const MFInt32 &getCoordIndex() const { return _coordIndex; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setCoordIndexUnchecked(const MFInt32 &value) { _coordIndex = value; }

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
   * MFInt32; access: inputOnly.
   * @details colorIndex values define the order in which Color|ColorRGBA values
   * are applied to polygons (or vertices). Dispatches to the handler registered
   * via setOnSet_colorIndexHandler(); a no-op if none is set.
   */
  void onSet_colorIndex(const MFInt32 &value) {
    if (_onSet_colorIndexHandler)
      _onSet_colorIndexHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void
  setOnSet_colorIndexHandler(std::function<void(const MFInt32 &)> handler) {
    _onSet_colorIndexHandler = std::move(handler);
  }

  /**
   * MFInt32; access: inputOnly.
   * @details coordIndex indices provide the order in which coordinates are
   * applied to construct each polyline. Dispatches to the handler registered
   * via setOnSet_coordIndexHandler(); a no-op if none is set.
   */
  void onSet_coordIndex(const MFInt32 &value) {
    if (_onSet_coordIndexHandler)
      _onSet_coordIndexHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void
  setOnSet_coordIndexHandler(std::function<void(const MFInt32 &)> handler) {
    _onSet_coordIndexHandler = std::move(handler);
  }

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
  MFNode _attrib{};

  SFNode _color{nullptr};

  MFInt32 _colorIndex{};

  SFBool _colorPerVertex{true};

  SFNode _coord{nullptr};

  MFInt32 _coordIndex{};

  SFNode _fogCoord{nullptr};

  SFNode _normal{nullptr};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFInt32 &)> _onSet_colorIndexHandler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFInt32 &)> _onSet_coordIndexHandler{};
};

} // namespace x3d::nodes
