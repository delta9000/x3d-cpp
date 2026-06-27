// PhysicalMaterial.hpp
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
  /**
   * @brief Default constructor for PhysicalMaterial
   */
  PhysicalMaterial() = default;

  /**
   * @brief Destructor for PhysicalMaterial
   */
  ~PhysicalMaterial() = default;

  /**
   * @brief Get the default value for baseColor
   * @return SFColor The default value
   */
  static SFColor getDefaultBaseColor() { return SFColor{1, 1, 1}; }

  /**
   * @brief Get the default value for baseTexture
   * @return SFNode The default value
   */
  static SFNode getDefaultBaseTexture() { return nullptr; }

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
   * @brief Get the default value for metallic
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMetallic() { return 1; }

  /**
   * @brief Get the default value for metallicRoughnessTexture
   * @return SFNode The default value
   */
  static SFNode getDefaultMetallicRoughnessTexture() { return nullptr; }

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
   * @brief Get the default value for roughness
   * @return SFFloat The default value
   */
  static SFFloat getDefaultRoughness() { return 1; }

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
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of baseColor. AccessType: inputOutput
   * @details similar to diffuseColor, TODO define more precisely.
   * @return SFColor The current value of baseColor.
   */
  SFColor getBaseColor() const { return _baseColor; }

  /**
   * @brief Sets the value of baseColor. AccessType: inputOutput
   * @details similar to diffuseColor, TODO define more precisely.
   * @param value The new value for baseColor.
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
   * @brief Non-validating write of baseColor (runtime/reader ingest path).
   * @details Assigns without the range check setBaseColor() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setBaseColor() stays the
   *          enforcement point for programmatic callers.
   */
  void setBaseColorUnchecked(const SFColor &value) { _baseColor = value; }

  /**
   * @brief Gets the value of baseTexture. AccessType: inputOutput
   * @details When applying baseColor for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   * @return SFNode The current value of baseTexture.
   */
  SFNode getBaseTexture() const { return _baseTexture; }

  /**
   * @brief Acceptable node types for the baseTexture field.
   * @details Permitted X3D node types: X3DSingleTextureNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableBaseTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * @brief Sets the value of baseTexture. AccessType: inputOutput
   * @details When applying baseColor for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   * @param value The new value for baseTexture.
   */
  void setBaseTexture(const SFNode &value) { _baseTexture = value; }

  void setBaseTexture(SFNode &&value) { _baseTexture = std::move(value); }

  /**
   * @brief Gets the value of baseTextureMapping. AccessType: inputOutput
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   * @return SFString The current value of baseTextureMapping.
   */
  SFString getBaseTextureMapping() const { return _baseTextureMapping; }

  /**
   * @brief Sets the value of baseTextureMapping. AccessType: inputOutput
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   * @param value The new value for baseTextureMapping.
   */
  void setBaseTextureMapping(const SFString &value) {

    _baseTextureMapping = value;
  }

  void setBaseTextureMapping(SFString &&value) {

    _baseTextureMapping = std::move(value);
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
   * @brief Gets the value of metallic. AccessType: inputOutput
   * @details metallic is a PBR parameter (TODO elaborate).
   * @return SFFloat The current value of metallic.
   */
  SFFloat getMetallic() const { return _metallic; }

  /**
   * @brief Sets the value of metallic. AccessType: inputOutput
   * @details metallic is a PBR parameter (TODO elaborate).
   * @param value The new value for metallic.
   */
  void setMetallic(const SFFloat &value) {

    validateMetallic(value);

    _metallic = value;
  }

  /**
   * @brief Non-validating write of metallic (runtime/reader ingest path).
   * @details Assigns without the range check setMetallic() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setMetallic() stays the
   *          enforcement point for programmatic callers.
   */
  void setMetallicUnchecked(const SFFloat &value) { _metallic = value; }

  /**
   * @brief Gets the value of metallicRoughnessTexture. AccessType: inputOutput
   * @details When applying metallic for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   * @return SFNode The current value of metallicRoughnessTexture.
   */
  SFNode getMetallicRoughnessTexture() const {
    return _metallicRoughnessTexture;
  }

  /**
   * @brief Acceptable node types for the metallicRoughnessTexture field.
   * @details Permitted X3D node types: X3DSingleTextureNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &
  acceptableMetallicRoughnessTextureNodeTypes() {
    static const std::vector<std::string> types = {"X3DSingleTextureNode"};
    return types;
  }

  /**
   * @brief Sets the value of metallicRoughnessTexture. AccessType: inputOutput
   * @details When applying metallic for this material node, the contained
   * texture provides Physically Based Rendering (PBR) modulation for each
   * pixel.
   * @param value The new value for metallicRoughnessTexture.
   */
  void setMetallicRoughnessTexture(const SFNode &value) {

    _metallicRoughnessTexture = value;
  }

  void setMetallicRoughnessTexture(SFNode &&value) {

    _metallicRoughnessTexture = std::move(value);
  }

  /**
   * @brief Gets the value of metallicRoughnessTextureMapping. AccessType:
   * inputOutput
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   * @return SFString The current value of metallicRoughnessTextureMapping.
   */
  SFString getMetallicRoughnessTextureMapping() const {
    return _metallicRoughnessTextureMapping;
  }

  /**
   * @brief Sets the value of metallicRoughnessTextureMapping. AccessType:
   * inputOutput
   * @details The mapping label identifies which texture coordinates and
   * transformations are used to compute texture effects from corresponding
   * geometry on a given material.
   * @param value The new value for metallicRoughnessTextureMapping.
   */
  void setMetallicRoughnessTextureMapping(const SFString &value) {

    _metallicRoughnessTextureMapping = value;
  }

  void setMetallicRoughnessTextureMapping(SFString &&value) {

    _metallicRoughnessTextureMapping = std::move(value);
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
   * @brief Gets the value of roughness. AccessType: inputOutput
   * @details roughness is a PBR parameter (TODO elaborate).
   * @return SFFloat The current value of roughness.
   */
  SFFloat getRoughness() const { return _roughness; }

  /**
   * @brief Sets the value of roughness. AccessType: inputOutput
   * @details roughness is a PBR parameter (TODO elaborate).
   * @param value The new value for roughness.
   */
  void setRoughness(const SFFloat &value) {

    validateRoughness(value);

    _roughness = value;
  }

  /**
   * @brief Non-validating write of roughness (runtime/reader ingest path).
   * @details Assigns without the range check setRoughness() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setRoughness() stays the
   *          enforcement point for programmatic callers.
   */
  void setRoughnessUnchecked(const SFFloat &value) { _roughness = value; }

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
   * @brief The X3D type name of this node (e.g. "PhysicalMaterial").
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

    if (value.r < 0)
      throw std::out_of_range("baseColor.r below minimum of 0");
    if (value.r > 1)
      throw std::out_of_range("baseColor.r above maximum of 1");

    if (value.g < 0)
      throw std::out_of_range("baseColor.g below minimum of 0");
    if (value.g > 1)
      throw std::out_of_range("baseColor.g above maximum of 1");

    if (value.b < 0)
      throw std::out_of_range("baseColor.b below minimum of 0");
    if (value.b > 1)
      throw std::out_of_range("baseColor.b above maximum of 1");
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

  static void validateMetallic(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("metallic below minimum of 0");
    if (value > 1)
      throw std::out_of_range("metallic above maximum of 1");
  }

  static void validateOcclusionStrength(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("occlusionStrength below minimum of 0");
    if (value > 1)
      throw std::out_of_range("occlusionStrength above maximum of 1");
  }

  static void validateRoughness(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("roughness below minimum of 0");
    if (value > 1)
      throw std::out_of_range("roughness above maximum of 1");
  }

  static void validateTransparency(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("transparency below minimum of 0");
    if (value > 1)
      throw std::out_of_range("transparency above maximum of 1");
  }

  /**
   * @brief Member variable for baseColor.
   */

  SFColor _baseColor{SFColor{1, 1, 1}};

  /**
   * @brief Member variable for baseTexture.
   */

  SFNode _baseTexture{nullptr};

  /**
   * @brief Member variable for baseTextureMapping.
   */

  SFString _baseTextureMapping{};

  /**
   * @brief Member variable for emissiveColor.
   */

  SFColor _emissiveColor{SFColor{0, 0, 0}};

  /**
   * @brief Member variable for emissiveTexture.
   */

  SFNode _emissiveTexture{nullptr};

  /**
   * @brief Member variable for metallic.
   */

  SFFloat _metallic{1};

  /**
   * @brief Member variable for metallicRoughnessTexture.
   */

  SFNode _metallicRoughnessTexture{nullptr};

  /**
   * @brief Member variable for metallicRoughnessTextureMapping.
   */

  SFString _metallicRoughnessTextureMapping{};

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
   * @brief Member variable for roughness.
   */

  SFFloat _roughness{1};

  /**
   * @brief Member variable for transparency.
   */

  SFFloat _transparency{0};
};

} // namespace x3d::nodes
