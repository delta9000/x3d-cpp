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
 * @class ComposedCubeMapTexture
 * @brief ComposedCubeMapTexture is a texture node that defines a cubic
 * environment map source as an explicit set of images drawn from individual 2D
 * texture nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalTexturing.html#ComposedCubeMapTexture
 */
class ComposedCubeMapTexture : public virtual X3DEnvironmentTextureNode {
public:
  ComposedCubeMapTexture() = default;

  ~ComposedCubeMapTexture() = default;

  static SFNode getDefaultBackTexture() { return nullptr; }

  static SFNode getDefaultBottomTexture() { return nullptr; }

  static SFNode getDefaultFrontTexture() { return nullptr; }

  static SFNode getDefaultLeftTexture() { return nullptr; }

  static SFNode getDefaultRightTexture() { return nullptr; }

  static SFNode getDefaultTextureProperties() { return nullptr; }

  static SFNode getDefaultTopTexture() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "texture"; }

  static std::string componentName() { return "CubeMapTexturing"; }

  static int componentLevel() { return 1; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodes).
   */
  const SFNode &getBackTexture() const { return _backTexture; }

  /**
   * @details Permitted X3D node types: X3DTexture2DNode
   */
  static const std::vector<std::string> &acceptableBackTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodes).
   */
  void setBackTexture(const SFNode &value) { _backTexture = value; }

  void setBackTexture(SFNode &&value) { _backTexture = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture, other texture nodes).
   */
  const SFNode &getBottomTexture() const { return _bottomTexture; }

  /**
   * @details Permitted X3D node types: X3DTexture2DNode
   */
  static const std::vector<std::string> &acceptableBottomTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture, other texture nodes).
   */
  void setBottomTexture(const SFNode &value) { _bottomTexture = value; }

  void setBottomTexture(SFNode &&value) { _bottomTexture = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodes).
   */
  const SFNode &getFrontTexture() const { return _frontTexture; }

  /**
   * @details Permitted X3D node types: X3DTexture2DNode
   */
  static const std::vector<std::string> &acceptableFrontTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodes).
   */
  void setFrontTexture(const SFNode &value) { _frontTexture = value; }

  void setFrontTexture(SFNode &&value) { _frontTexture = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodese).
   */
  const SFNode &getLeftTexture() const { return _leftTexture; }

  /**
   * @details Permitted X3D node types: X3DTexture2DNode
   */
  static const std::vector<std::string> &acceptableLeftTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodese).
   */
  void setLeftTexture(const SFNode &value) { _leftTexture = value; }

  void setLeftTexture(SFNode &&value) { _leftTexture = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodes).
   */
  const SFNode &getRightTexture() const { return _rightTexture; }

  /**
   * @details Permitted X3D node types: X3DTexture2DNode
   */
  static const std::vector<std::string> &acceptableRightTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodes).
   */
  void setRightTexture(const SFNode &value) { _rightTexture = value; }

  void setRightTexture(SFNode &&value) { _rightTexture = std::move(value); }

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
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodes).
   */
  const SFNode &getTopTexture() const { return _topTexture; }

  /**
   * @details Permitted X3D node types: X3DTexture2DNode
   */
  static const std::vector<std::string> &acceptableTopTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Parent ComposedCubeMapTexture element can contain up to six image
   * nodes (ImageTexture PixelTexture MovieTexture, other texture nodes).
   */
  void setTopTexture(const SFNode &value) { _topTexture = value; }

  void setTopTexture(SFNode &&value) { _topTexture = std::move(value); }

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
  SFNode _backTexture{nullptr};

  SFNode _bottomTexture{nullptr};

  SFNode _frontTexture{nullptr};

  SFNode _leftTexture{nullptr};

  SFNode _rightTexture{nullptr};

  SFNode _textureProperties{nullptr};

  SFNode _topTexture{nullptr};
};

} // namespace x3d::nodes
