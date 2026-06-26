// TwoSidedMaterial.hpp
#ifndef TWOSIDEDMATERIAL_HPP
#define TWOSIDEDMATERIAL_HPP

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

/**
 * @class TwoSidedMaterial
 * @brief TwoSidedMaterial specifies surface rendering properties for associated
 * geometry nodes, for outer (front) and inner (back) sides of polygons.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shape.html#TwoSidedMaterial
 */
class TwoSidedMaterial : public virtual X3DMaterialNode {
public:
  /**
   * @brief Default constructor for TwoSidedMaterial
   */
  TwoSidedMaterial() = default;

  /**
   * @brief Destructor for TwoSidedMaterial
   */
  ~TwoSidedMaterial() = default;

  /**
   * @brief Get the default value for ambientIntensity
   * @return SFFloat The default value
   */
  static SFFloat getDefaultAmbientIntensity() { return 0.2; }

  /**
   * @brief Get the default value for backAmbientIntensity
   * @return SFFloat The default value
   */
  static SFFloat getDefaultBackAmbientIntensity() { return 0.2; }

  /**
   * @brief Get the default value for backDiffuseColor
   * @return SFColor The default value
   */
  static SFColor getDefaultBackDiffuseColor() { return SFColor{0.8, 0.8, 0.8}; }

  /**
   * @brief Get the default value for backEmissiveColor
   * @return SFColor The default value
   */
  static SFColor getDefaultBackEmissiveColor() { return SFColor{0, 0, 0}; }

  /**
   * @brief Get the default value for backShininess
   * @return SFFloat The default value
   */
  static SFFloat getDefaultBackShininess() { return 0.2; }

  /**
   * @brief Get the default value for backSpecularColor
   * @return SFColor The default value
   */
  static SFColor getDefaultBackSpecularColor() { return SFColor{0, 0, 0}; }

  /**
   * @brief Get the default value for backTransparency
   * @return SFFloat The default value
   */
  static SFFloat getDefaultBackTransparency() { return 0; }

  /**
   * @brief Get the default value for diffuseColor
   * @return SFColor The default value
   */
  static SFColor getDefaultDiffuseColor() { return SFColor{0.8, 0.8, 0.8}; }

  /**
   * @brief Get the default value for emissiveColor
   * @return SFColor The default value
   */
  static SFColor getDefaultEmissiveColor() { return SFColor{0, 0, 0}; }

  /**
   * @brief Get the default value for separateBackColor
   * @return SFBool The default value
   */
  static SFBool getDefaultSeparateBackColor() { return false; }

  /**
   * @brief Get the default value for shininess
   * @return SFFloat The default value
   */
  static SFFloat getDefaultShininess() { return 0.2; }

  /**
   * @brief Get the default value for specularColor
   * @return SFColor The default value
   */
  static SFColor getDefaultSpecularColor() { return SFColor{0, 0, 0}; }

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
  static int componentLevel() { return 4; }

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
   * @brief Gets the value of backAmbientIntensity. AccessType: inputOutput
   * @details how much ambient omnidirectional light is reflected from all light
   * sources.
   * @return SFFloat The current value of backAmbientIntensity.
   */
  SFFloat getBackAmbientIntensity() const { return _backAmbientIntensity; }

  /**
   * @brief Sets the value of backAmbientIntensity. AccessType: inputOutput
   * @details how much ambient omnidirectional light is reflected from all light
   * sources.
   * @param value The new value for backAmbientIntensity.
   */
  void setBackAmbientIntensity(const SFFloat &value) {

    validateBackAmbientIntensity(value);

    _backAmbientIntensity = value;
  }

  /**
   * @brief Non-validating write of backAmbientIntensity (runtime/reader ingest
   * path).
   * @details Assigns without the range check setBackAmbientIntensity()
   * enforces, so a permissive reader keeps an out-of-range authored value
   * rather than reject the whole document. setBackAmbientIntensity() stays the
   *          enforcement point for programmatic callers.
   */
  void setBackAmbientIntensityUnchecked(const SFFloat &value) {
    _backAmbientIntensity = value;
  }

  /**
   * @brief Gets the value of backDiffuseColor. AccessType: inputOutput
   * @details how much direct, angle-dependent light is reflected from all light
   * sources.
   * @return SFColor The current value of backDiffuseColor.
   */
  SFColor getBackDiffuseColor() const { return _backDiffuseColor; }

