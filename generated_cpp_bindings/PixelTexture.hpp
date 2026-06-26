// PixelTexture.hpp
#ifndef PIXELTEXTURE_HPP
#define PIXELTEXTURE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DAppearanceChildNode.hpp"

#include "X3DTextureNode.hpp"

#include "X3DSingleTextureNode.hpp"

#include "X3DTexture2DNode.hpp"

/**
 * @class PixelTexture
 * @brief PixelTexture creates a 2D-image texture map using a numeric array of
 * pixel values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texturing.html#PixelTexture
 */
class PixelTexture : public virtual X3DTexture2DNode {
public:
  /**
   * @brief Default constructor for PixelTexture
   */
  PixelTexture() = default;

  /**
   * @brief Destructor for PixelTexture
   */
  ~PixelTexture() = default;

  /**
   * @brief Get the default value for image
   * @return SFImage The default value
   */
  static SFImage getDefaultImage() {
    return SFImage{0, 0, 0, std::vector<unsigned char>{}};
  }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DTexture2DNode";
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
  static std::string componentName() { return "Texturing"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of image. AccessType: inputOutput
   * @details Defines image: width, height, number_of_components per each pixel
   * value, and list of pixel_values.
   * @return SFImage The current value of image.
   */
  SFImage getImage() const { return _image; }

  /**
   * @brief Sets the value of image. AccessType: inputOutput
   * @details Defines image: width, height, number_of_components per each pixel
   * value, and list of pixel_values.
   * @param value The new value for image.
   */
  void setImage(const SFImage &value) { _image = value; }

  void setImage(SFImage &&value) { _image = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "PixelTexture").
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

  SFImage _image{SFImage{0, 0, 0, std::vector<unsigned char>{}}};
};

#endif // PIXELTEXTURE_HPP