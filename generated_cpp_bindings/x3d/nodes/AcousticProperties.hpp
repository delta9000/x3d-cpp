// AcousticProperties.hpp
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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class AcousticProperties
 * @brief AcousticProperties specifies the interaction of sound waves with
 * characteristics of geometric objects in the scene.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shape.html#AcousticProperties
 */
class AcousticProperties : public virtual X3DAppearanceChildNode {
public:
  /**
   * @brief Default constructor for AcousticProperties
   */
  AcousticProperties() = default;

  /**
   * @brief Destructor for AcousticProperties
   */
  ~AcousticProperties() = default;

  /**
   * @brief Get the default value for absorption
   * @return SFFloat The default value
   */
  static SFFloat getDefaultAbsorption() { return 0; }

  /**
   * @brief Get the default value for diffuse
   * @return SFFloat The default value
   */
  static SFFloat getDefaultDiffuse() { return 0; }

  /**
   * @brief Get the default value for enabled
   * @return SFBool The default value
   */
  static SFBool getDefaultEnabled() { return true; }

  /**
   * @brief Get the default value for refraction
   * @return SFFloat The default value
   */
  static SFFloat getDefaultRefraction() { return 0; }

  /**
   * @brief Get the default value for specular
   * @return SFFloat The default value
   */
  static SFFloat getDefaultSpecular() { return 0; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "acousticProperties"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Shape"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 5; }

  /**
   * @brief Gets the value of absorption. AccessType: inputOutput
   * @details specifies the sound absorption coefficient of a surface, meaning
   * the ratio of sound intensity not reflected by a surface.
   * @return SFFloat The current value of absorption.
   */
  SFFloat getAbsorption() const { return _absorption; }

  /**
   * @brief Sets the value of absorption. AccessType: inputOutput
   * @details specifies the sound absorption coefficient of a surface, meaning
   * the ratio of sound intensity not reflected by a surface.
   * @param value The new value for absorption.
   */
  void setAbsorption(const SFFloat &value) {

    validateAbsorption(value);

    _absorption = value;
  }

  /**
   * @brief Non-validating write of absorption (runtime/reader ingest path).
   * @details Assigns without the range check setAbsorption() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setAbsorption() stays the
   *          enforcement point for programmatic callers.
   */
  void setAbsorptionUnchecked(const SFFloat &value) { _absorption = value; }

  /**
   * @brief Gets the value of description. AccessType: inputOutput
   * @details Author-provided prose that describes intended purpose of the url
   * asset.
   * @return SFString The current value of description.
   */
  SFString getDescription() const { return _description; }

  /**
   * @brief Sets the value of description. AccessType: inputOutput
   * @details Author-provided prose that describes intended purpose of the url
   * asset.
   * @param value The new value for description.
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

  /**
   * @brief Gets the value of diffuse. AccessType: inputOutput
   * @details diffuse coefficient of sound reflection indicates how much of the
   * incident sound energy is reflected back in multiple directions.
   * @return SFFloat The current value of diffuse.
   */
  SFFloat getDiffuse() const { return _diffuse; }

  /**
   * @brief Sets the value of diffuse. AccessType: inputOutput
   * @details diffuse coefficient of sound reflection indicates how much of the
   * incident sound energy is reflected back in multiple directions.
   * @param value The new value for diffuse.
   */
  void setDiffuse(const SFFloat &value) {

    validateDiffuse(value);

    _diffuse = value;
  }

  /**
   * @brief Non-validating write of diffuse (runtime/reader ingest path).
   * @details Assigns without the range check setDiffuse() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setDiffuse() stays the
   *          enforcement point for programmatic callers.
   */
  void setDiffuseUnchecked(const SFFloat &value) { _diffuse = value; }

  /**
   * @brief Gets the value of enabled. AccessType: inputOutput
   * @details Enables/disables node operation.
   * @return SFBool The current value of enabled.
   */
  SFBool getEnabled() const { return _enabled; }

  /**
   * @brief Sets the value of enabled. AccessType: inputOutput
   * @details Enables/disables node operation.
   * @param value The new value for enabled.
   */
  void setEnabled(const SFBool &value) { _enabled = value; }

  /**
   * @brief Gets the value of refraction. AccessType: inputOutput
   * @details sound refraction coefficient of a medium, which determines change
   * in propagation direction of sound wave when obliquely crossing boundary
   * between two mediums where its speed is different.
   * @return SFFloat The current value of refraction.
   */
  SFFloat getRefraction() const { return _refraction; }

  /**
   * @brief Sets the value of refraction. AccessType: inputOutput
   * @details sound refraction coefficient of a medium, which determines change
   * in propagation direction of sound wave when obliquely crossing boundary
   * between two mediums where its speed is different.
   * @param value The new value for refraction.
   */
  void setRefraction(const SFFloat &value) {

    validateRefraction(value);

    _refraction = value;
  }

  /**
   * @brief Non-validating write of refraction (runtime/reader ingest path).
   * @details Assigns without the range check setRefraction() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setRefraction() stays the
   *          enforcement point for programmatic callers.
   */
  void setRefractionUnchecked(const SFFloat &value) { _refraction = value; }

  /**
   * @brief Gets the value of specular. AccessType: inputOutput
   * @details specular coefficient of sound reflection striking a plane surface,
   * directly reflected back into space, where angle of reflection equals angle
   * of incidence.
   * @return SFFloat The current value of specular.
   */
  SFFloat getSpecular() const { return _specular; }

  /**
   * @brief Sets the value of specular. AccessType: inputOutput
   * @details specular coefficient of sound reflection striking a plane surface,
   * directly reflected back into space, where angle of reflection equals angle
   * of incidence.
   * @param value The new value for specular.
   */
  void setSpecular(const SFFloat &value) {

    validateSpecular(value);

    _specular = value;
  }

  /**
   * @brief Non-validating write of specular (runtime/reader ingest path).
   * @details Assigns without the range check setSpecular() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setSpecular() stays the
   *          enforcement point for programmatic callers.
   */
  void setSpecularUnchecked(const SFFloat &value) { _specular = value; }

  /**
   * @brief The X3D type name of this node (e.g. "AcousticProperties").
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
  static void checkRangesAbsorption(const SFFloat &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesDiffuse(const SFFloat &value,
                                 const std::string &nodeType,
                                 const std::string &defName,
                                 std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesRefraction(const SFFloat &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesSpecular(const SFFloat &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out);

private:
  static void validateAbsorption(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("absorption below minimum of 0");
    if (value > 1)
      throw std::out_of_range("absorption above maximum of 1");
  }

  static void validateDiffuse(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("diffuse below minimum of 0");
    if (value > 1)
      throw std::out_of_range("diffuse above maximum of 1");
  }

  static void validateRefraction(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("refraction below minimum of 0");
    if (value > 1)
      throw std::out_of_range("refraction above maximum of 1");
  }

  static void validateSpecular(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("specular below minimum of 0");
    if (value > 1)
      throw std::out_of_range("specular above maximum of 1");
  }

  /**
   * @brief Member variable for absorption.
   */

  SFFloat _absorption{0};

  /**
   * @brief Member variable for description.
   */

  SFString _description{};

  /**
   * @brief Member variable for diffuse.
   */

  SFFloat _diffuse{0};

  /**
   * @brief Member variable for enabled.
   */

  SFBool _enabled{true};

  /**
   * @brief Member variable for refraction.
   */

  SFFloat _refraction{0};

  /**
   * @brief Member variable for specular.
   */

  SFFloat _specular{0};
};

} // namespace x3d::nodes
