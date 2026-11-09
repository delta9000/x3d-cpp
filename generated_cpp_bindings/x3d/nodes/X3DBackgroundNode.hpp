#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DChildNode.hpp"

#include "x3d/nodes/X3DBindableNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DBackgroundNode
 * @brief Abstract type from which all backgrounds inherit, also defining a
 * background binding stack.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalEffects.html#X3DBackgroundNode
 */
class X3DBackgroundNode : public virtual X3DBindableNode {
public:
  X3DBackgroundNode() = default;

  virtual ~X3DBackgroundNode() = default;

  static MFColor getDefaultSkyColor() {
    return std::vector<SFColor>{SFColor{0, 0, 0}};
  }

  static SFFloat getDefaultTransparency() { return 0; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "EnvironmentalEffects"; }

  static int componentLevel() { return 1; }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details
   */
  const MFFloat &getGroundAngle() const { return _groundAngle; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setGroundAngle(const MFFloat &value) {

    validateGroundAngle(value);

    _groundAngle = value;
  }

  void setGroundAngle(MFFloat &&value) {

    validateGroundAngle(value);

    _groundAngle = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setGroundAngle() for range validation.
   */
  void setGroundAngleUnchecked(const MFFloat &value) { _groundAngle = value; }

  /**
   * MFColor; access: inputOutput. Borrows the stored value.
   * @details
   */
  const MFColor &getGroundColor() const { return _groundColor; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setGroundColor(const MFColor &value) {

    validateGroundColor(value);

    _groundColor = value;
  }

  void setGroundColor(MFColor &&value) {

    validateGroundColor(value);

    _groundColor = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setGroundColor() for range validation.
   */
  void setGroundColorUnchecked(const MFColor &value) { _groundColor = value; }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details
   */
  const MFFloat &getSkyAngle() const { return _skyAngle; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setSkyAngle(const MFFloat &value) {

    validateSkyAngle(value);

    _skyAngle = value;
  }

  void setSkyAngle(MFFloat &&value) {

    validateSkyAngle(value);

    _skyAngle = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setSkyAngle() for range validation.
   */
  void setSkyAngleUnchecked(const MFFloat &value) { _skyAngle = value; }

  /**
   * MFColor; access: inputOutput. Borrows the stored value.
   * @details
   */
  const MFColor &getSkyColor() const { return _skyColor; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setSkyColor(const MFColor &value) {

    validateSkyColor(value);

    _skyColor = value;
  }

  void setSkyColor(MFColor &&value) {

    validateSkyColor(value);

    _skyColor = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setSkyColor() for range validation.
   */
  void setSkyColorUnchecked(const MFColor &value) { _skyColor = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFFloat &getTransparency() const { return _transparency; }

  /**
   * Access: inputOutput.
   * @details
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
  static void checkRangesGroundAngle(const MFFloat &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesGroundColor(const MFColor &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesSkyAngle(const MFFloat &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesSkyColor(const MFColor &value,
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
  static void validateGroundAngle(const MFFloat &value) {

    for (const auto &v : value) {

      if (v < 0.0f)
        throw std::out_of_range("groundAngle below minimum of 0");
      if (v > 1.5708f)
        throw std::out_of_range("groundAngle above maximum of 1.5708");
    }
  }

  static void validateGroundColor(const MFColor &value) {

    for (const auto &v : value) {

      if (v.r < 0.0f)
        throw std::out_of_range("groundColor.r below minimum of 0");
      if (v.r > 1.0f)
        throw std::out_of_range("groundColor.r above maximum of 1");

      if (v.g < 0.0f)
        throw std::out_of_range("groundColor.g below minimum of 0");
      if (v.g > 1.0f)
        throw std::out_of_range("groundColor.g above maximum of 1");

      if (v.b < 0.0f)
        throw std::out_of_range("groundColor.b below minimum of 0");
      if (v.b > 1.0f)
        throw std::out_of_range("groundColor.b above maximum of 1");
    }
  }

  static void validateSkyAngle(const MFFloat &value) {

    for (const auto &v : value) {

      if (v < 0.0f)
        throw std::out_of_range("skyAngle below minimum of 0");
      if (v > 3.1416f)
        throw std::out_of_range("skyAngle above maximum of 3.1416");
    }
  }

  static void validateSkyColor(const MFColor &value) {

    for (const auto &v : value) {

      if (v.r < 0.0f)
        throw std::out_of_range("skyColor.r below minimum of 0");
      if (v.r > 1.0f)
        throw std::out_of_range("skyColor.r above maximum of 1");

      if (v.g < 0.0f)
        throw std::out_of_range("skyColor.g below minimum of 0");
      if (v.g > 1.0f)
        throw std::out_of_range("skyColor.g above maximum of 1");

      if (v.b < 0.0f)
        throw std::out_of_range("skyColor.b below minimum of 0");
      if (v.b > 1.0f)
        throw std::out_of_range("skyColor.b above maximum of 1");
    }
  }

  static void validateTransparency(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("transparency below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("transparency above maximum of 1");
  }

  MFFloat _groundAngle{};

  MFColor _groundColor{};

  MFFloat _skyAngle{};

  MFColor _skyColor{std::vector<SFColor>{SFColor{0, 0, 0}}};

  SFFloat _transparency{0};
};

} // namespace x3d::nodes
