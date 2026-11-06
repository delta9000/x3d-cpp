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

#include "x3d/nodes/X3DEnvironmentTextureNode.hpp"

#include "x3d/nodes/X3DUrlObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ImageCubeMapTexture
 * @brief ImageCubeMapTexture is a texture node that defines a cubic environment
 * map source as a single file format that contains multiple images, one for
 * each side.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalTexturing.html#ImageCubeMapTexture
 */
class ImageCubeMapTexture : public virtual X3DEnvironmentTextureNode,
                            public virtual X3DUrlObject {
public:
  ImageCubeMapTexture() = default;

  ~ImageCubeMapTexture() = default;

  static SFNode getDefaultTextureProperties() { return nullptr; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DTexture2DNode";
  }

  static std::string getDefaultContainerField() { return "texture"; }

  static std::string componentName() { return "CubeMapTexturing"; }

  static int componentLevel() { return 2; }

  /**
   * SFNode; access: initializeOnly. Borrows the stored value.
   * @details Single contained TextureProperties node that can specify
   * additional visual attributes applied to corresponding texture images.
   */
  const SFNode &getTextureProperties() const { return _textureProperties; }

  /**
   * @details Permitted X3D node types: TextureProperties
   */
  static const std::vector<std::string> &
  acceptableTexturePropertiesNodeTypes() {
    static const std::vector<std::string> types = {"TextureProperties"};
    return types;
  }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setTexturePropertiesUnchecked(const SFNode &value) {
    _textureProperties = value;
  }

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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

private:
  SFNode _textureProperties{nullptr};
};

} // namespace x3d::nodes
