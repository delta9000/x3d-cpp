// Material.hpp
#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DAppearanceChildNode.hpp"

#include "X3DMaterialNode.hpp"

#include "X3DOneSidedMaterialNode.hpp"

/**
 * @class Material
 * @brief Material specifies surface rendering properties for associated
 * geometry nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shape.html#Material
 */
class Material : public virtual X3DOneSidedMaterialNode {
public:
  /**
   * @brief Default constructor for Material
   */
  Material() = default;

  /**
   * @brief Destructor for Material
   */
  ~Material() = default;

  /**
   * @brief Get the default value for ambientIntensity
   * @return SFFloat The default value
   */
  static SFFloat getDefaultAmbientIntensity() { return 0.2; }

  /**
   * @brief Get the default value for ambientTexture
   * @return SFNode The default value
   */
  static SFNode getDefaultAmbientTexture() { return nullptr; }

  /**
   * @brief Get the default value for diffuseColor
   * @return SFColor The default value
   */
  static SFColor getDefaultDiffuseColor() { return SFColor{0.8, 0.8, 0.8}; }

  /**
   * @brief Get the default value for diffuseTexture
   * @return SFNode The default value
   */
  static SFNode getDefaultDiffuseTexture() { return nullptr; }

  /**
   * @brief Get the default value for emissiveColor
   * @return SFColor The default value
   */
  static SFColor getDefaultEmissiveColor() { return SFColor{0, 0, 0}; }

  /**
   * @brief Get the default value for emissiveTexture
   * @return SFNode The default value
   */
  static SFNode getDefaultEmissiveTexture() { return nullptr; }

  /**
   * @brief Get the default value for normalTexture
   * @return SFNode The default value
   */
  static SFNode getDefaultNormalTexture() { return nullptr; }

  /**
   * @brief Get the default value for occlusionStrength
   * @return SFFloat The default value
   */
  static SFFloat getDefaultOcclusionStrength() { return 1; }

  /**
   * @brief Get the default value for occlusionTexture
   * @return SFNode The default value
   */
  static SFNode getDefaultOcclusionTexture() { return nullptr; }

  /**
   * @brief Get the default value for shininess
   * @return SFFloat The default value
   */
  static SFFloat getDefaultShininess() { return 0.2; }

  /**
   * @brief Get the default value for shininessTexture
   * @return SFNode The default value
   */
  static SFNode getDefaultShininessTexture() { return nullptr; }

  /**
   * @brief Get the default value for specularColor
   * @return SFColor The default value
   */
  static SFColor getDefaultSpecularColor() { return SFColor{0, 0, 0}; }

  /**
   * @brief Get the default value for specularTexture
   * @return SFNode The default value
   */
  static SFNode getDefaultSpecularTexture() { return nullptr; }

  /**
   * @brief Get the default value for transparency
   * @return SFFloat The default value
   */
  static SFFloat getDefaultTransparency() { return 0; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "material"; }

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
   * @brief Gets the value of ambientIntensity. AccessType: inputOutput
   * @details how much ambient omnidirectional light is reflected from all light
   * sources.
   * @return SFFloat The current value of ambientIntensity.
   */
  SFFloat getAmbientIntensity() const { return _ambientIntensity; }

  /**
   * @brief Sets the value of ambientIntensity. AccessType: inputOutput
   * @details how much ambient omnidirectional light is reflected from all light
   * sources.
   * @param value The new value for ambientIntensity.
   */
  void setAmbientIntensity(const SFFloat &value) {

    validateAmbientIntensity(value);

    _ambientIntensity = value;
  }

  /**
   * @brief Non-validating write of ambientIntensity (runtime/reader ingest
   * path).
   * @details Assigns without the range check setAmbientIntensity() enforces, so
   * a permissive reader keeps an out-of-range authored value rather than reject
   * the whole document. setAmbientIntensity() stays the enforcement point for
   * programmatic callers.
   */
  void setAmbientIntensityUnchecked(const SFFloat &value) {
    _ambientIntensity = value;
  }

  /**
   * @brief Gets the value of ambientTexture. AccessType: inputOutput
   * @details When applying ambientIntensity for this material node, the
   * contained texture provides Physically Based Rendering (PBR) modulation for
   * each pixel.
   * @return SFNode The current value of ambientTexture.
   */
  SFNode getAmbientTexture() const { return _ambientTexture; }

