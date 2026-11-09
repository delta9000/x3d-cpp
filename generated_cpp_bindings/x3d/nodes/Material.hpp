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
 * @class Material
 * @brief Material specifies surface rendering properties for associated
 * geometry nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shape.html#Material
 */
class Material : public virtual X3DOneSidedMaterialNode {
public:
  Material() = default;

  ~Material() = default;

  static SFFloat getDefaultAmbientIntensity() { return 0.2; }

  static SFNode getDefaultAmbientTexture() { return nullptr; }

  static SFColor getDefaultDiffuseColor() { return SFColor{0.8, 0.8, 0.8}; }

  static SFNode getDefaultDiffuseTexture() { return nullptr; }

  static SFColor getDefaultEmissiveColor() { return SFColor{0, 0, 0}; }

  static SFNode getDefaultEmissiveTexture() { return nullptr; }

  static SFNode getDefaultNormalTexture() { return nullptr; }

  static SFFloat getDefaultOcclusionStrength() { return 1; }

  static SFNode getDefaultOcclusionTexture() { return nullptr; }

  static SFFloat getDefaultShininess() { return 0.2; }

  static SFNode getDefaultShininessTexture() { return nullptr; }

  static SFColor getDefaultSpecularColor() { return SFColor{0, 0, 0}; }

  static SFNode getDefaultSpecularTexture() { return nullptr; }

  static SFFloat getDefaultTransparency() { return 0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "material"; }

  static std::string componentName() { return "Shape"; }

