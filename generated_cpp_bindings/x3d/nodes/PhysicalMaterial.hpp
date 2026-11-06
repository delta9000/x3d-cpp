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

#include "x3d/nodes/X3DMaterialNode.hpp"

#include "x3d/nodes/X3DOneSidedMaterialNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class PhysicalMaterial
 * @brief PhysicalMaterial specifies surface rendering properties for associated
 * geometry nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shape.html#PhysicalMaterial
 */
class PhysicalMaterial : public virtual X3DOneSidedMaterialNode {
public:
  PhysicalMaterial() = default;

  ~PhysicalMaterial() = default;

  static SFColor getDefaultBaseColor() { return SFColor{1, 1, 1}; }

  static SFNode getDefaultBaseTexture() { return nullptr; }

  static SFColor getDefaultEmissiveColor() { return SFColor{0, 0, 0}; }

  static SFNode getDefaultEmissiveTexture() { return nullptr; }

  static SFFloat getDefaultMetallic() { return 1; }

  static SFNode getDefaultMetallicRoughnessTexture() { return nullptr; }

  static SFNode getDefaultNormalTexture() { return nullptr; }

  static SFFloat getDefaultOcclusionStrength() { return 1; }

  static SFNode getDefaultOcclusionTexture() { return nullptr; }

  static SFFloat getDefaultRoughness() { return 1; }

  static SFFloat getDefaultTransparency() { return 0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "material"; }

  static std::string componentName() { return "Shape"; }

  static int componentLevel() { return 2; }

  /**
   * SFColor; access: inputOutput. Borrows the stored value.
   * @details similar to diffuseColor, TODO define more precisely.
   */
  const SFColor &getBaseColor() const { return _baseColor; }

  /**
   * Access: inputOutput.
   * @details similar to diffuseColor, TODO define more precisely.
   */
  void setBaseColor(const SFColor &value) {

    validateBaseColor(value);

    _baseColor = value;
  }

