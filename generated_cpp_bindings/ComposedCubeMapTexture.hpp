// ComposedCubeMapTexture.hpp
#ifndef COMPOSEDCUBEMAPTEXTURE_HPP
#define COMPOSEDCUBEMAPTEXTURE_HPP

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

/**
 * @class ComposedCubeMapTexture
 * @brief ComposedCubeMapTexture is a texture node that defines a cubic
 * environment map source as an explicit set of images drawn from individual 2D
 * texture nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalTexturing.html#ComposedCubeMapTexture
 */
class ComposedCubeMapTexture : public virtual X3DEnvironmentTextureNode {
public:
  /**
   * @brief Default constructor for ComposedCubeMapTexture
   */
  ComposedCubeMapTexture() = default;

  /**
   * @brief Destructor for ComposedCubeMapTexture
   */
  ~ComposedCubeMapTexture() = default;

  /**
   * @brief Get the default value for backTexture
   * @return SFNode The default value
   */
  static SFNode getDefaultBackTexture() { return nullptr; }

  /**
   * @brief Get the default value for bottomTexture
   * @return SFNode The default value
   */
  static SFNode getDefaultBottomTexture() { return nullptr; }

  /**
   * @brief Get the default value for frontTexture
   * @return SFNode The default value
   */
  static SFNode getDefaultFrontTexture() { return nullptr; }

  /**
   * @brief Get the default value for leftTexture
   * @return SFNode The default value
   */
  static SFNode getDefaultLeftTexture() { return nullptr; }

  /**
   * @brief Get the default value for rightTexture
   * @return SFNode The default value
   */
  static SFNode getDefaultRightTexture() { return nullptr; }

  /**
   * @brief Get the default value for textureProperties
   * @return SFNode The default value
   */
  static SFNode getDefaultTextureProperties() { return nullptr; }

  /**
   * @brief Get the default value for topTexture
   * @return SFNode The default value
   */
  static SFNode getDefaultTopTexture() { return nullptr; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

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
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of backTexture. AccessType: inputOutput
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodes).
   * @return SFNode The current value of backTexture.
   */
  SFNode getBackTexture() const { return _backTexture; }

  /**
   * @brief Acceptable node types for the backTexture field.
   * @details Permitted X3D node types: X3DTexture2DNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableBackTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * @brief Sets the value of backTexture. AccessType: inputOutput
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodes).
   * @param value The new value for backTexture.
   */
  void setBackTexture(const SFNode &value) { _backTexture = value; }

  void setBackTexture(SFNode &&value) { _backTexture = std::move(value); }

  /**
   * @brief Gets the value of bottomTexture. AccessType: inputOutput
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture, other texture nodes).
   * @return SFNode The current value of bottomTexture.
   */
  SFNode getBottomTexture() const { return _bottomTexture; }

  /**
   * @brief Acceptable node types for the bottomTexture field.
   * @details Permitted X3D node types: X3DTexture2DNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableBottomTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * @brief Sets the value of bottomTexture. AccessType: inputOutput
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture, other texture nodes).
   * @param value The new value for bottomTexture.
   */
  void setBottomTexture(const SFNode &value) { _bottomTexture = value; }

  void setBottomTexture(SFNode &&value) { _bottomTexture = std::move(value); }

  /**
   * @brief Gets the value of frontTexture. AccessType: inputOutput
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodes).
   * @return SFNode The current value of frontTexture.
   */
  SFNode getFrontTexture() const { return _frontTexture; }

  /**
   * @brief Acceptable node types for the frontTexture field.
   * @details Permitted X3D node types: X3DTexture2DNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableFrontTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * @brief Sets the value of frontTexture. AccessType: inputOutput
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodes).
   * @param value The new value for frontTexture.
   */
  void setFrontTexture(const SFNode &value) { _frontTexture = value; }

  void setFrontTexture(SFNode &&value) { _frontTexture = std::move(value); }

  /**
   * @brief Gets the value of leftTexture. AccessType: inputOutput
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodese).
   * @return SFNode The current value of leftTexture.
   */
  SFNode getLeftTexture() const { return _leftTexture; }

  /**
   * @brief Acceptable node types for the leftTexture field.
   * @details Permitted X3D node types: X3DTexture2DNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableLeftTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * @brief Sets the value of leftTexture. AccessType: inputOutput
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodese).
   * @param value The new value for leftTexture.
   */
  void setLeftTexture(const SFNode &value) { _leftTexture = value; }

  void setLeftTexture(SFNode &&value) { _leftTexture = std::move(value); }

  /**
   * @brief Gets the value of rightTexture. AccessType: inputOutput
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodes).
   * @return SFNode The current value of rightTexture.
   */
  SFNode getRightTexture() const { return _rightTexture; }

  /**
   * @brief Acceptable node types for the rightTexture field.
   * @details Permitted X3D node types: X3DTexture2DNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableRightTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * @brief Sets the value of rightTexture. AccessType: inputOutput
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodes).
   * @param value The new value for rightTexture.
   */
  void setRightTexture(const SFNode &value) { _rightTexture = value; }

  void setRightTexture(SFNode &&value) { _rightTexture = std::move(value); }

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
   * @brief Gets the value of topTexture. AccessType: inputOutput
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodes).
   * @return SFNode The current value of topTexture.
   */
  SFNode getTopTexture() const { return _topTexture; }

  /**
   * @brief Acceptable node types for the topTexture field.
   * @details Permitted X3D node types: X3DTexture2DNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableTopTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * @brief Sets the value of topTexture. AccessType: inputOutput
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodes).
   * @param value The new value for topTexture.
   */
  void setTopTexture(const SFNode &value) { _topTexture = value; }

  void setTopTexture(SFNode &&value) { _topTexture = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "ComposedCubeMapTexture").
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
   * @brief Member variable for backTexture.
   */

  SFNode _backTexture{nullptr};

  /**
   * @brief Member variable for bottomTexture.
   */

  SFNode _bottomTexture{nullptr};

  /**
   * @brief Member variable for frontTexture.
   */

  SFNode _frontTexture{nullptr};

  /**
   * @brief Member variable for leftTexture.
   */

  SFNode _leftTexture{nullptr};

  /**
   * @brief Member variable for rightTexture.
   */

  SFNode _rightTexture{nullptr};

  /**
   * @brief Member variable for textureProperties.
   */

  SFNode _textureProperties{nullptr};

  /**
   * @brief Member variable for topTexture.
   */

  SFNode _topTexture{nullptr};
};

#endif // COMPOSEDCUBEMAPTEXTURE_HPP