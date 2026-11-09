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
  AcousticProperties() = default;

  ~AcousticProperties() = default;

  static SFFloat getDefaultAbsorption() { return 0; }

  static SFFloat getDefaultDiffuse() { return 0; }

  static SFBool getDefaultEnabled() { return true; }

  static SFFloat getDefaultRefraction() { return 0; }

  static SFFloat getDefaultSpecular() { return 0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "acousticProperties"; }

  static std::string componentName() { return "Shape"; }

  static int componentLevel() { return 5; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details specifies the sound absorption coefficient of a surface, meaning
   * the ratio of sound intensity not reflected by a surface.
   */
  const SFFloat &getAbsorption() const { return _absorption; }

  /**
   * Access: inputOutput.
   * @details specifies the sound absorption coefficient of a surface, meaning
   * the ratio of sound intensity not reflected by a surface.
   */
  void setAbsorption(const SFFloat &value) {

    validateAbsorption(value);

    _absorption = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setAbsorption() for range validation.
   */
  void setAbsorptionUnchecked(const SFFloat &value) { _absorption = value; }

  /**
   * SFString; access: inputOutput. Borrows the stored value.
   * @details Author-provided prose that describes intended purpose of the url
   * asset.
   */
  const SFString &getDescription() const { return _description; }

  /**
   * Access: inputOutput.
   * @details Author-provided prose that describes intended purpose of the url
   * asset.
   */
  void setDescription(const SFString &value) { _description = value; }

  void setDescription(SFString &&value) { _description = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details diffuse coefficient of sound reflection indicates how much of the
   * incident sound energy is reflected back in multiple directions.
   */
  const SFFloat &getDiffuse() const { return _diffuse; }

  /**
   * Access: inputOutput.
   * @details diffuse coefficient of sound reflection indicates how much of the
   * incident sound energy is reflected back in multiple directions.
   */
  void setDiffuse(const SFFloat &value) {

    validateDiffuse(value);

    _diffuse = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setDiffuse() for range validation.
   */
  void setDiffuseUnchecked(const SFFloat &value) { _diffuse = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Enables/disables node operation.
   */
  const SFBool &getEnabled() const { return _enabled; }

  /**
   * Access: inputOutput.
   * @details Enables/disables node operation.
   */
  void setEnabled(const SFBool &value) { _enabled = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details sound refraction coefficient of a medium, which determines change
   * in propagation direction of sound wave when obliquely crossing boundary
   * between two mediums where its speed is different.
   */
  const SFFloat &getRefraction() const { return _refraction; }

  /**
   * Access: inputOutput.
   * @details sound refraction coefficient of a medium, which determines change
   * in propagation direction of sound wave when obliquely crossing boundary
   * between two mediums where its speed is different.
   */
  void setRefraction(const SFFloat &value) {

    validateRefraction(value);

    _refraction = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setRefraction() for range validation.
   */
  void setRefractionUnchecked(const SFFloat &value) { _refraction = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details specular coefficient of sound reflection striking a plane surface,
   * directly reflected back into space, where angle of reflection equals angle
   * of incidence.
   */
  const SFFloat &getSpecular() const { return _specular; }

  /**
   * Access: inputOutput.
   * @details specular coefficient of sound reflection striking a plane surface,
   * directly reflected back into space, where angle of reflection equals angle
   * of incidence.
   */
  void setSpecular(const SFFloat &value) {

    validateSpecular(value);

    _specular = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setSpecular() for range validation.
   */
  void setSpecularUnchecked(const SFFloat &value) { _specular = value; }

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

    if (value < 0.0f)
      throw std::out_of_range("absorption below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("absorption above maximum of 1");
  }

  static void validateDiffuse(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("diffuse below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("diffuse above maximum of 1");
  }

  static void validateRefraction(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("refraction below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("refraction above maximum of 1");
  }

  static void validateSpecular(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("specular below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("specular above maximum of 1");
  }

  SFFloat _absorption{0};

  SFString _description{};

  SFFloat _diffuse{0};

  SFBool _enabled{true};

  SFFloat _refraction{0};

  SFFloat _specular{0};
};

} // namespace x3d::nodes
