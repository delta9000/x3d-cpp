// Appearance.hpp
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
  /**
   * @brief Default constructor for Appearance
   */
  Appearance() = default;

  /**
   * @brief Destructor for Appearance
   */
  ~Appearance() = default;

  /**
   * @brief Get the default value for acousticProperties
   * @return SFNode The default value
   */
  static SFNode getDefaultAcousticProperties() { return nullptr; }

  /**
   * @brief Get the default value for alphaCutoff
   * @return SFFloat The default value
   */
  static SFFloat getDefaultAlphaCutoff() { return 0.5; }

  /**
   * @brief Get the default value for alphaMode
   * @return AlphaModeChoices The default value
   */
  static AlphaModeChoices getDefaultAlphaMode() {
    return AlphaModeChoices::AUTO;
  }

  /**
   * @brief Get the default value for backMaterial
   * @return SFNode The default value
   */
  static SFNode getDefaultBackMaterial() { return nullptr; }

  /**
   * @brief Get the default value for fillProperties
   * @return SFNode The default value
   */
  static SFNode getDefaultFillProperties() { return nullptr; }

  /**
   * @brief Get the default value for lineProperties
   * @return SFNode The default value
   */
  static SFNode getDefaultLineProperties() { return nullptr; }

  /**
   * @brief Get the default value for material
   * @return SFNode The default value
   */
  static SFNode getDefaultMaterial() { return nullptr; }

  /**
   * @brief Get the default value for pointProperties
   * @return SFNode The default value
   */
  static SFNode getDefaultPointProperties() { return nullptr; }

  /**
   * @brief Get the default value for texture
   * @return SFNode The default value
   */
  static SFNode getDefaultTexture() { return nullptr; }

  /**
   * @brief Get the default value for textureTransform
   * @return SFNode The default value
   */
  static SFNode getDefaultTextureTransform() { return nullptr; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "appearance"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Shape"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of acousticProperties. AccessType: inputOutput
   * @details Single contained acousticProperties node that can specify
   * additional acoustic attributes applied to associated surface geometry.
   * @return SFNode The current value of acousticProperties.
   */
  SFNode getAcousticProperties() const { return _acousticProperties; }

  /**
   * @brief Acceptable node types for the acousticProperties field.
   * @details Permitted X3D node types: AcousticProperties
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &
  acceptableAcousticPropertiesNodeTypes() {
    static const std::vector<std::string> types = {"AcousticProperties"};
    return types;
  }

  /**
   * @brief Sets the value of acousticProperties. AccessType: inputOutput
   * @details Single contained acousticProperties node that can specify
   * additional acoustic attributes applied to associated surface geometry.
   * @param value The new value for acousticProperties.
   */
  void setAcousticProperties(const SFNode &value) {

    _acousticProperties = value;
  }

  void setAcousticProperties(SFNode &&value) {

    _acousticProperties = std::move(value);
  }

  /**
   * @brief Gets the value of alphaCutoff. AccessType: inputOutput
   * @details Threshold value used for pixel rendering either transparent or
   * opaque, used when alphaMode="MASK".
   * @return SFFloat The current value of alphaCutoff.
   */
  SFFloat getAlphaCutoff() const { return _alphaCutoff; }

  /**
   * @brief Sets the value of alphaCutoff. AccessType: inputOutput
   * @details Threshold value used for pixel rendering either transparent or
   * opaque, used when alphaMode="MASK".
   * @param value The new value for alphaCutoff.
   */
  void setAlphaCutoff(const SFFloat &value) {

    validateAlphaCutoff(value);

    _alphaCutoff = value;
  }

  /**
   * @brief Non-validating write of alphaCutoff (runtime/reader ingest path).
   * @details Assigns without the range check setAlphaCutoff() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setAlphaCutoff() stays the
   *          enforcement point for programmatic callers.
   */
  void setAlphaCutoffUnchecked(const SFFloat &value) { _alphaCutoff = value; }

  /**
   * @brief Gets the value of alphaMode. AccessType: inputOutput
   * @details Provides options for control of alpha transparency handling for
   * textures.
   * @return AlphaModeChoices The current value of alphaMode.
   */
  AlphaModeChoices getAlphaMode() const { return _alphaMode; }

  /**
   * @brief Sets the value of alphaMode. AccessType: inputOutput
   * @details Provides options for control of alpha transparency handling for
   * textures.
   * @param value The new value for alphaMode.
   */
  void setAlphaMode(const AlphaModeChoices &value) { _alphaMode = value; }

  /**
   * @brief Gets the value of backMaterial. AccessType: inputOutput
   * @details
   * @return SFNode The current value of backMaterial.
   */
  SFNode getBackMaterial() const { return _backMaterial; }

  /**
   * @brief Acceptable node types for the backMaterial field.
   * @details Permitted X3D node types: X3DMaterialNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableBackMaterialNodeTypes() {
    static const std::vector<std::string> types = {"X3DMaterialNode"};
    return types;
  }

  /**
   * @brief Sets the value of backMaterial. AccessType: inputOutput
   * @details
   * @param value The new value for backMaterial.
   */
  void setBackMaterial(const SFNode &value) { _backMaterial = value; }

  void setBackMaterial(SFNode &&value) { _backMaterial = std::move(value); }

  /**
   * @brief Gets the value of fillProperties. AccessType: inputOutput
   * @details Single contained FillProperties node that can specify additional
   * visual attributes applied to polygonal areas of corresponding geometry, on
   * top of whatever other appearance is already defined.
   * @return SFNode The current value of fillProperties.
   */
  SFNode getFillProperties() const { return _fillProperties; }

  /**
   * @brief Acceptable node types for the fillProperties field.
   * @details Permitted X3D node types: FillProperties
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableFillPropertiesNodeTypes() {
    static const std::vector<std::string> types = {"FillProperties"};
    return types;
  }

  /**
   * @brief Sets the value of fillProperties. AccessType: inputOutput
   * @details Single contained FillProperties node that can specify additional
   * visual attributes applied to polygonal areas of corresponding geometry, on
   * top of whatever other appearance is already defined.
   * @param value The new value for fillProperties.
   */
  void setFillProperties(const SFNode &value) { _fillProperties = value; }

  void setFillProperties(SFNode &&value) { _fillProperties = std::move(value); }

  /**
   * @brief Gets the value of lineProperties. AccessType: inputOutput
   * @details Single contained LineProperties node that can specify additional
   * visual attributes applied to corresponding line geometry.
   * @return SFNode The current value of lineProperties.
   */
  SFNode getLineProperties() const { return _lineProperties; }

  /**
   * @brief Acceptable node types for the lineProperties field.
   * @details Permitted X3D node types: LineProperties
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableLinePropertiesNodeTypes() {
    static const std::vector<std::string> types = {"LineProperties"};
    return types;
  }

  /**
   * @brief Sets the value of lineProperties. AccessType: inputOutput
   * @details Single contained LineProperties node that can specify additional
   * visual attributes applied to corresponding line geometry.
   * @param value The new value for lineProperties.
   */
  void setLineProperties(const SFNode &value) { _lineProperties = value; }

  void setLineProperties(SFNode &&value) { _lineProperties = std::move(value); }

  /**
   * @brief Gets the value of material. AccessType: inputOutput
   * @details Single contained Material node that can specify visual attributes
   * for lighting response (color types, transparency, etc.
   * @return SFNode The current value of material.
   */
  SFNode getMaterial() const { return _material; }

  /**
   * @brief Acceptable node types for the material field.
   * @details Permitted X3D node types: X3DMaterialNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableMaterialNodeTypes() {
    static const std::vector<std::string> types = {"X3DMaterialNode"};
    return types;
  }

  /**
   * @brief Sets the value of material. AccessType: inputOutput
   * @details Single contained Material node that can specify visual attributes
   * for lighting response (color types, transparency, etc.
   * @param value The new value for material.
   */
  void setMaterial(const SFNode &value) { _material = value; }

  void setMaterial(SFNode &&value) { _material = std::move(value); }

  /**
   * @brief Gets the value of pointProperties. AccessType: inputOutput
   * @details Single contained PointProperties node that can specify additional
   * visual attributes applied to corresponding point geometry.
   * @return SFNode The current value of pointProperties.
   */
  SFNode getPointProperties() const { return _pointProperties; }

  /**
   * @brief Acceptable node types for the pointProperties field.
   * @details Permitted X3D node types: PointProperties
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptablePointPropertiesNodeTypes() {
    static const std::vector<std::string> types = {"PointProperties"};
    return types;
  }

  /**
   * @brief Sets the value of pointProperties. AccessType: inputOutput
   * @details Single contained PointProperties node that can specify additional
   * visual attributes applied to corresponding point geometry.
   * @param value The new value for pointProperties.
   */
  void setPointProperties(const SFNode &value) { _pointProperties = value; }

  void setPointProperties(SFNode &&value) {

    _pointProperties = std::move(value);
  }

  /**
   * @brief Gets the value of shaders. AccessType: inputOutput
   * @details Zero or more contained programmable shader nodes (ComposedShader,
   * PackagedShader, ProgramShader) that specify, in order of preference,
   * author-programmed rendering characteristics.
   * @return const MFNode& The current value of shaders.
   */
  const MFNode &getShaders() const { return _shaders; }

  /**
   * @brief Acceptable node types for the shaders field.
   * @details Permitted X3D node types: X3DShaderNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableShadersNodeTypes() {
    static const std::vector<std::string> types = {"X3DShaderNode"};
    return types;
  }

  /**
   * @brief Sets the value of shaders. AccessType: inputOutput
   * @details Zero or more contained programmable shader nodes (ComposedShader,
   * PackagedShader, ProgramShader) that specify, in order of preference,
   * author-programmed rendering characteristics.
   * @param value The new value for shaders.
   */
  void setShaders(const MFNode &value) { _shaders = value; }

  void setShaders(MFNode &&value) { _shaders = std::move(value); }

  /**
   * @brief Gets the value of texture. AccessType: inputOutput
   * @details Single contained texture node (ImageTexture, MovieTexture,
   * PixelTexture, MultiTexture) that maps image(s) to surface geometry.
   * @return SFNode The current value of texture.
   */
  SFNode getTexture() const { return _texture; }

  /**
   * @brief Acceptable node types for the texture field.
   * @details Permitted X3D node types: X3DTextureNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DTextureNode"};
    return types;
  }

  /**
   * @brief Sets the value of texture. AccessType: inputOutput
   * @details Single contained texture node (ImageTexture, MovieTexture,
   * PixelTexture, MultiTexture) that maps image(s) to surface geometry.
   * @param value The new value for texture.
   */
  void setTexture(const SFNode &value) { _texture = value; }

  void setTexture(SFNode &&value) { _texture = std::move(value); }

  /**
   * @brief Gets the value of textureTransform. AccessType: inputOutput
   * @details Single contained TextureTransform node that defines 2D
   * transformation applied to texture coordinates.
   * @return SFNode The current value of textureTransform.
   */
  SFNode getTextureTransform() const { return _textureTransform; }

  /**
   * @brief Acceptable node types for the textureTransform field.
   * @details Permitted X3D node types: X3DTextureTransformNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableTextureTransformNodeTypes() {
    static const std::vector<std::string> types = {"X3DTextureTransformNode"};
    return types;
  }

  /**
   * @brief Sets the value of textureTransform. AccessType: inputOutput
   * @details Single contained TextureTransform node that defines 2D
   * transformation applied to texture coordinates.
   * @param value The new value for textureTransform.
   */
  void setTextureTransform(const SFNode &value) { _textureTransform = value; }

  void setTextureTransform(SFNode &&value) {

    _textureTransform = std::move(value);
  }

  /**
   * @brief The X3D type name of this node (e.g. "Appearance").
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

  /**
   * @brief Member variable for acousticProperties.
   */

  SFNode _acousticProperties{nullptr};

  /**
   * @brief Member variable for alphaCutoff.
   */

  SFFloat _alphaCutoff{0.5};

  /**
   * @brief Member variable for alphaMode.
   */

  AlphaModeChoices _alphaMode{AlphaModeChoices::AUTO};

  /**
   * @brief Member variable for backMaterial.
   */

  SFNode _backMaterial{nullptr};

  /**
   * @brief Member variable for fillProperties.
   */

  SFNode _fillProperties{nullptr};

  /**
   * @brief Member variable for lineProperties.
   */

  SFNode _lineProperties{nullptr};

  /**
   * @brief Member variable for material.
   */

  SFNode _material{nullptr};

  /**
   * @brief Member variable for pointProperties.
   */

  SFNode _pointProperties{nullptr};

  /**
   * @brief Member variable for shaders.
   */

  MFNode _shaders{};

  /**
   * @brief Member variable for texture.
   */

  SFNode _texture{nullptr};

  /**
   * @brief Member variable for textureTransform.
   */

  SFNode _textureTransform{nullptr};
};

} // namespace x3d::nodes
