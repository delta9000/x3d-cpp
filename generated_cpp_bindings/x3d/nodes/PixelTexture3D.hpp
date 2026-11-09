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
  PixelTexture3D() = default;

  ~PixelTexture3D() = default;

  static MFInt32 getDefaultImage() { return std::vector<int>{0, 0, 0, 0}; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DTexture3DNode";
  }

  static std::string getDefaultContainerField() { return "texture"; }

  static std::string componentName() { return "Texturing3D"; }

  static int componentLevel() { return 1; }

  /**
   * MFInt32; access: inputOutput. Borrows the stored value.
   * @details image describes raw data for this 3D texture: number of components
   * to the image [0,4], width, height and depth of the texture, followed by
   * (width x height x depth) pixel values.
   */
  const MFInt32 &getImage() const { return _image; }

  /**
   * Access: inputOutput.
   * @details image describes raw data for this 3D texture: number of components
   * to the image [0,4], width, height and depth of the texture, followed by
   * (width x height x depth) pixel values.
   */
  void setImage(const MFInt32 &value) { _image = value; }

  void setImage(MFInt32 &&value) { _image = std::move(value); }

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
  MFInt32 _image{std::vector<int>{0, 0, 0, 0}};
};

} // namespace x3d::nodes
