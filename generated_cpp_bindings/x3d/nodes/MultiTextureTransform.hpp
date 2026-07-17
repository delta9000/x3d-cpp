// MultiTextureTransform.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DAppearanceChildNode.hpp"

#include "x3d/nodes/X3DTextureTransformNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class MultiTextureTransform
 * @brief MultiTextureTransform contains multiple TextureTransform nodes, each
 * provided for use by corresponding ImageTexture MovieTexture or PixelTexture
 * nodes within a sibling MultiTexture node.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texturing.html#MultiTextureTransform
 */
class MultiTextureTransform : public virtual X3DTextureTransformNode {
public:
  /**
   * @brief Default constructor for MultiTextureTransform
   */
  MultiTextureTransform() = default;

  /**
   * @brief Destructor for MultiTextureTransform
   */
  ~MultiTextureTransform() = default;

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "textureTransform"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Texturing"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of textureTransform. AccessType: inputOutput
   * @details Zero or more contained TextureTransform nodes, for each of the
   * different texture channels, that define 2D transformation applied to
   * texture coordinates.
   * @return const MFNode& The current value of textureTransform.
   */
  const MFNode &getTextureTransform() const { return _textureTransform; }

  /**
   * @brief Acceptable node types for the textureTransform field.
   * @details Permitted X3D node types: X3DSingleTextureTransformNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableTextureTransformNodeTypes() {
    static const std::vector<std::string> types = {
        "X3DSingleTextureTransformNode"};
    return types;
  }

  /**
   * @brief Sets the value of textureTransform. AccessType: inputOutput
   * @details Zero or more contained TextureTransform nodes, for each of the
   * different texture channels, that define 2D transformation applied to
   * texture coordinates.
   * @param value The new value for textureTransform.
   */
  void setTextureTransform(const MFNode &value) { _textureTransform = value; }

  void setTextureTransform(MFNode &&value) {

    _textureTransform = std::move(value);
  }

  /**
   * @brief The X3D type name of this node (e.g. "MultiTextureTransform").
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
   * @brief Member variable for textureTransform.
   */

  MFNode _textureTransform{};
};

} // namespace x3d::nodes