  static int componentLevel() { return 1; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details how much ambient omnidirectional light is reflected from all light
   * sources.
   */
  const SFFloat &getAmbientIntensity() const { return _ambientIntensity; }

  /**
   * Access: inputOutput.
   * @details how much ambient omnidirectional light is reflected from all light
   * sources.
   */
  void setAmbientIntensity(const SFFloat &value) {

    validateAmbientIntensity(value);

    _ambientIntensity = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setAmbientIntensity() for range validation.
   */
  void setAmbientIntensityUnchecked(const SFFloat &value) {
    _ambientIntensity = value;
  }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details When applying ambientIntensity for this material node, the
   * contained texture provides Physically Based Rendering (PBR) modulation for
   * each pixel.
   */
  const SFNode &getAmbientTexture() const { return _ambientTexture; }

  /**
   * @details Permitted X3D node types: X3DSingleTextureNode
   */
  static const std::vector<std::string> &acceptableAmbientTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details When applying ambientIntensity for this material node, the
   * contained texture provides Physically Based Rendering (PBR) modulation for
   * each pixel.
   */
  void setAmbientTexture(const SFNode &value) { _ambientTexture = value; }

  void setAmbientTexture(SFNode &&value) { _ambientTexture = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   */
  const SFString &getAmbientTextureMapping() const {
    return _ambientTextureMapping;
  }

  /**
   * Access: inputOutput.
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   */
  void setAmbientTextureMapping(const SFString &value) {

    _ambientTextureMapping = value;
  }

  void setAmbientTextureMapping(SFString &&value) {

    _ambientTextureMapping = std::move(value);
  }

  /**
   * SFColor; access: inputOutput. Borrows the stored value.
   * @details how much direct, angle-dependent light is reflected from all light
   * sources.
   */
  const SFColor &getDiffuseColor() const { return _diffuseColor; }

  /**
   * Access: inputOutput.
   * @details how much direct, angle-dependent light is reflected from all light
   * sources.
   */
  void setDiffuseColor(const SFColor &value) {

    validateDiffuseColor(value);

    _diffuseColor = value;
  }

  void setDiffuseColor(SFColor &&value) {

    validateDiffuseColor(value);

    _diffuseColor = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setDiffuseColor() for range validation.
   */
  void setDiffuseColorUnchecked(const SFColor &value) { _diffuseColor = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details When applying diffuseColor for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   */
  const SFNode &getDiffuseTexture() const { return _diffuseTexture; }

  /**
   * @details Permitted X3D node types: X3DSingleTextureNode
   */
  static const std::vector<std::string> &acceptableDiffuseTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details When applying diffuseColor for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   */
  void setDiffuseTexture(const SFNode &value) { _diffuseTexture = value; }

  void setDiffuseTexture(SFNode &&value) { _diffuseTexture = std::move(value); }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   */
  const SFString &getDiffuseTextureMapping() const {
    return _diffuseTextureMapping;
  }

  /**
   * Access: inputOutput.
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   */
  void setDiffuseTextureMapping(const SFString &value) {

    _diffuseTextureMapping = value;
  }

  void setDiffuseTextureMapping(SFString &&value) {

    _diffuseTextureMapping = std::move(value);
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
   * @details Lower shininess values provide soft specular glows, while higher
   * values result in sharper, smaller highlights.
   */
  const SFFloat &getShininess() const { return _shininess; }

  /**
   * Access: inputOutput.
   * @details Lower shininess values provide soft specular glows, while higher
   * values result in sharper, smaller highlights.
   */
  void setShininess(const SFFloat &value) {

    validateShininess(value);

    _shininess = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setShininess() for range validation.
   */
  void setShininessUnchecked(const SFFloat &value) { _shininess = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details When applying shininess for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   */
  const SFNode &getShininessTexture() const { return _shininessTexture; }

  /**
   * @details Permitted X3D node types: X3DSingleTextureNode
   */
  static const std::vector<std::string> &acceptableShininessTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details When applying shininess for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   */
  void setShininessTexture(const SFNode &value) { _shininessTexture = value; }

  void setShininessTexture(SFNode &&value) {

    _shininessTexture = std::move(value);
  }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   */
  const SFString &getShininessTextureMapping() const {
    return _shininessTextureMapping;
  }

  /**
   * Access: inputOutput.
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   */
  void setShininessTextureMapping(const SFString &value) {

    _shininessTextureMapping = value;
  }

  void setShininessTextureMapping(SFString &&value) {

    _shininessTextureMapping = std::move(value);
  }

  /**
   * SFColor; access: inputOutput. Borrows the stored value.
   * @details specular highlights are brightness reflections (example: shiny
   * spots on an apple).
   */
  const SFColor &getSpecularColor() const { return _specularColor; }

  /**
   * Access: inputOutput.
   * @details specular highlights are brightness reflections (example: shiny
   * spots on an apple).
   */
  void setSpecularColor(const SFColor &value) {

    validateSpecularColor(value);

    _specularColor = value;
  }

  void setSpecularColor(SFColor &&value) {

    validateSpecularColor(value);

    _specularColor = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setSpecularColor() for range validation.
   */
  void setSpecularColorUnchecked(const SFColor &value) {
    _specularColor = value;
  }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details When applying specularColor for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   */
  const SFNode &getSpecularTexture() const { return _specularTexture; }

  /**
   * @details Permitted X3D node types: X3DSingleTextureNode
   */
  static const std::vector<std::string> &acceptableSpecularTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details When applying specularColor for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   */
  void setSpecularTexture(const SFNode &value) { _specularTexture = value; }

  void setSpecularTexture(SFNode &&value) {

    _specularTexture = std::move(value);
  }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   */
  const SFString &getSpecularTextureMapping() const {
    return _specularTextureMapping;
  }

  /**
   * Access: inputOutput.
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   */
  void setSpecularTextureMapping(const SFString &value) {

    _specularTextureMapping = value;
  }

  void setSpecularTextureMapping(SFString &&value) {

    _specularTextureMapping = std::move(value);
  }

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
  static void checkRangesAmbientIntensity(const SFFloat &value,
                                          const std::string &nodeType,
                                          const std::string &defName,
                                          std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesDiffuseColor(const SFColor &value,
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
  static void checkRangesOcclusionStrength(const SFFloat &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesShininess(const SFFloat &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesSpecularColor(const SFColor &value,
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
  static void validateAmbientIntensity(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("ambientIntensity below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("ambientIntensity above maximum of 1");
  }

  static void validateDiffuseColor(const SFColor &value) {

    if (value.r < 0.0f)
      throw std::out_of_range("diffuseColor.r below minimum of 0");
    if (value.r > 1.0f)
      throw std::out_of_range("diffuseColor.r above maximum of 1");

    if (value.g < 0.0f)
      throw std::out_of_range("diffuseColor.g below minimum of 0");
    if (value.g > 1.0f)
      throw std::out_of_range("diffuseColor.g above maximum of 1");

    if (value.b < 0.0f)
      throw std::out_of_range("diffuseColor.b below minimum of 0");
    if (value.b > 1.0f)
      throw std::out_of_range("diffuseColor.b above maximum of 1");
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

  static void validateOcclusionStrength(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("occlusionStrength below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("occlusionStrength above maximum of 1");
  }

  static void validateShininess(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("shininess below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("shininess above maximum of 1");
  }

  static void validateSpecularColor(const SFColor &value) {

    if (value.r < 0.0f)
      throw std::out_of_range("specularColor.r below minimum of 0");
    if (value.r > 1.0f)
      throw std::out_of_range("specularColor.r above maximum of 1");

    if (value.g < 0.0f)
      throw std::out_of_range("specularColor.g below minimum of 0");
    if (value.g > 1.0f)
      throw std::out_of_range("specularColor.g above maximum of 1");

    if (value.b < 0.0f)
      throw std::out_of_range("specularColor.b below minimum of 0");
    if (value.b > 1.0f)
      throw std::out_of_range("specularColor.b above maximum of 1");
  }

  static void validateTransparency(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("transparency below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("transparency above maximum of 1");
  }

  SFFloat _ambientIntensity{0.2};

  SFNode _ambientTexture{nullptr};

  SFString _ambientTextureMapping{};

  SFColor _diffuseColor{SFColor{0.8, 0.8, 0.8}};

  SFNode _diffuseTexture{nullptr};

  SFString _diffuseTextureMapping{};

  SFColor _emissiveColor{SFColor{0, 0, 0}};

  SFNode _emissiveTexture{nullptr};

  SFNode _normalTexture{nullptr};

  SFFloat _occlusionStrength{1};

  SFNode _occlusionTexture{nullptr};

  SFString _occlusionTextureMapping{};

  SFFloat _shininess{0.2};

  SFNode _shininessTexture{nullptr};

  SFString _shininessTextureMapping{};

  SFColor _specularColor{SFColor{0, 0, 0}};

  SFNode _specularTexture{nullptr};

  SFString _specularTextureMapping{};

  SFFloat _transparency{0};
};

} // namespace x3d::nodes
