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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class TwoSidedMaterial
 * @brief TwoSidedMaterial specifies surface rendering properties for associated
 * geometry nodes, for outer (front) and inner (back) sides of polygons.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shape.html#TwoSidedMaterial
 */
class TwoSidedMaterial : public virtual X3DMaterialNode {
public:
  TwoSidedMaterial() = default;

  ~TwoSidedMaterial() = default;

  static SFFloat getDefaultAmbientIntensity() { return 0.2; }

  static SFFloat getDefaultBackAmbientIntensity() { return 0.2; }

  static SFColor getDefaultBackDiffuseColor() { return SFColor{0.8, 0.8, 0.8}; }

  static SFColor getDefaultBackEmissiveColor() { return SFColor{0, 0, 0}; }

  static SFFloat getDefaultBackShininess() { return 0.2; }

  static SFColor getDefaultBackSpecularColor() { return SFColor{0, 0, 0}; }

  static SFFloat getDefaultBackTransparency() { return 0; }

  static SFColor getDefaultDiffuseColor() { return SFColor{0.8, 0.8, 0.8}; }

  static SFColor getDefaultEmissiveColor() { return SFColor{0, 0, 0}; }

  static SFBool getDefaultSeparateBackColor() { return false; }

  static SFFloat getDefaultShininess() { return 0.2; }

  static SFColor getDefaultSpecularColor() { return SFColor{0, 0, 0}; }

  static SFFloat getDefaultTransparency() { return 0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "material"; }

  static std::string componentName() { return "Shape"; }

  static int componentLevel() { return 4; }

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
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details how much ambient omnidirectional light is reflected from all light
   * sources.
   */
  const SFFloat &getBackAmbientIntensity() const {
    return _backAmbientIntensity;
  }