  /**
   * @brief Sets the value of backDiffuseColor. AccessType: inputOutput
   * @details how much direct, angle-dependent light is reflected from all light
   * sources.
   * @param value The new value for backDiffuseColor.
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
   * @brief Non-validating write of backDiffuseColor (runtime/reader ingest
   * path).
   * @details Assigns without the range check setBackDiffuseColor() enforces, so
   * a permissive reader keeps an out-of-range authored value rather than reject
   * the whole document. setBackDiffuseColor() stays the enforcement point for
   * programmatic callers.
   */
  void setBackDiffuseColorUnchecked(const SFColor &value) {
    _backDiffuseColor = value;
  }

  /**
   * @brief Gets the value of backEmissiveColor. AccessType: inputOutput
   * @details how much glowing light is emitted from this object.
   * @return SFColor The current value of backEmissiveColor.
   */
  SFColor getBackEmissiveColor() const { return _backEmissiveColor; }

  /**
   * @brief Sets the value of backEmissiveColor. AccessType: inputOutput
   * @details how much glowing light is emitted from this object.
   * @param value The new value for backEmissiveColor.
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
   * @brief Non-validating write of backEmissiveColor (runtime/reader ingest
   * path).
   * @details Assigns without the range check setBackEmissiveColor() enforces,
   * so a permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setBackEmissiveColor() stays the
   *          enforcement point for programmatic callers.
   */
  void setBackEmissiveColorUnchecked(const SFColor &value) {
    _backEmissiveColor = value;
  }

  /**
   * @brief Gets the value of backShininess. AccessType: inputOutput
   * @details Lower shininess values provide soft specular glows, while higher
   * values result in sharper, smaller highlights.
   * @return SFFloat The current value of backShininess.
   */
  SFFloat getBackShininess() const { return _backShininess; }

  /**
   * @brief Sets the value of backShininess. AccessType: inputOutput
   * @details Lower shininess values provide soft specular glows, while higher
   * values result in sharper, smaller highlights.
   * @param value The new value for backShininess.
   */
  void setBackShininess(const SFFloat &value) {

    validateBackShininess(value);

    _backShininess = value;
  }

  /**
   * @brief Non-validating write of backShininess (runtime/reader ingest path).
   * @details Assigns without the range check setBackShininess() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setBackShininess() stays the
   *          enforcement point for programmatic callers.
   */
  void setBackShininessUnchecked(const SFFloat &value) {
    _backShininess = value;
  }

  /**
   * @brief Gets the value of backSpecularColor. AccessType: inputOutput
   * @details specular highlights are brightness reflections (example: shiny
   * spots on an apple).
   * @return SFColor The current value of backSpecularColor.
   */
  SFColor getBackSpecularColor() const { return _backSpecularColor; }

  /**
   * @brief Sets the value of backSpecularColor. AccessType: inputOutput
   * @details specular highlights are brightness reflections (example: shiny
   * spots on an apple).
   * @param value The new value for backSpecularColor.
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
   * @brief Non-validating write of backSpecularColor (runtime/reader ingest
   * path).
   * @details Assigns without the range check setBackSpecularColor() enforces,
   * so a permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setBackSpecularColor() stays the
   *          enforcement point for programmatic callers.
   */
  void setBackSpecularColorUnchecked(const SFColor &value) {
    _backSpecularColor = value;
  }

  /**
   * @brief Gets the value of backTransparency. AccessType: inputOutput
   * @details how "clear" an object is: 1.
   * @return SFFloat The current value of backTransparency.
   */
  SFFloat getBackTransparency() const { return _backTransparency; }

  /**
   * @brief Sets the value of backTransparency. AccessType: inputOutput
   * @details how "clear" an object is: 1.
   * @param value The new value for backTransparency.
   */
  void setBackTransparency(const SFFloat &value) {

    validateBackTransparency(value);

    _backTransparency = value;
  }

  /**
   * @brief Non-validating write of backTransparency (runtime/reader ingest
   * path).
   * @details Assigns without the range check setBackTransparency() enforces, so
   * a permissive reader keeps an out-of-range authored value rather than reject
   * the whole document. setBackTransparency() stays the enforcement point for
   * programmatic callers.
   */
  void setBackTransparencyUnchecked(const SFFloat &value) {
    _backTransparency = value;
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
   * @brief Gets the value of separateBackColor. AccessType: inputOutput
   * @details separateBackColor determines whether separate Material values are
   * used for back faces.
   * @return SFBool The current value of separateBackColor.
   */
  SFBool getSeparateBackColor() const { return _separateBackColor; }

  /**
   * @brief Sets the value of separateBackColor. AccessType: inputOutput
   * @details separateBackColor determines whether separate Material values are
   * used for back faces.
   * @param value The new value for separateBackColor.
   */
  void setSeparateBackColor(const SFBool &value) { _separateBackColor = value; }

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
   * @brief The X3D type name of this node (e.g. "TwoSidedMaterial").
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

    if (value < 0)
      throw std::out_of_range("ambientIntensity below minimum of 0");
    if (value > 1)
      throw std::out_of_range("ambientIntensity above maximum of 1");
  }

