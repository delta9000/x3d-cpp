// TextureTransformMatrix3D.hpp
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
 * @class TextureTransformMatrix3D
 * @brief TextureTransformMatrix3D applies a 3D transformation to texture
 * coordinates.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texture3D.html#TextureTransformMatrix3D
 */
class TextureTransformMatrix3D : public virtual X3DTextureTransformNode {
public:
  /**
   * @brief Default constructor for TextureTransformMatrix3D
   */
  TextureTransformMatrix3D() = default;

  /**
   * @brief Destructor for TextureTransformMatrix3D
   */
  ~TextureTransformMatrix3D() = default;

  /**
   * @brief Get the default value for matrix
   * @return SFMatrix4f The default value
   */
  static SFMatrix4f getDefaultMatrix() {
    return SFMatrix4f{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  }

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
  static std::string componentName() { return "Texturing3D"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of mapping. AccessType: inputOutput
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   * @return SFString The current value of mapping.
   */
  SFString getMapping() const { return _mapping; }

  /**
   * @brief Sets the value of mapping. AccessType: inputOutput
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   * @param value The new value for mapping.
   */
  void setMapping(const SFString &value) { _mapping = value; }

  void setMapping(SFString &&value) { _mapping = std::move(value); }

  /**
   * @brief Gets the value of matrix. AccessType: inputOutput
   * @details matrix is a generalized, unfiltered 4x4 transformation matrix to
   * modify texture (opposite effect appears on geometry).
   * @return SFMatrix4f The current value of matrix.
   */
  SFMatrix4f getMatrix() const { return _matrix; }

  /**
   * @brief Sets the value of matrix. AccessType: inputOutput
   * @details matrix is a generalized, unfiltered 4x4 transformation matrix to
   * modify texture (opposite effect appears on geometry).
   * @param value The new value for matrix.
   */
  void setMatrix(const SFMatrix4f &value) { _matrix = value; }

  void setMatrix(SFMatrix4f &&value) { _matrix = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "TextureTransformMatrix3D").
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
   * @brief Member variable for mapping.
   */

  SFString _mapping{};

  /**
   * @brief Member variable for matrix.
   */

  SFMatrix4f _matrix{
      SFMatrix4f{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}};
};

} // namespace x3d::nodes
