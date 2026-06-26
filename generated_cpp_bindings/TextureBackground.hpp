// TextureBackground.hpp
#ifndef TEXTUREBACKGROUND_HPP
#define TEXTUREBACKGROUND_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DBindableNode.hpp"

#include "X3DBackgroundNode.hpp"

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
  /**
   * @brief Default constructor for TextureBackground
   */
  TextureBackground() = default;

  /**
   * @brief Destructor for TextureBackground
   */
  ~TextureBackground() = default;

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
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "EnvironmentalEffects"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 3; }

  /**
   * @brief Gets the value of backTexture. AccessType: inputOutput
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   * @return SFNode The current value of backTexture.
   */
  SFNode getBackTexture() const { return _backTexture; }

  /**
   * @brief Acceptable node types for the backTexture field.
   * @details Permitted X3D node types: X3DTexture2DNode, MultiTexture
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableBackTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode",
                                                   "MultiTexture"};
    return types;
  }

  /**
   * @brief Sets the value of backTexture. AccessType: inputOutput
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   * @param value The new value for backTexture.
   */
  void setBackTexture(const SFNode &value) { _backTexture = value; }

  void setBackTexture(SFNode &&value) { _backTexture = std::move(value); }

  /**
   * @brief Gets the value of bottomTexture. AccessType: inputOutput
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   * @return SFNode The current value of bottomTexture.
   */
  SFNode getBottomTexture() const { return _bottomTexture; }

  /**
   * @brief Acceptable node types for the bottomTexture field.
   * @details Permitted X3D node types: X3DTexture2DNode, MultiTexture
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableBottomTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode",
                                                   "MultiTexture"};
    return types;
  }

  /**
   * @brief Sets the value of bottomTexture. AccessType: inputOutput
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   * @param value The new value for bottomTexture.
   */
  void setBottomTexture(const SFNode &value) { _bottomTexture = value; }

  void setBottomTexture(SFNode &&value) { _bottomTexture = std::move(value); }

  /**
   * @brief Gets the value of frontTexture. AccessType: inputOutput
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   * @return SFNode The current value of frontTexture.
   */
  SFNode getFrontTexture() const { return _frontTexture; }

  /**
   * @brief Acceptable node types for the frontTexture field.
   * @details Permitted X3D node types: X3DTexture2DNode, MultiTexture
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableFrontTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode",
                                                   "MultiTexture"};
    return types;
  }

  /**
   * @brief Sets the value of frontTexture. AccessType: inputOutput
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   * @param value The new value for frontTexture.
   */
  void setFrontTexture(const SFNode &value) { _frontTexture = value; }

  void setFrontTexture(SFNode &&value) { _frontTexture = std::move(value); }

  /**
   * @brief Gets the value of leftTexture. AccessType: inputOutput
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   * @return SFNode The current value of leftTexture.
   */
  SFNode getLeftTexture() const { return _leftTexture; }

  /**
   * @brief Acceptable node types for the leftTexture field.
   * @details Permitted X3D node types: X3DTexture2DNode, MultiTexture
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableLeftTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode",
                                                   "MultiTexture"};
    return types;
  }

  /**
   * @brief Sets the value of leftTexture. AccessType: inputOutput
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   * @param value The new value for leftTexture.
   */
  void setLeftTexture(const SFNode &value) { _leftTexture = value; }

  void setLeftTexture(SFNode &&value) { _leftTexture = std::move(value); }

  /**
   * @brief Gets the value of rightTexture. AccessType: inputOutput
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   * @return SFNode The current value of rightTexture.
   */
  SFNode getRightTexture() const { return _rightTexture; }

  /**
   * @brief Acceptable node types for the rightTexture field.
   * @details Permitted X3D node types: X3DTexture2DNode, MultiTexture
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableRightTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode",
                                                   "MultiTexture"};
    return types;
  }

  /**
   * @brief Sets the value of rightTexture. AccessType: inputOutput
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   * @param value The new value for rightTexture.
   */
  void setRightTexture(const SFNode &value) { _rightTexture = value; }

  void setRightTexture(SFNode &&value) { _rightTexture = std::move(value); }

  /**
   * @brief Gets the value of topTexture. AccessType: inputOutput
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   * @return SFNode The current value of topTexture.
   */
  SFNode getTopTexture() const { return _topTexture; }

  /**
   * @brief Acceptable node types for the topTexture field.
   * @details Permitted X3D node types: X3DTexture2DNode, MultiTexture
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableTopTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture2DNode",
                                                   "MultiTexture"};
    return types;
  }

  /**
   * @brief Sets the value of topTexture. AccessType: inputOutput
   * @details Parent TextureBackground element can contain up to six image nodes
   * (ImageTexture PixelTexture MovieTexture MultiTexture).
   * @param value The new value for topTexture.
   */
  void setTopTexture(const SFNode &value) { _topTexture = value; }

  void setTopTexture(SFNode &&value) { _topTexture = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "TextureBackground").
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
   * @brief Member variable for topTexture.
   */

  SFNode _topTexture{nullptr};
};

#endif // TEXTUREBACKGROUND_HPP