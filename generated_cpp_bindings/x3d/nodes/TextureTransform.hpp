// TextureTransform.hpp
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
 * @class TextureTransform
 * @brief TextureTransform shifts 2D texture coordinates for positioning,
 * orienting and scaling image textures on geometry.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texturing.html#TextureTransform
 */
class TextureTransform : public virtual X3DTextureTransformNode {
public:
  /**
   * @brief Default constructor for TextureTransform
   */
  TextureTransform() = default;

  /**
   * @brief Destructor for TextureTransform
   */
  ~TextureTransform() = default;

  /**
   * @brief Get the default value for center
   * @return SFVec2f The default value
   */
  static SFVec2f getDefaultCenter() { return SFVec2f{0, 0}; }

  /**
   * @brief Get the default value for rotation
   * @return SFFloat The default value
   */
  static SFFloat getDefaultRotation() { return 0; }

  /**
   * @brief Get the default value for scale
   * @return SFVec2f The default value
   */
  static SFVec2f getDefaultScale() { return SFVec2f{1, 1}; }

  /**
   * @brief Get the default value for translation
   * @return SFVec2f The default value
   */
  static SFVec2f getDefaultTranslation() { return SFVec2f{0, 0}; }

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
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of center. AccessType: inputOutput
   * @details center point in 2D (s,t) texture coordinates for rotation and
   * scaling.
   * @return SFVec2f The current value of center.
   */
  SFVec2f getCenter() const { return _center; }

  /**
   * @brief Sets the value of center. AccessType: inputOutput
   * @details center point in 2D (s,t) texture coordinates for rotation and
   * scaling.
   * @param value The new value for center.
   */
  void setCenter(const SFVec2f &value) { _center = value; }

  void setCenter(SFVec2f &&value) { _center = std::move(value); }

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
   * @brief Gets the value of rotation. AccessType: inputOutput
   * @details single rotation angle of texture about center (opposite effect
   * appears on geometry).
   * @return SFFloat The current value of rotation.
   */
  SFFloat getRotation() const { return _rotation; }

  /**
   * @brief Sets the value of rotation. AccessType: inputOutput
   * @details single rotation angle of texture about center (opposite effect
   * appears on geometry).
   * @param value The new value for rotation.
   */
  void setRotation(const SFFloat &value) { _rotation = value; }

  /**
   * @brief Gets the value of scale. AccessType: inputOutput
   * @details Non-uniform planar scaling of texture about center (opposite
   * effect appears on geometry).
   * @return SFVec2f The current value of scale.
   */
  SFVec2f getScale() const { return _scale; }

  /**
   * @brief Sets the value of scale. AccessType: inputOutput
   * @details Non-uniform planar scaling of texture about center (opposite
   * effect appears on geometry).
   * @param value The new value for scale.
   */
  void setScale(const SFVec2f &value) { _scale = value; }

  void setScale(SFVec2f &&value) { _scale = std::move(value); }

  /**
   * @brief Gets the value of translation. AccessType: inputOutput
   * @details Lateral/vertical shift in 2D (s,t) texture coordinates (opposite
   * effect appears on geometry).
   * @return SFVec2f The current value of translation.
   */
  SFVec2f getTranslation() const { return _translation; }

  /**
   * @brief Sets the value of translation. AccessType: inputOutput
   * @details Lateral/vertical shift in 2D (s,t) texture coordinates (opposite
   * effect appears on geometry).
   * @param value The new value for translation.
   */
  void setTranslation(const SFVec2f &value) { _translation = value; }

  void setTranslation(SFVec2f &&value) { _translation = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "TextureTransform").
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
   * @brief Member variable for center.
   */

  SFVec2f _center{SFVec2f{0, 0}};

  /**
   * @brief Member variable for mapping.
   */

  SFString _mapping{};

  /**
   * @brief Member variable for rotation.
   */

  SFFloat _rotation{0};

  /**
   * @brief Member variable for scale.
   */

  SFVec2f _scale{SFVec2f{1, 1}};

  /**
   * @brief Member variable for translation.
   */

  SFVec2f _translation{SFVec2f{0, 0}};
};

} // namespace x3d::nodes
