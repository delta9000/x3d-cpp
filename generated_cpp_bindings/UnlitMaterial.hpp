// UnlitMaterial.hpp
#ifndef UNLITMATERIAL_HPP
#define UNLITMATERIAL_HPP

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
 * @class UnlitMaterial
 * @brief UnlitMaterial specifies surface rendering properties for associated
 * geometry nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shape.html#UnlitMaterial
 */
class UnlitMaterial : public virtual X3DOneSidedMaterialNode {
public:
  /**
   * @brief Default constructor for UnlitMaterial
   */
  UnlitMaterial() = default;

  /**
   * @brief Destructor for UnlitMaterial
   */
  ~UnlitMaterial() = default;

  /**
   * @brief Get the default value for emissiveColor
   * @return SFColor The default value
   */
  static SFColor getDefaultEmissiveColor() { return SFColor{1, 1, 1}; }

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
   * @brief The X3D type name of this node (e.g. "UnlitMaterial").
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
  static void checkRangesEmissiveColor(const SFColor &value,
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

  static void validateTransparency(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("transparency below minimum of 0");
    if (value > 1)
      throw std::out_of_range("transparency above maximum of 1");
  }

  /**
   * @brief Member variable for emissiveColor.
   */

  SFColor _emissiveColor{SFColor{1, 1, 1}};

  /**
   * @brief Member variable for emissiveTexture.
   */

  SFNode _emissiveTexture{nullptr};

  /**
   * @brief Member variable for normalTexture.
   */

  SFNode _normalTexture{nullptr};

  /**
   * @brief Member variable for transparency.
   */

  SFFloat _transparency{0};
};

#endif // UNLITMATERIAL_HPP