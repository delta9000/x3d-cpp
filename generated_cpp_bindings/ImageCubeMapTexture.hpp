// ImageCubeMapTexture.hpp
#ifndef IMAGECUBEMAPTEXTURE_HPP
#define IMAGECUBEMAPTEXTURE_HPP

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

#include "X3DEnvironmentTextureNode.hpp"

#include "X3DUrlObject.hpp"

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
  /**
   * @brief Default constructor for ImageCubeMapTexture
   */
  ImageCubeMapTexture() = default;

  /**
   * @brief Destructor for ImageCubeMapTexture
   */
  ~ImageCubeMapTexture() = default;

  /**
   * @brief Get the default value for textureProperties
   * @return SFNode The default value
   */
  static SFNode getDefaultTextureProperties() { return nullptr; }

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
  static std::string componentName() { return "CubeMapTexturing"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of textureProperties. AccessType: initializeOnly
   * @details Single contained TextureProperties node that can specify
   * additional visual attributes applied to corresponding texture images.
   * @return SFNode The current value of textureProperties.
   */
  SFNode getTextureProperties() const { return _textureProperties; }

  /**
   * @brief Acceptable node types for the textureProperties field.
   * @details Permitted X3D node types: TextureProperties
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &
  acceptableTexturePropertiesNodeTypes() {
    static const std::vector<std::string> types = {"TextureProperties"};
    return types;
  }
  /**
   * @brief Data-layer write of textureProperties (reader/init ingest path).
   * @details textureProperties is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setTextureProperties().
   */
  void setTexturePropertiesUnchecked(const SFNode &value) {
    _textureProperties = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "ImageCubeMapTexture").
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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

private:
  /**
   * @brief Member variable for textureProperties.
   */

  SFNode _textureProperties{nullptr};
};

#endif // IMAGECUBEMAPTEXTURE_HPP