  /**
   * @brief Acceptable node types for the ambientTexture field.
   * @details Permitted X3D node types: X3DSingleTextureNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableAmbientTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * @brief Sets the value of ambientTexture. AccessType: inputOutput
   * @details When applying ambientIntensity for this material node, the
   * contained texture provides Physically Based Rendering (PBR) modulation for
   * each pixel.
   * @param value The new value for ambientTexture.
   */
  void setAmbientTexture(const SFNode &value) { _ambientTexture = value; }

  void setAmbientTexture(SFNode &&value) { _ambientTexture = std::move(value); }

  /**
   * @brief Gets the value of ambientTextureMapping. AccessType: inputOutput
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   * @return SFString The current value of ambientTextureMapping.
   */
  SFString getAmbientTextureMapping() const { return _ambientTextureMapping; }

  /**
   * @brief Sets the value of ambientTextureMapping. AccessType: inputOutput
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   * @param value The new value for ambientTextureMapping.
   */
  void setAmbientTextureMapping(const SFString &value) {

    _ambientTextureMapping = value;
  }

  void setAmbientTextureMapping(SFString &&value) {

    _ambientTextureMapping = std::move(value);
  }

  /**
   * @brief Gets the value of diffuseColor. AccessType: inputOutput
   * @details how much direct, angle-dependent light is reflected from all light
   * sources.
   * @return SFColor The current value of diffuseColor.
   */
  SFColor getDiffuseColor() const { return _diffuseColor; }

  /**
   * @brief Sets the value of diffuseColor. AccessType: inputOutput
   * @details how much direct, angle-dependent light is reflected from all light
   * sources.
   * @param value The new value for diffuseColor.
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
   * @brief Non-validating write of diffuseColor (runtime/reader ingest path).
   * @details Assigns without the range check setDiffuseColor() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setDiffuseColor() stays the
   *          enforcement point for programmatic callers.
   */
  void setDiffuseColorUnchecked(const SFColor &value) { _diffuseColor = value; }

  /**
   * @brief Gets the value of diffuseTexture. AccessType: inputOutput
   * @details When applying diffuseColor for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   * @return SFNode The current value of diffuseTexture.
   */
  SFNode getDiffuseTexture() const { return _diffuseTexture; }

  /**
   * @brief Acceptable node types for the diffuseTexture field.
   * @details Permitted X3D node types: X3DSingleTextureNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableDiffuseTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * @brief Sets the value of diffuseTexture. AccessType: inputOutput
   * @details When applying diffuseColor for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   * @param value The new value for diffuseTexture.
   */
  void setDiffuseTexture(const SFNode &value) { _diffuseTexture = value; }

  void setDiffuseTexture(SFNode &&value) { _diffuseTexture = std::move(value); }

  /**
   * @brief Gets the value of diffuseTextureMapping. AccessType: inputOutput
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   * @return SFString The current value of diffuseTextureMapping.
   */
  SFString getDiffuseTextureMapping() const { return _diffuseTextureMapping; }

  /**
   * @brief Sets the value of diffuseTextureMapping. AccessType: inputOutput
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   * @param value The new value for diffuseTextureMapping.
   */
  void setDiffuseTextureMapping(const SFString &value) {

    _diffuseTextureMapping = value;
  }

  void setDiffuseTextureMapping(SFString &&value) {

    _diffuseTextureMapping = std::move(value);
  }

  /**
   * @brief Gets the value of emissiveColor. AccessType: inputOutput
   * @details how much glowing light is emitted from this object.
   * @return SFColor The current value of emissiveColor.
   */
  SFColor getEmissiveColor() const { return _emissiveColor; }

  /**
   * @brief Sets the value of emissiveColor. AccessType: inputOutput
   * @details how much glowing light is emitted from this object.
   * @param value The new value for emissiveColor.
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
   * @brief Non-validating write of emissiveColor (runtime/reader ingest path).
   * @details Assigns without the range check setEmissiveColor() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setEmissiveColor() stays the
   *          enforcement point for programmatic callers.
   */
  void setEmissiveColorUnchecked(const SFColor &value) {
    _emissiveColor = value;
  }

  /**
   * @brief Gets the value of emissiveTexture. AccessType: inputOutput
   * @details When applying emissiveColor for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   * @return SFNode The current value of emissiveTexture.
   */
  SFNode getEmissiveTexture() const { return _emissiveTexture; }

  /**
   * @brief Acceptable node types for the emissiveTexture field.
   * @details Permitted X3D node types: X3DSingleTextureNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableEmissiveTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * @brief Sets the value of emissiveTexture. AccessType: inputOutput
   * @details When applying emissiveColor for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   * @param value The new value for emissiveTexture.
   */
  void setEmissiveTexture(const SFNode &value) { _emissiveTexture = value; }

