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

#include "x3d/nodes/X3DSingleTextureNode.hpp"

#include "x3d/nodes/X3DTexture2DNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class PixelTexture
 * @brief PixelTexture creates a 2D-image texture map using a numeric array of
 * pixel values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/texturing.html#PixelTexture
 */
class PixelTexture : public virtual X3DTexture2DNode {
public:
  PixelTexture() = default;

  ~PixelTexture() = default;

  static SFImage getDefaultImage() {
    return SFImage{0, 0, 0, std::vector<unsigned char>{}};
  }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DTexture2DNode";
  }

  static std::string getDefaultContainerField() { return "texture"; }

  static std::string componentName() { return "Texturing"; }

  static int componentLevel() { return 1; }

  /**
   * SFImage; access: inputOutput. Borrows the stored value.
   * @details Defines image: width, height, number_of_components per each pixel
   * value, and list of pixel_values.
   */
  const SFImage &getImage() const { return _image; }

  /**
   * Access: inputOutput.
   * @details Defines image: width, height, number_of_components per each pixel
   * value, and list of pixel_values.
   */
  void setImage(const SFImage &value) { _image = value; }

  void setImage(SFImage &&value) { _image = std::move(value); }

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
  SFImage _image{SFImage{0, 0, 0, std::vector<unsigned char>{}}};
};

} // namespace x3d::nodes