  static void validateBackAmbientIntensity(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("backAmbientIntensity below minimum of 0");
    if (value > 1)
      throw std::out_of_range("backAmbientIntensity above maximum of 1");
  }

  static void validateBackDiffuseColor(const SFColor &value) {

    if (value.r < 0)
      throw std::out_of_range("backDiffuseColor.r below minimum of 0");
    if (value.r > 1)
      throw std::out_of_range("backDiffuseColor.r above maximum of 1");

    if (value.g < 0)
      throw std::out_of_range("backDiffuseColor.g below minimum of 0");
    if (value.g > 1)
      throw std::out_of_range("backDiffuseColor.g above maximum of 1");

    if (value.b < 0)
      throw std::out_of_range("backDiffuseColor.b below minimum of 0");
    if (value.b > 1)
      throw std::out_of_range("backDiffuseColor.b above maximum of 1");
  }

  static void validateBackEmissiveColor(const SFColor &value) {

    if (value.r < 0)
      throw std::out_of_range("backEmissiveColor.r below minimum of 0");
    if (value.r > 1)
      throw std::out_of_range("backEmissiveColor.r above maximum of 1");

    if (value.g < 0)
      throw std::out_of_range("backEmissiveColor.g below minimum of 0");
    if (value.g > 1)
      throw std::out_of_range("backEmissiveColor.g above maximum of 1");

    if (value.b < 0)
      throw std::out_of_range("backEmissiveColor.b below minimum of 0");
    if (value.b > 1)
      throw std::out_of_range("backEmissiveColor.b above maximum of 1");
  }

  static void validateBackShininess(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("backShininess below minimum of 0");
    if (value > 1)
      throw std::out_of_range("backShininess above maximum of 1");
  }

  static void validateBackSpecularColor(const SFColor &value) {

    if (value.r < 0)
      throw std::out_of_range("backSpecularColor.r below minimum of 0");
    if (value.r > 1)
      throw std::out_of_range("backSpecularColor.r above maximum of 1");

    if (value.g < 0)
      throw std::out_of_range("backSpecularColor.g below minimum of 0");
    if (value.g > 1)
      throw std::out_of_range("backSpecularColor.g above maximum of 1");

    if (value.b < 0)
      throw std::out_of_range("backSpecularColor.b below minimum of 0");
    if (value.b > 1)
      throw std::out_of_range("backSpecularColor.b above maximum of 1");
  }

  static void validateBackTransparency(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("backTransparency below minimum of 0");
    if (value > 1)
      throw std::out_of_range("backTransparency above maximum of 1");
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
   * @brief Member variable for backAmbientIntensity.
   */

  SFFloat _backAmbientIntensity{0.2};

  /**
   * @brief Member variable for backDiffuseColor.
   */

  SFColor _backDiffuseColor{SFColor{0.8, 0.8, 0.8}};

  /**
   * @brief Member variable for backEmissiveColor.
   */

  SFColor _backEmissiveColor{SFColor{0, 0, 0}};

  /**
   * @brief Member variable for backShininess.
   */

  SFFloat _backShininess{0.2};

  /**
   * @brief Member variable for backSpecularColor.
   */

  SFColor _backSpecularColor{SFColor{0, 0, 0}};

  /**
   * @brief Member variable for backTransparency.
   */

  SFFloat _backTransparency{0};

  /**
   * @brief Member variable for diffuseColor.
   */

  SFColor _diffuseColor{SFColor{0.8, 0.8, 0.8}};

  /**
   * @brief Member variable for emissiveColor.
   */

  SFColor _emissiveColor{SFColor{0, 0, 0}};

  /**
   * @brief Member variable for separateBackColor.
   */

  SFBool _separateBackColor{false};

  /**
   * @brief Member variable for shininess.
   */

  SFFloat _shininess{0.2};

  /**
   * @brief Member variable for specularColor.
   */

  SFColor _specularColor{SFColor{0, 0, 0}};

  /**
   * @brief Member variable for transparency.
   */

  SFFloat _transparency{0};
};

#endif // TWOSIDEDMATERIAL_HPP