  void setEmissiveTexture(SFNode &&value) {

    _emissiveTexture = std::move(value);
  }

  /**
   * @brief Gets the value of normalTexture. AccessType: inputOutput
   * @details When applying normalScale for this material node, the contained
   * texture modulates the texture across the surface.
   * @return SFNode The current value of normalTexture.
   */
  SFNode getNormalTexture() const { return _normalTexture; }

  /**
   * @brief Acceptable node types for the normalTexture field.
   * @details Permitted X3D node types: X3DSingleTextureNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableNormalTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * @brief Sets the value of normalTexture. AccessType: inputOutput
   * @details When applying normalScale for this material node, the contained
   * texture modulates the texture across the surface.
   * @param value The new value for normalTexture.
   */
  void setNormalTexture(const SFNode &value) { _normalTexture = value; }

  void setNormalTexture(SFNode &&value) { _normalTexture = std::move(value); }

  /**
   * @brief Gets the value of occlusionStrength. AccessType: inputOutput
   * @details occlusionStrength indicates areas of indirect lighting, typically
   * called ambient occlusion.
   * @return SFFloat The current value of occlusionStrength.
   */
  SFFloat getOcclusionStrength() const { return _occlusionStrength; }

  /**
   * @brief Sets the value of occlusionStrength. AccessType: inputOutput
   * @details occlusionStrength indicates areas of indirect lighting, typically
   * called ambient occlusion.
   * @param value The new value for occlusionStrength.
   */
  void setOcclusionStrength(const SFFloat &value) {

    validateOcclusionStrength(value);

    _occlusionStrength = value;
  }

  /**
   * @brief Non-validating write of occlusionStrength (runtime/reader ingest
   * path).
   * @details Assigns without the range check setOcclusionStrength() enforces,
   * so a permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setOcclusionStrength() stays the
   *          enforcement point for programmatic callers.
   */
  void setOcclusionStrengthUnchecked(const SFFloat &value) {
    _occlusionStrength = value;
  }

  /**
   * @brief Gets the value of occlusionTexture. AccessType: inputOutput
   * @details When applying occlusionStrength for this material node, the
   * contained texture provides Physically Based Rendering (PBR) modulation for
   * each pixel.
   * @return SFNode The current value of occlusionTexture.
   */
  SFNode getOcclusionTexture() const { return _occlusionTexture; }

  /**
   * @brief Acceptable node types for the occlusionTexture field.
   * @details Permitted X3D node types: X3DSingleTextureNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableOcclusionTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * @brief Sets the value of occlusionTexture. AccessType: inputOutput
   * @details When applying occlusionStrength for this material node, the
   * contained texture provides Physically Based Rendering (PBR) modulation for
   * each pixel.
   * @param value The new value for occlusionTexture.
   */
  void setOcclusionTexture(const SFNode &value) { _occlusionTexture = value; }

  void setOcclusionTexture(SFNode &&value) {

    _occlusionTexture = std::move(value);
  }

  /**
   * @brief Gets the value of occlusionTextureMapping. AccessType: inputOutput
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   * @return SFString The current value of occlusionTextureMapping.
   */
  SFString getOcclusionTextureMapping() const {
    return _occlusionTextureMapping;
  }

  /**
   * @brief Sets the value of occlusionTextureMapping. AccessType: inputOutput
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   * @param value The new value for occlusionTextureMapping.
   */
  void setOcclusionTextureMapping(const SFString &value) {

    _occlusionTextureMapping = value;
  }

  void setOcclusionTextureMapping(SFString &&value) {

    _occlusionTextureMapping = std::move(value);
  }

  /**
   * @brief Gets the value of shininess. AccessType: inputOutput
   * @details Lower shininess values provide soft specular glows, while higher
   * values result in sharper, smaller highlights.
   * @return SFFloat The current value of shininess.
   */
  SFFloat getShininess() const { return _shininess; }

  /**
   * @brief Sets the value of shininess. AccessType: inputOutput
   * @details Lower shininess values provide soft specular glows, while higher
   * values result in sharper, smaller highlights.
   * @param value The new value for shininess.
   */
  void setShininess(const SFFloat &value) {

    validateShininess(value);

    _shininess = value;
  }

  /**
   * @brief Non-validating write of shininess (runtime/reader ingest path).
   * @details Assigns without the range check setShininess() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setShininess() stays the
   *          enforcement point for programmatic callers.
   */
  void setShininessUnchecked(const SFFloat &value) { _shininess = value; }

