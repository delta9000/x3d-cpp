#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DChildNode.hpp"

#include "x3d/nodes/X3DBindableNode.hpp"

#include "x3d/nodes/X3DBackgroundNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class TextureBackground
 * @brief TextureBackground simulates ground and sky, using vertical arrays of
 * wraparound color values, TextureBackground can also provide backdrop texture
 * images on all six sides.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalEffects.html#TextureBackground
 */
class TextureBackground : public virtual X3DBackgroundNode {
public:
  TextureBackground() = default;

  ~TextureBackground() = default;

  static SFNode getDefaultBackTexture() { return nullptr; }

  static SFNode getDefaultBottomTexture() { return nullptr; }

  static SFNode getDefaultFrontTexture() { return nullptr; }

  static SFNode getDefaultLeftTexture() { return nullptr; }

  static SFNode getDefaultRightTexture() { return nullptr; }

  static SFNode getDefaultTopTexture() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "EnvironmentalEffects"; }

  static int componentLevel() { return 3; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   */
  const SFNode &getBackTexture() const { return _backTexture; }

  /**
   * @details Permitted X3D node types: X3DTexture2DNode, MultiTexture
   */
  static const std::vector<std::string> &acceptableBackTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode",
                                                   "MultiTexture"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   */
  void setBackTexture(const SFNode &value) { _backTexture = value; }

  void setBackTexture(SFNode &&value) { _backTexture = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   */
  const SFNode &getBottomTexture() const { return _bottomTexture; }

  /**
   * @details Permitted X3D node types: X3DTexture2DNode, MultiTexture
   */
  static const std::vector<std::string> &acceptableBottomTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode",
                                                   "MultiTexture"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   */
  void setBottomTexture(const SFNode &value) { _bottomTexture = value; }

  void setBottomTexture(SFNode &&value) { _bottomTexture = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   */
  const SFNode &getFrontTexture() const { return _frontTexture; }

  /**
   * @details Permitted X3D node types: X3DTexture2DNode, MultiTexture
   */
  static const std::vector<std::string> &acceptableFrontTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode",
                                                   "MultiTexture"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   */
  void setFrontTexture(const SFNode &value) { _frontTexture = value; }

  void setFrontTexture(SFNode &&value) { _frontTexture = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   */
  const SFNode &getLeftTexture() const { return _leftTexture; }

  /**
   * @details Permitted X3D node types: X3DTexture2DNode, MultiTexture
   */
  static const std::vector<std::string> &acceptableLeftTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode",
                                                   "MultiTexture"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   */
  void setLeftTexture(const SFNode &value) { _leftTexture = value; }

  void setLeftTexture(SFNode &&value) { _leftTexture = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   */
  const SFNode &getRightTexture() const { return _rightTexture; }

  /**
   * @details Permitted X3D node types: X3DTexture2DNode, MultiTexture
   */
  static const std::vector<std::string> &acceptableRightTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode",
                                                   "MultiTexture"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   */
  void setRightTexture(const SFNode &value) { _rightTexture = value; }

  void setRightTexture(SFNode &&value) { _rightTexture = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   */
  const SFNode &getTopTexture() const { return _topTexture; }

  /**
   * @details Permitted X3D node types: X3DTexture2DNode, MultiTexture
   */
  static const std::vector<std::string> &acceptableTopTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode",
                                                   "MultiTexture"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

private:
  SFNode _backTexture{nullptr};

  SFNode _bottomTexture{nullptr};

  SFNode _frontTexture{nullptr};

  SFNode _leftTexture{nullptr};

  SFNode _rightTexture{nullptr};

  SFNode _topTexture{nullptr};
};

} // namespace x3d::nodes
