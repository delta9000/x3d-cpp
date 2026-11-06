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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class GeneratedCubeMapTexture
 * @brief GeneratedCubeMapTexture is a texture node that defines a cubic
 * environment map that sources its data from internally generated images.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalTexturing.html#GeneratedCubeMapTexture
 */
class GeneratedCubeMapTexture : public virtual X3DEnvironmentTextureNode {
public:
  GeneratedCubeMapTexture() = default;

  ~GeneratedCubeMapTexture() = default;

  static SFInt32 getDefaultSize() { return 128; }

  static SFNode getDefaultTextureProperties() { return nullptr; }

  static GeneratedCubeMapTextureUpdateChoices getDefaultUpdate() {
    return GeneratedCubeMapTextureUpdateChoices::NONE;
  }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "texture"; }

  static std::string componentName() { return "CubeMapTexturing"; }

  static int componentLevel() { return 3; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details size indicates the resolution of the generated images in number of
   * pixels per side.
   */
  const SFInt32 &getSize() const { return _size; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setSizeUnchecked(const SFInt32 &value) { _size = value; }

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

  /**
   * GeneratedCubeMapTextureUpdateChoices; access: inputOutput. Borrows the
   * stored value.
   * @details update controls regeneration of the texture.
   */
  const GeneratedCubeMapTextureUpdateChoices &getUpdate() const {
    return _update;
  }

  /**
   * Access: inputOutput.
   * @details update controls regeneration of the texture.
   */
  void setUpdate(const GeneratedCubeMapTextureUpdateChoices &value) {

    _update = value;
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

private:
  SFInt32 _size{128};

  SFNode _textureProperties{nullptr};

  GeneratedCubeMapTextureUpdateChoices _update{
      GeneratedCubeMapTextureUpdateChoices::NONE};
};

} // namespace x3d::nodes