  /**
   * @brief Gets the value of shininessTexture. AccessType: inputOutput
   * @details When applying shininess for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   * @return SFNode The current value of shininessTexture.
   */
  SFNode getShininessTexture() const { return _shininessTexture; }

  /**
   * @brief Acceptable node types for the shininessTexture field.
   * @details Permitted X3D node types: X3DSingleTextureNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableShininessTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * @brief Sets the value of shininessTexture. AccessType: inputOutput
   * @details When applying shininess for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   * @param value The new value for shininessTexture.
   */
  void setShininessTexture(const SFNode &value) { _shininessTexture = value; }

  void setShininessTexture(SFNode &&value) {

    _shininessTexture = std::move(value);
  }

  /**
   * @brief Gets the value of shininessTextureMapping. AccessType: inputOutput
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   * @return SFString The current value of shininessTextureMapping.
   */
  SFString getShininessTextureMapping() const {
    return _shininessTextureMapping;
  }

  /**
   * @brief Sets the value of shininessTextureMapping. AccessType: inputOutput
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   * @param value The new value for shininessTextureMapping.
   */
  void setShininessTextureMapping(const SFString &value) {

    _shininessTextureMapping = value;
  }

  void setShininessTextureMapping(SFString &&value) {

    _shininessTextureMapping = std::move(value);
  }

  /**
   * @brief Gets the value of specularColor. AccessType: inputOutput
   * @details specular highlights are brightness reflections (example: shiny
   * spots on an apple).
   * @return SFColor The current value of specularColor.
   */
  SFColor getSpecularColor() const { return _specularColor; }

  /**
   * @brief Sets the value of specularColor. AccessType: inputOutput
   * @details specular highlights are brightness reflections (example: shiny
   * spots on an apple).
   * @param value The new value for specularColor.
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
   * @brief Non-validating write of specularColor (runtime/reader ingest path).
   * @details Assigns without the range check setSpecularColor() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setSpecularColor() stays the
   *          enforcement point for programmatic callers.
   */
  void setSpecularColorUnchecked(const SFColor &value) {
    _specularColor = value;
  }

  /**
   * @brief Gets the value of specularTexture. AccessType: inputOutput
   * @details When applying specularColor for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   * @return SFNode The current value of specularTexture.
   */
  SFNode getSpecularTexture() const { return _specularTexture; }

  /**
   * @brief Acceptable node types for the specularTexture field.
   * @details Permitted X3D node types: X3DSingleTextureNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableSpecularTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * @brief Sets the value of specularTexture. AccessType: inputOutput
   * @details When applying specularColor for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   * @param value The new value for specularTexture.
   */
  void setSpecularTexture(const SFNode &value) { _specularTexture = value; }

  void setSpecularTexture(SFNode &&value) {

    _specularTexture = std::move(value);
  }

  /**
   * @brief Gets the value of specularTextureMapping. AccessType: inputOutput
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   * @return SFString The current value of specularTextureMapping.
   */
  SFString getSpecularTextureMapping() const { return _specularTextureMapping; }

  /**
   * @brief Sets the value of specularTextureMapping. AccessType: inputOutput
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   * @param value The new value for specularTextureMapping.
   */
  void setSpecularTextureMapping(const SFString &value) {

    _specularTextureMapping = value;
  }

  void setSpecularTextureMapping(SFString &&value) {

    _specularTextureMapping = std::move(value);
  }

  /**
   * @brief Gets the value of transparency. AccessType: inputOutput
   * @details how "clear" an object is: 1.
   * @return SFFloat The current value of transparency.
   */
  SFFloat getTransparency() const { return _transparency; }

  /**
   * @brief Sets the value of transparency. AccessType: inputOutput
   * @details how "clear" an object is: 1.
   * @param value The new value for transparency.
   */
  void setTransparency(const SFFloat &value) {

    validateTransparency(value);

    _transparency = value;
  }

  /**
   * @brief Non-validating write of transparency (runtime/reader ingest path).
   * @details Assigns without the range check setTransparency() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setTransparency() stays the
   *          enforcement point for programmatic callers.
   */
  void setTransparencyUnchecked(const SFFloat &value) { _transparency = value; }

