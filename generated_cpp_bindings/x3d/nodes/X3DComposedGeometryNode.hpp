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
 * @class X3DComposedGeometryNode
 * @brief Composed geometry nodes produce renderable geometry, can contain Color
 * Coordinate Normal TextureCoordinate, and are contained by a Shape node.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rendering.html#X3DComposedGeometryNode
 */
class X3DComposedGeometryNode : public virtual X3DGeometryNode {
public:
  X3DComposedGeometryNode() = default;

  virtual ~X3DComposedGeometryNode() = default;

  static SFBool getDefaultCcw() { return true; }

  static SFNode getDefaultColor() { return nullptr; }

  static SFBool getDefaultColorPerVertex() { return true; }

  static SFNode getDefaultCoord() { return nullptr; }

  static SFNode getDefaultFogCoord() { return nullptr; }

  static SFNode getDefaultNormal() { return nullptr; }

  static SFBool getDefaultNormalPerVertex() { return true; }

  static SFBool getDefaultSolid() { return true; }

  static SFNode getDefaultTexCoord() { return nullptr; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Rendering"; }

  static int componentLevel() { return 1; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details
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
   * @details
   */
  void setAttrib(const MFNode &value) { _attrib = value; }

  void setAttrib(MFNode &&value) { _attrib = std::move(value); }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFBool &getCcw() const { return _ccw; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setCcwUnchecked(const SFBool &value) { _ccw = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details
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
   * @details
   */
  void setColor(const SFNode &value) { _color = value; }

  void setColor(SFNode &&value) { _color = std::move(value); }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details
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
   * @details
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
   * @details
   */
  void setCoord(const SFNode &value) { _coord = value; }

  void setCoord(SFNode &&value) { _coord = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details
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
   * @details
   */
  void setFogCoord(const SFNode &value) { _fogCoord = value; }

  void setFogCoord(SFNode &&value) { _fogCoord = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details
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
   * @details
   */
  void setNormal(const SFNode &value) { _normal = value; }

  void setNormal(SFNode &&value) { _normal = std::move(value); }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details
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
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details
   */
  const SFBool &getSolid() const { return _solid; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setSolidUnchecked(const SFBool &value) { _solid = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFNode &getTexCoord() const { return _texCoord; }

  /**
   * @details Permitted X3D node types: X3DSingleTextureCoordinateNode,
   * MultiTextureCoordinate
   */
  static const std::vector<std::string> &acceptableTexCoordNodeTypes() {
    static const std::vector<std::string> types = {
        "X3DSingleTextureCoordinateNode", "MultiTextureCoordinate"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setTexCoord(const SFNode &value) { _texCoord = value; }

  void setTexCoord(SFNode &&value) { _texCoord = std::move(value); }

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

private:
  MFNode _attrib{};

  SFBool _ccw{true};

  SFNode _color{nullptr};

  SFBool _colorPerVertex{true};

  SFNode _coord{nullptr};

  SFNode _fogCoord{nullptr};

  SFNode _normal{nullptr};

  SFBool _normalPerVertex{true};

  SFBool _solid{true};

  SFNode _texCoord{nullptr};
};

} // namespace x3d::nodes