  /**
   * Access: inputOutput.
   * @details how much ambient omnidirectional light is reflected from all light
   * sources.
   */
  void setBackAmbientIntensity(const SFFloat &value) {

    validateBackAmbientIntensity(value);

    _backAmbientIntensity = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setBackAmbientIntensity() for range validation.
   */
  void setBackAmbientIntensityUnchecked(const SFFloat &value) {
    _backAmbientIntensity = value;
  }

  /**
   * SFColor; access: inputOutput. Borrows the stored value.
   * @details how much direct, angle-dependent light is reflected from all light
   * sources.
   */
  const SFColor &getBackDiffuseColor() const { return _backDiffuseColor; }

  /**
   * Access: inputOutput.
   * @details how much direct, angle-dependent light is reflected from all light
   * sources.
   */
  void setBackDiffuseColor(const SFColor &value) {

    validateBackDiffuseColor(value);

    _backDiffuseColor = value;
  }

  void setBackDiffuseColor(SFColor &&value) {

    validateBackDiffuseColor(value);

    _backDiffuseColor = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setBackDiffuseColor() for range validation.
   */
  void setBackDiffuseColorUnchecked(const SFColor &value) {
    _backDiffuseColor = value;
  }

  /**
   * SFColor; access: inputOutput. Borrows the stored value.
   * @details how much glowing light is emitted from this object.
   */
  const SFColor &getBackEmissiveColor() const { return _backEmissiveColor; }

  /**
   * Access: inputOutput.
   * @details how much glowing light is emitted from this object.
   */
  void setBackEmissiveColor(const SFColor &value) {

    validateBackEmissiveColor(value);

    _backEmissiveColor = value;
  }

  void setBackEmissiveColor(SFColor &&value) {

    validateBackEmissiveColor(value);

    _backEmissiveColor = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setBackEmissiveColor() for range validation.
   */
  void setBackEmissiveColorUnchecked(const SFColor &value) {
    _backEmissiveColor = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Lower shininess values provide soft specular glows, while higher
   * values result in sharper, smaller highlights.
   */
  const SFFloat &getBackShininess() const { return _backShininess; }

  /**
   * Access: inputOutput.
   * @details Lower shininess values provide soft specular glows, while higher
   * values result in sharper, smaller highlights.
   */
  void setBackShininess(const SFFloat &value) {

    validateBackShininess(value);

    _backShininess = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setBackShininess() for range validation.
   */
  void setBackShininessUnchecked(const SFFloat &value) {
    _backShininess = value;
  }

  /**
   * SFColor; access: inputOutput. Borrows the stored value.
   * @details specular highlights are brightness reflections (example: shiny
   * spots on an apple).
   */
  const SFColor &getBackSpecularColor() const { return _backSpecularColor; }

  /**
   * Access: inputOutput.
   * @details specular highlights are brightness reflections (example: shiny
   * spots on an apple).
   */
  void setBackSpecularColor(const SFColor &value) {

    validateBackSpecularColor(value);

    _backSpecularColor = value;
  }

  void setBackSpecularColor(SFColor &&value) {

    validateBackSpecularColor(value);

    _backSpecularColor = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setBackSpecularColor() for range validation.
   */
  void setBackSpecularColorUnchecked(const SFColor &value) {
    _backSpecularColor = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details how "clear" an object is: 1.
   */
  const SFFloat &getBackTransparency() const { return _backTransparency; }

  /**
   * Access: inputOutput.
   * @details how "clear" an object is: 1.
   */
  void setBackTransparency(const SFFloat &value) {

    validateBackTransparency(value);

    _backTransparency = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setBackTransparency() for range validation.
   */
  void setBackTransparencyUnchecked(const SFFloat &value) {
    _backTransparency = value;
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
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details separateBackColor determines whether separate Material values are
   * used for back faces.
   */
  const SFBool &getSeparateBackColor() const { return _separateBackColor; }

  /**
   * Access: inputOutput.
   * @details separateBackColor determines whether separate Material values are
   * used for back faces.
   */
  void setSeparateBackColor(const SFBool &value) { _separateBackColor = value; }

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
  static void checkRangesBackAmbientIntensity(
      const SFFloat &value, const std::string &nodeType,
      const std::string &defName, std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesBackDiffuseColor(const SFColor &value,
                                          const std::string &nodeType,
                                          const std::string &defName,
                                          std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesBackEmissiveColor(const SFColor &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesBackShininess(const SFFloat &value,
                                       const std::string &nodeType,
                                       const std::string &defName,
                                       std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesBackSpecularColor(const SFColor &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesBackTransparency(const SFFloat &value,
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

  static void validateBackAmbientIntensity(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("backAmbientIntensity below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("backAmbientIntensity above maximum of 1");
  }

  static void validateBackDiffuseColor(const SFColor &value) {

    if (value.r < 0.0f)
      throw std::out_of_range("backDiffuseColor.r below minimum of 0");
    if (value.r > 1.0f)
      throw std::out_of_range("backDiffuseColor.r above maximum of 1");

    if (value.g < 0.0f)
      throw std::out_of_range("backDiffuseColor.g below minimum of 0");
    if (value.g > 1.0f)
      throw std::out_of_range("backDiffuseColor.g above maximum of 1");

    if (value.b < 0.0f)
      throw std::out_of_range("backDiffuseColor.b below minimum of 0");
    if (value.b > 1.0f)
      throw std::out_of_range("backDiffuseColor.b above maximum of 1");
  }

  static void validateBackEmissiveColor(const SFColor &value) {

    if (value.r < 0.0f)
      throw std::out_of_range("backEmissiveColor.r below minimum of 0");
    if (value.r > 1.0f)
      throw std::out_of_range("backEmissiveColor.r above maximum of 1");

    if (value.g < 0.0f)
      throw std::out_of_range("backEmissiveColor.g below minimum of 0");
    if (value.g > 1.0f)
      throw std::out_of_range("backEmissiveColor.g above maximum of 1");

    if (value.b < 0.0f)
      throw std::out_of_range("backEmissiveColor.b below minimum of 0");
    if (value.b > 1.0f)
      throw std::out_of_range("backEmissiveColor.b above maximum of 1");
  }

  static void validateBackShininess(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("backShininess below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("backShininess above maximum of 1");
  }

  static void validateBackSpecularColor(const SFColor &value) {

    if (value.r < 0.0f)
      throw std::out_of_range("backSpecularColor.r below minimum of 0");
    if (value.r > 1.0f)
      throw std::out_of_range("backSpecularColor.r above maximum of 1");

    if (value.g < 0.0f)
      throw std::out_of_range("backSpecularColor.g below minimum of 0");
    if (value.g > 1.0f)
      throw std::out_of_range("backSpecularColor.g above maximum of 1");

    if (value.b < 0.0f)
      throw std::out_of_range("backSpecularColor.b below minimum of 0");
    if (value.b > 1.0f)
      throw std::out_of_range("backSpecularColor.b above maximum of 1");
  }

  static void validateBackTransparency(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("backTransparency below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("backTransparency above maximum of 1");
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

  SFFloat _backAmbientIntensity{0.2};

  SFColor _backDiffuseColor{SFColor{0.8, 0.8, 0.8}};

  SFColor _backEmissiveColor{SFColor{0, 0, 0}};

  SFFloat _backShininess{0.2};

  SFColor _backSpecularColor{SFColor{0, 0, 0}};

  SFFloat _backTransparency{0};

  SFColor _diffuseColor{SFColor{0.8, 0.8, 0.8}};

  SFColor _emissiveColor{SFColor{0, 0, 0}};

  SFBool _separateBackColor{false};

  SFFloat _shininess{0.2};

  SFColor _specularColor{SFColor{0, 0, 0}};

  SFFloat _transparency{0};
};

} // namespace x3d::nodes