  /**
   * @brief The X3D type name of this node (e.g. "Material").
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

    if (value < 0)
      throw std::out_of_range("ambientIntensity below minimum of 0");
    if (value > 1)
      throw std::out_of_range("ambientIntensity above maximum of 1");
  }

  static void validateDiffuseColor(const SFColor &value) {

    if (value.r < 0)
      throw std::out_of_range("diffuseColor.r below minimum of 0");
    if (value.r > 1)
      throw std::out_of_range("diffuseColor.r above maximum of 1");

    if (value.g < 0)
      throw std::out_of_range("diffuseColor.g below minimum of 0");
    if (value.g > 1)
      throw std::out_of_range("diffuseColor.g above maximum of 1");

    if (value.b < 0)
      throw std::out_of_range("diffuseColor.b below minimum of 0");
    if (value.b > 1)
      throw std::out_of_range("diffuseColor.b above maximum of 1");
  }

  static void validateEmissiveColor(const SFColor &value) {

    if (value.r < 0)
      throw std::out_of_range("emissiveColor.r below minimum of 0");
    if (value.r > 1)
      throw std::out_of_range("emissiveColor.r above maximum of 1");

    if (value.g < 0)
      throw std::out_of_range("emissiveColor.g below minimum of 0");
    if (value.g > 1)
      throw std::out_of_range("emissiveColor.g above maximum of 1");

    if (value.b < 0)
      throw std::out_of_range("emissiveColor.b below minimum of 0");
    if (value.b > 1)
      throw std::out_of_range("emissiveColor.b above maximum of 1");
  }

  static void validateOcclusionStrength(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("occlusionStrength below minimum of 0");
    if (value > 1)
      throw std::out_of_range("occlusionStrength above maximum of 1");
  }

  static void validateShininess(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("shininess below minimum of 0");
    if (value > 1)
      throw std::out_of_range("shininess above maximum of 1");
  }

  static void validateSpecularColor(const SFColor &value) {

    if (value.r < 0)
      throw std::out_of_range("specularColor.r below minimum of 0");
    if (value.r > 1)
      throw std::out_of_range("specularColor.r above maximum of 1");

    if (value.g < 0)
      throw std::out_of_range("specularColor.g below minimum of 0");
    if (value.g > 1)
      throw std::out_of_range("specularColor.g above maximum of 1");

    if (value.b < 0)
      throw std::out_of_range("specularColor.b below minimum of 0");
    if (value.b > 1)
      throw std::out_of_range("specularColor.b above maximum of 1");
  }

  static void validateTransparency(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("transparency below minimum of 0");
    if (value > 1)
      throw std::out_of_range("transparency above maximum of 1");
  }

  /**
   * @brief Member variable for ambientIntensity.
   */

  SFFloat _ambientIntensity{0.2};

  /**
   * @brief Member variable for ambientTexture.
   */

  SFNode _ambientTexture{nullptr};

  /**
   * @brief Member variable for ambientTextureMapping.
   */

  SFString _ambientTextureMapping{};

  /**
   * @brief Member variable for diffuseColor.
   */

  SFColor _diffuseColor{SFColor{0.8, 0.8, 0.8}};

  /**
   * @brief Member variable for diffuseTexture.
   */

  SFNode _diffuseTexture{nullptr};

  /**
   * @brief Member variable for diffuseTextureMapping.
   */

  SFString _diffuseTextureMapping{};

  /**
   * @brief Member variable for emissiveColor.
   */

  SFColor _emissiveColor{SFColor{0, 0, 0}};

  /**
   * @brief Member variable for emissiveTexture.
   */

  SFNode _emissiveTexture{nullptr};

  /**
   * @brief Member variable for normalTexture.
   */

  SFNode _normalTexture{nullptr};

  /**
   * @brief Member variable for occlusionStrength.
   */

  SFFloat _occlusionStrength{1};

  /**
   * @brief Member variable for occlusionTexture.
   */

  SFNode _occlusionTexture{nullptr};

  /**
   * @brief Member variable for occlusionTextureMapping.
   */

  SFString _occlusionTextureMapping{};

  /**
   * @brief Member variable for shininess.
   */

  SFFloat _shininess{0.2};

  /**
   * @brief Member variable for shininessTexture.
   */

  SFNode _shininessTexture{nullptr};

  /**
   * @brief Member variable for shininessTextureMapping.
   */

  SFString _shininessTextureMapping{};

  /**
   * @brief Member variable for specularColor.
   */

  SFColor _specularColor{SFColor{0, 0, 0}};

  /**
   * @brief Member variable for specularTexture.
   */

  SFNode _specularTexture{nullptr};

  /**
   * @brief Member variable for specularTextureMapping.
   */

  SFString _specularTextureMapping{};

  /**
   * @brief Member variable for transparency.
   */

  SFFloat _transparency{0};
};

#endif // MATERIAL_HPP