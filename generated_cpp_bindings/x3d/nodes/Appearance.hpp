#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DAppearanceNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Appearance
 * @brief Appearance specifies the visual properties of geometry by containing
 * the Material, ImageTexture/MovieTexture/PixelTexture, FillProperties,
 * LineProperties, programmable shader nodes (ComposedShader, PackagedShader,
 * ProgramShader) and TextureTransform nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shape.html#Appearance
 */
class Appearance : public virtual X3DAppearanceNode {
public:
  Appearance() = default;

  ~Appearance() = default;

  static SFNode getDefaultAcousticProperties() { return nullptr; }

  static SFFloat getDefaultAlphaCutoff() { return 0.5; }

  static AlphaModeChoices getDefaultAlphaMode() {
    return AlphaModeChoices::AUTO;
  }

  static SFNode getDefaultBackMaterial() { return nullptr; }

  static SFNode getDefaultFillProperties() { return nullptr; }

  static SFNode getDefaultLineProperties() { return nullptr; }

  static SFNode getDefaultMaterial() { return nullptr; }

  static SFNode getDefaultPointProperties() { return nullptr; }

  static SFNode getDefaultTexture() { return nullptr; }

  static SFNode getDefaultTextureTransform() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "appearance"; }

  static std::string componentName() { return "Shape"; }

  static int componentLevel() { return 1; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained acousticProperties node that can specify
   * additional acoustic attributes applied to associated surface geometry.
   */
  const SFNode &getAcousticProperties() const { return _acousticProperties; }

  /**
   * @details Permitted X3D node types: AcousticProperties
   */
  static const std::vector<std::string> &
  acceptableAcousticPropertiesNodeTypes() {
    static const std::vector<std::string> types = {"AcousticProperties"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained acousticProperties node that can specify
   * additional acoustic attributes applied to associated surface geometry.
   */
  void setAcousticProperties(const SFNode &value) {

    _acousticProperties = value;
  }

  void setAcousticProperties(SFNode &&value) {

    _acousticProperties = std::move(value);
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Threshold value used for pixel rendering either transparent or
   * opaque, used when alphaMode="MASK".
   */
  const SFFloat &getAlphaCutoff() const { return _alphaCutoff; }

  /**
   * Access: inputOutput.
   * @details Threshold value used for pixel rendering either transparent or
   * opaque, used when alphaMode="MASK".
   */
  void setAlphaCutoff(const SFFloat &value) {

    validateAlphaCutoff(value);

    _alphaCutoff = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setAlphaCutoff() for range validation.
   */
  void setAlphaCutoffUnchecked(const SFFloat &value) { _alphaCutoff = value; }

  /**
   * AlphaModeChoices; access: inputOutput. Borrows the stored value.
   * @details Provides options for control of alpha transparency handling for
   * textures.
   */
  const AlphaModeChoices &getAlphaMode() const { return _alphaMode; }

  /**
   * Access: inputOutput.
   * @details Provides options for control of alpha transparency handling for
   * textures.
   */
  void setAlphaMode(const AlphaModeChoices &value) { _alphaMode = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFNode &getBackMaterial() const { return _backMaterial; }

  /**
   * @details Permitted X3D node types: X3DMaterialNode
   */
  static const std::vector<std::string> &acceptableBackMaterialNodeTypes() {
    static const std::vector<std::string> types = {"X3DMaterialNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details
   */
  void setBackMaterial(const SFNode &value) { _backMaterial = value; }

  void setBackMaterial(SFNode &&value) { _backMaterial = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained FillProperties node that can specify additional
   * visual attributes applied to polygonal areas of corresponding geometry, on
   * top of whatever other appearance is already defined.
   */
  const SFNode &getFillProperties() const { return _fillProperties; }

  /**
   * @details Permitted X3D node types: FillProperties
   */
  static const std::vector<std::string> &acceptableFillPropertiesNodeTypes() {
    static const std::vector<std::string> types = {"FillProperties"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained FillProperties node that can specify additional
   * visual attributes applied to polygonal areas of corresponding geometry, on
   * top of whatever other appearance is already defined.
   */
  void setFillProperties(const SFNode &value) { _fillProperties = value; }

  void setFillProperties(SFNode &&value) { _fillProperties = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained LineProperties node that can specify additional
   * visual attributes applied to corresponding line geometry.
   */
  const SFNode &getLineProperties() const { return _lineProperties; }

  /**
   * @details Permitted X3D node types: LineProperties
   */
  static const std::vector<std::string> &acceptableLinePropertiesNodeTypes() {
    static const std::vector<std::string> types = {"LineProperties"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained LineProperties node that can specify additional
   * visual attributes applied to corresponding line geometry.
   */
  void setLineProperties(const SFNode &value) { _lineProperties = value; }

  void setLineProperties(SFNode &&value) { _lineProperties = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained Material node that can specify visual attributes
   * for lighting response (color types, transparency, etc.
   */
  const SFNode &getMaterial() const { return _material; }

  /**
   * @details Permitted X3D node types: X3DMaterialNode
   */
  static const std::vector<std::string> &acceptableMaterialNodeTypes() {
    static const std::vector<std::string> types = {"X3DMaterialNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained Material node that can specify visual attributes
   * for lighting response (color types, transparency, etc.
   */
  void setMaterial(const SFNode &value) { _material = value; }

  void setMaterial(SFNode &&value) { _material = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained PointProperties node that can specify additional
   * visual attributes applied to corresponding point geometry.
   */
  const SFNode &getPointProperties() const { return _pointProperties; }

  /**
   * @details Permitted X3D node types: PointProperties
   */
  static const std::vector<std::string> &acceptablePointPropertiesNodeTypes() {
    static const std::vector<std::string> types = {"PointProperties"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained PointProperties node that can specify additional
   * visual attributes applied to corresponding point geometry.
   */
  void setPointProperties(const SFNode &value) { _pointProperties = value; }

  void setPointProperties(SFNode &&value) {

    _pointProperties = std::move(value);
  }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details Zero or more contained programmable shader nodes (ComposedShader,
   * PackagedShader, ProgramShader) that specify, in order of preference,
   * author-programmed rendering characteristics.
   */
  const MFNode &getShaders() const { return _shaders; }

  /**
   * @details Permitted X3D node types: X3DShaderNode
   */
  static const std::vector<std::string> &acceptableShadersNodeTypes() {
    static const std::vector<std::string> types = {"X3DShaderNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Zero or more contained programmable shader nodes (ComposedShader,
   * PackagedShader, ProgramShader) that specify, in order of preference,
   * author-programmed rendering characteristics.
   */
  void setShaders(const MFNode &value) { _shaders = value; }

  void setShaders(MFNode &&value) { _shaders = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained texture node (ImageTexture, MovieTexture,
   * PixelTexture, MultiTexture) that maps image(s) to surface geometry.
   */
  const SFNode &getTexture() const { return _texture; }

  /**
   * @details Permitted X3D node types: X3DTextureNode
   */
  static const std::vector<std::string> &acceptableTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTextureNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained texture node (ImageTexture, MovieTexture,
   * PixelTexture, MultiTexture) that maps image(s) to surface geometry.
   */
  void setTexture(const SFNode &value) { _texture = value; }

  void setTexture(SFNode &&value) { _texture = std::move(value); }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained TextureTransform node that defines 2D
   * transformation applied to texture coordinates.
   */
  const SFNode &getTextureTransform() const { return _textureTransform; }

  /**
   * @details Permitted X3D node types: X3DTextureTransformNode
   */
  static const std::vector<std::string> &acceptableTextureTransformNodeTypes() {
    static const std::vector<std::string> types = {"X3DTextureTransformNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained TextureTransform node that defines 2D
   * transformation applied to texture coordinates.
   */
  void setTextureTransform(const SFNode &value) { _textureTransform = value; }

  void setTextureTransform(SFNode &&value) {

    _textureTransform = std::move(value);
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

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesAlphaCutoff(const SFFloat &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

private:
  static void validateAlphaCutoff(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("alphaCutoff below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("alphaCutoff above maximum of 1");
  }

  SFNode _acousticProperties{nullptr};

  SFFloat _alphaCutoff{0.5};

  AlphaModeChoices _alphaMode{AlphaModeChoices::AUTO};

  SFNode _backMaterial{nullptr};

  SFNode _fillProperties{nullptr};

  SFNode _lineProperties{nullptr};

  SFNode _material{nullptr};

  SFNode _pointProperties{nullptr};

  MFNode _shaders{};

  SFNode _texture{nullptr};

  SFNode _textureTransform{nullptr};
};

} // namespace x3d::nodes