  void setBaseColor(SFColor &&value) {

    validateBaseColor(value);

    _baseColor = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setBaseColor() for range validation.
   */
  void setBaseColorUnchecked(const SFColor &value) { _baseColor = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details When applying baseColor for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   */
  const SFNode &getBaseTexture() const { return _baseTexture; }

  /**
   * @details Permitted X3D node types: X3DSingleTextureNode
   */
  static const std::vector<std::string> &acceptableBaseTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details When applying baseColor for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   */
  void setBaseTexture(const SFNode &value) { _baseTexture = value; }

  void setBaseTexture(SFNode &&value) { _baseTexture = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   */
  const SFString &getBaseTextureMapping() const { return _baseTextureMapping; }

  /**
   * Access: inputOutput.
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   */
  void setBaseTextureMapping(const SFString &value) {

    _baseTextureMapping = value;
  }

  void setBaseTextureMapping(SFString &&value) {

    _baseTextureMapping = std::move(value);
  }

  /**
   * SFColor; access: inputOutput. Borrows the stored value.
   * @details how much glowing light is emitted from this object.
   */
  const SFColor &getEmissiveColor() const { return _emissiveColor; }

  /**
   * Access: inputOutput.
   * @details how much glowing light is emitted from this object.
   */
  void setEmissiveColor(const SFColor &value) {

    validateEmissiveColor(value);

    _emissiveColor = value;
  }

  void setEmissiveColor(SFColor &&value) {

    validateEmissiveColor(value);

    _emissiveColor = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setEmissiveColor() for range validation.
   */
  void setEmissiveColorUnchecked(const SFColor &value) {
    _emissiveColor = value;
  }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details When applying emissiveColor for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   */
  const SFNode &getEmissiveTexture() const { return _emissiveTexture; }

  /**
   * @details Permitted X3D node types: X3DSingleTextureNode
   */
  static const std::vector<std::string> &acceptableEmissiveTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details When applying emissiveColor for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   */
  void setEmissiveTexture(const SFNode &value) { _emissiveTexture = value; }

  void setEmissiveTexture(SFNode &&value) {

    _emissiveTexture = std::move(value);
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details metallic is a PBR parameter (TODO elaborate).
   */
  const SFFloat &getMetallic() const { return _metallic; }

  /**
   * Access: inputOutput.
   * @details metallic is a PBR parameter (TODO elaborate).
   */
  void setMetallic(const SFFloat &value) {

    validateMetallic(value);

    _metallic = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setMetallic() for range validation.
   */
  void setMetallicUnchecked(const SFFloat &value) { _metallic = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details When applying metallic for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   */
  const SFNode &getMetallicRoughnessTexture() const {
    return _metallicRoughnessTexture;
  }

  /**
   * @details Permitted X3D node types: X3DSingleTextureNode
   */
  static const std::vector<std::string> &
  acceptableMetallicRoughnessTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details When applying metallic for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   */
  void setMetallicRoughnessTexture(const SFNode &value) {

    _metallicRoughnessTexture = value;
  }

  void setMetallicRoughnessTexture(SFNode &&value) {

    _metallicRoughnessTexture = std::move(value);
  }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   */
  const SFString &getMetallicRoughnessTextureMapping() const {
    return _metallicRoughnessTextureMapping;
  }

  /**
   * Access: inputOutput.
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   */
  void setMetallicRoughnessTextureMapping(const SFString &value) {

    _metallicRoughnessTextureMapping = value;
  }

  void setMetallicRoughnessTextureMapping(SFString &&value) {

    _metallicRoughnessTextureMapping = std::move(value);
  }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details When applying normalScale for this material node, the contained
   * texture modulates the texture across the surface.
   */
  const SFNode &getNormalTexture() const { return _normalTexture; }

  /**
   * @details Permitted X3D node types: X3DSingleTextureNode
   */
  static const std::vector<std::string> &acceptableNormalTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details When applying normalScale for this material node, the contained
   * texture modulates the texture across the surface.
   */
  void setNormalTexture(const SFNode &value) { _normalTexture = value; }

  void setNormalTexture(SFNode &&value) { _normalTexture = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details occlusionStrength indicates areas of indirect lighting, typically
   * called ambient occlusion.
   */
  const SFFloat &getOcclusionStrength() const { return _occlusionStrength; }

  /**
   * Access: inputOutput.
   * @details occlusionStrength indicates areas of indirect lighting, typically
   * called ambient occlusion.
   */
  void setOcclusionStrength(const SFFloat &value) {

    validateOcclusionStrength(value);

    _occlusionStrength = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setOcclusionStrength() for range validation.
   */
  void setOcclusionStrengthUnchecked(const SFFloat &value) {
    _occlusionStrength = value;
  }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details When applying occlusionStrength for this material node, the
   * contained texture provides Physically Based Rendering (PBR) modulation for
   * each pixel.
   */
  const SFNode &getOcclusionTexture() const { return _occlusionTexture; }

  /**
   * @details Permitted X3D node types: X3DSingleTextureNode
   */
  static const std::vector<std::string> &acceptableOcclusionTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details When applying occlusionStrength for this material node, the
   * contained texture provides Physically Based Rendering (PBR) modulation for
   * each pixel.
   */
  void setOcclusionTexture(const SFNode &value) { _occlusionTexture = value; }

  void setOcclusionTexture(SFNode &&value) {

    _occlusionTexture = std::move(value);
  }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   */
  const SFString &getOcclusionTextureMapping() const {
    return _occlusionTextureMapping;
  }

  /**
   * Access: inputOutput.
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   */
  void setOcclusionTextureMapping(const SFString &value) {

    _occlusionTextureMapping = value;
  }

  void setOcclusionTextureMapping(SFString &&value) {

    _occlusionTextureMapping = std::move(value);
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details roughness is a PBR parameter (TODO elaborate).
   */
  const SFFloat &getRoughness() const { return _roughness; }

  /**
   * Access: inputOutput.
   * @details roughness is a PBR parameter (TODO elaborate).
   */
  void setRoughness(const SFFloat &value) {

    validateRoughness(value);

    _roughness = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setRoughness() for range validation.
   */
  void setRoughnessUnchecked(const SFFloat &value) { _roughness = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details how "clear" an object is: 1.
   */
  const SFFloat &getTransparency() const { return _transparency; }

  /**
   * Access: inputOutput.
   * @details how "clear" an object is: 1.
   */
  void setTransparency(const SFFloat &value) {

    validateTransparency(value);

    _transparency = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setTransparency() for range validation.
   */
  void setTransparencyUnchecked(const SFFloat &value) { _transparency = value; }

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
  static void checkRangesBaseColor(const SFColor &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesEmissiveColor(const SFColor &value,
                                       const std::string &nodeType,
                                       const std::string &defName,
                                       std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesMetallic(const SFFloat &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesOcclusionStrength(const SFFloat &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesRoughness(const SFFloat &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesTransparency(const SFFloat &value,
                                      const std::string &nodeType,
                                      const std::string &defName,
                                      std::vector<RangeDiagnostic> &out);

private:
  static void validateBaseColor(const SFColor &value) {

    if (value.r < 0.0f)
      throw std::out_of_range("baseColor.r below minimum of 0");
    if (value.r > 1.0f)
      throw std::out_of_range("baseColor.r above maximum of 1");

    if (value.g < 0.0f)
      throw std::out_of_range("baseColor.g below minimum of 0");
    if (value.g > 1.0f)
      throw std::out_of_range("baseColor.g above maximum of 1");

    if (value.b < 0.0f)
      throw std::out_of_range("baseColor.b below minimum of 0");
    if (value.b > 1.0f)
      throw std::out_of_range("baseColor.b above maximum of 1");
  }

  static void validateEmissiveColor(const SFColor &value) {

    if (value.r < 0.0f)
      throw std::out_of_range("emissiveColor.r below minimum of 0");
    if (value.r > 1.0f)
      throw std::out_of_range("emissiveColor.r above maximum of 1");

    if (value.g < 0.0f)
      throw std::out_of_range("emissiveColor.g below minimum of 0");
    if (value.g > 1.0f)
      throw std::out_of_range("emissiveColor.g above maximum of 1");

    if (value.b < 0.0f)
      throw std::out_of_range("emissiveColor.b below minimum of 0");
    if (value.b > 1.0f)
      throw std::out_of_range("emissiveColor.b above maximum of 1");
  }

  static void validateMetallic(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("metallic below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("metallic above maximum of 1");
  }

  static void validateOcclusionStrength(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("occlusionStrength below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("occlusionStrength above maximum of 1");
  }

  static void validateRoughness(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("roughness below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("roughness above maximum of 1");
  }

  static void validateTransparency(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("transparency below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("transparency above maximum of 1");
  }

  SFColor _baseColor{SFColor{1, 1, 1}};

  SFNode _baseTexture{nullptr};

  SFString _baseTextureMapping{};

  SFColor _emissiveColor{SFColor{0, 0, 0}};

  SFNode _emissiveTexture{nullptr};

  SFFloat _metallic{1};

  SFNode _metallicRoughnessTexture{nullptr};

  SFString _metallicRoughnessTextureMapping{};

  SFNode _normalTexture{nullptr};

  SFFloat _occlusionStrength{1};

  SFNode _occlusionTexture{nullptr};

  SFString _occlusionTextureMapping{};

  SFFloat _roughness{1};

  SFFloat _transparency{0};
};

} // namespace x3d::nodes
