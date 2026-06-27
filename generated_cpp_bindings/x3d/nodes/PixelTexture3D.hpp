// PixelTexture3D.hpp
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
 * @class PixelTexture3D
 * @brief PixelTexture3D defines a 3D image-based texture map as an explicit
 * array of pixel values (image field).
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texture3D.html#PixelTexture3D
 */
class PixelTexture3D : public virtual X3DTexture3DNode {
public:
  /**
   * @brief Default constructor for PixelTexture3D
   */
  PixelTexture3D() = default;

  /**
   * @brief Destructor for PixelTexture3D
   */
  ~PixelTexture3D() = default;

  /**
   * @brief Get the default value for image
   * @return MFInt32 The default value
   */
  static MFInt32 getDefaultImage() { return std::vector<int>{0, 0, 0, 0}; }

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
   * @brief Gets the value of image. AccessType: inputOutput
   * @details image describes raw data for this 3D texture: number of components
   * to the image [0,4], width, height and depth of the texture, followed by
   * (width x height x depth) pixel values.
   * @return MFInt32 The current value of image.
   */
  MFInt32 getImage() const { return _image; }

  /**
   * @brief Sets the value of image. AccessType: inputOutput
   * @details image describes raw data for this 3D texture: number of components
   * to the image [0,4], width, height and depth of the texture, followed by
   * (width x height x depth) pixel values.
   * @param value The new value for image.
   */
  void setImage(const MFInt32 &value) { _image = value; }

  void setImage(MFInt32 &&value) { _image = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "PixelTexture3D").
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
   * @brief Member variable for image.
   */

  MFInt32 _image{std::vector<int>{0, 0, 0, 0}};
};

} // namespace x3d::nodes
