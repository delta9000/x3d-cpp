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
 * @class UnlitMaterial
 * @brief UnlitMaterial specifies surface rendering properties for associated
 * geometry nodes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shape.html#UnlitMaterial
 */
class UnlitMaterial : public virtual X3DOneSidedMaterialNode {
public:
  UnlitMaterial() = default;

  ~UnlitMaterial() = default;

  static SFColor getDefaultEmissiveColor() { return SFColor{1, 1, 1}; }

  static SFNode getDefaultEmissiveTexture() { return nullptr; }

  static SFNode getDefaultNormalTexture() { return nullptr; }

  static SFFloat getDefaultTransparency() { return 0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "material"; }

  static std::string componentName() { return "Shape"; }

  static int componentLevel() { return 1; }

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

  static void validateTransparency(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("transparency below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("transparency above maximum of 1");
  }

  SFColor _emissiveColor{SFColor{1, 1, 1}};

  SFNode _emissiveTexture{nullptr};

  SFNode _normalTexture{nullptr};

  SFFloat _transparency{0};
};

} // namespace x3d::nodes
