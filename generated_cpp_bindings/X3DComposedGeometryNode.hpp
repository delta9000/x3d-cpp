// X3DComposedGeometryNode.hpp
#ifndef X3DCOMPOSEDGEOMETRYNODE_HPP
#define X3DCOMPOSEDGEOMETRYNODE_HPP

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
 * @class X3DComposedGeometryNode
 * @brief Composed geometry nodes produce renderable geometry, can contain Color
 * Coordinate Normal TextureCoordinate, and are contained by a Shape node.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rendering.html#X3DComposedGeometryNode
 */
class X3DComposedGeometryNode : public virtual X3DGeometryNode {
public:
  /**
   * @brief Default constructor for X3DComposedGeometryNode
   */
  X3DComposedGeometryNode() = default;

  /**
   * @brief Virtual destructor for X3DComposedGeometryNode
   */
  virtual ~X3DComposedGeometryNode() = default;

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
   * @brief Get the default container field value
   * @details No explicit containerField in the spec for this node; falls back
   *          to the X3D default field "children".
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

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
   * @details
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
   * @details
   * @param value The new value for attrib.
   */
  void setAttrib(const MFNode &value) { _attrib = value; }

  void setAttrib(MFNode &&value) { _attrib = std::move(value); }

  /**
   * @brief Gets the value of ccw. AccessType: initializeOnly
   * @details
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
   * @details
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
   * @details
   * @param value The new value for color.
   */
  void setColor(const SFNode &value) { _color = value; }

  void setColor(SFNode &&value) { _color = std::move(value); }

  /**
   * @brief Gets the value of colorPerVertex. AccessType: initializeOnly
   * @details
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
   * @details
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
   * @details
   * @param value The new value for coord.
   */
  void setCoord(const SFNode &value) { _coord = value; }

  void setCoord(SFNode &&value) { _coord = std::move(value); }

  /**
   * @brief Gets the value of fogCoord. AccessType: inputOutput
   * @details
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
   * @details
   * @param value The new value for fogCoord.
   */
  void setFogCoord(const SFNode &value) { _fogCoord = value; }

  void setFogCoord(SFNode &&value) { _fogCoord = std::move(value); }

  /**
   * @brief Gets the value of normal. AccessType: inputOutput
   * @details
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
   * @details
   * @param value The new value for normal.
   */
  void setNormal(const SFNode &value) { _normal = value; }

  void setNormal(SFNode &&value) { _normal = std::move(value); }

  /**
   * @brief Gets the value of normalPerVertex. AccessType: initializeOnly
   * @details
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
   * @brief Gets the value of solid. AccessType: initializeOnly
   * @details
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
   * @details
   * @return SFNode The current value of texCoord.
   */
  SFNode getTexCoord() const { return _texCoord; }

  /**
   * @brief Acceptable node types for the texCoord field.
   * @details Permitted X3D node types: X3DSingleTextureCoordinateNode,
   * MultiTextureCoordinate
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableTexCoordNodeTypes() {
    static const std::vector<std::string> types = {
        "X3DSingleTextureCoordinateNode", "MultiTextureCoordinate"};
    return types;
  }

  /**
   * @brief Sets the value of texCoord. AccessType: inputOutput
   * @details
   * @param value The new value for texCoord.
   */
  void setTexCoord(const SFNode &value) { _texCoord = value; }

  void setTexCoord(SFNode &&value) { _texCoord = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "X3DComposedGeometryNode").
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
   * @brief Member variable for normalPerVertex.
   */

  SFBool _normalPerVertex{true};

  /**
   * @brief Member variable for solid.
   */

  SFBool _solid{true};

  /**
   * @brief Member variable for texCoord.
   */

  SFNode _texCoord{nullptr};
};

#endif // X3DCOMPOSEDGEOMETRYNODE_HPP