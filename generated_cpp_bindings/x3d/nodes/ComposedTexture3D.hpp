// ComposedTexture3D.hpp
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

#include "x3d/nodes/X3DTextureNode.hpp"

#include "x3d/nodes/X3DTexture3DNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ComposedTexture3D
 * @brief ComposedTexture3D defines a 3D image-based texture map as a collection
 * of 2D texture sources at various depths.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texture3D.html#ComposedTexture3D
 */
class ComposedTexture3D : public virtual X3DTexture3DNode {
public:
  /**
   * @brief Default constructor for ComposedTexture3D
   */
  ComposedTexture3D() = default;

  /**
   * @brief Destructor for ComposedTexture3D
   */
  ~ComposedTexture3D() = default;

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DTexture3DNode";
  }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "texture"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Texturing3D"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of texture. AccessType: inputOutput
   * @details collection of 2D texture sources.
   * @return const MFNode& The current value of texture.
   */
  const MFNode &getTexture() const { return _texture; }

  /**
   * @brief Acceptable node types for the texture field.
   * @details Permitted X3D node types: X3DTexture2DNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * @brief Sets the value of texture. AccessType: inputOutput
   * @details collection of 2D texture sources.
   * @param value The new value for texture.
   */
  void setTexture(const MFNode &value) { _texture = value; }

  void setTexture(MFNode &&value) { _texture = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "ComposedTexture3D").
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
   * @brief Member variable for texture.
   */

  MFNode _texture{};
};

} // namespace x3d::nodes
