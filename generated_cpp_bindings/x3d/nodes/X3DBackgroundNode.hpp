// X3DBackgroundNode.hpp
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
  /**
   * @brief Default constructor for X3DBackgroundNode
   */
  X3DBackgroundNode() = default;

  /**
   * @brief Virtual destructor for X3DBackgroundNode
   */
  virtual ~X3DBackgroundNode() = default;

  /**
   * @brief Get the default value for skyColor
   * @return MFColor The default value
   */
  static MFColor getDefaultSkyColor() {
    return std::vector<SFColor>{SFColor{0, 0, 0}};
  }

  /**
   * @brief Get the default value for transparency
   * @return SFFloat The default value
   */
  static SFFloat getDefaultTransparency() { return 0; }

  /**
   * @brief Get the default container field value
   * @details No explicit containerField in the spec for this node; falls back
   *          to the X3D default field "children".
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "EnvironmentalEffects"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of groundAngle. AccessType: inputOutput
   * @details
   * @return MFFloat The current value of groundAngle.
   */
  MFFloat getGroundAngle() const { return _groundAngle; }

  /**
   * @brief Sets the value of groundAngle. AccessType: inputOutput
   * @details
   * @param value The new value for groundAngle.
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
   * @brief Non-validating write of groundAngle (runtime/reader ingest path).
   * @details Assigns without the range check setGroundAngle() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setGroundAngle() stays the
   *          enforcement point for programmatic callers.
   */
  void setGroundAngleUnchecked(const MFFloat &value) { _groundAngle = value; }

  /**
   * @brief Gets the value of groundColor. AccessType: inputOutput
   * @details
   * @return MFColor The current value of groundColor.
   */
  MFColor getGroundColor() const { return _groundColor; }

  /**
   * @brief Sets the value of groundColor. AccessType: inputOutput
   * @details
   * @param value The new value for groundColor.
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
   * @brief Non-validating write of groundColor (runtime/reader ingest path).
   * @details Assigns without the range check setGroundColor() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setGroundColor() stays the
   *          enforcement point for programmatic callers.
   */
  void setGroundColorUnchecked(const MFColor &value) { _groundColor = value; }

  /**
   * @brief Gets the value of skyAngle. AccessType: inputOutput
   * @details
   * @return MFFloat The current value of skyAngle.
   */
  MFFloat getSkyAngle() const { return _skyAngle; }

  /**
   * @brief Sets the value of skyAngle. AccessType: inputOutput
   * @details
   * @param value The new value for skyAngle.
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
   * @brief Non-validating write of skyAngle (runtime/reader ingest path).
   * @details Assigns without the range check setSkyAngle() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setSkyAngle() stays the
   *          enforcement point for programmatic callers.
   */
  void setSkyAngleUnchecked(const MFFloat &value) { _skyAngle = value; }

  /**
   * @brief Gets the value of skyColor. AccessType: inputOutput
   * @details
   * @return MFColor The current value of skyColor.
   */
  MFColor getSkyColor() const { return _skyColor; }

  /**
   * @brief Sets the value of skyColor. AccessType: inputOutput
   * @details
   * @param value The new value for skyColor.
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
   * @brief Non-validating write of skyColor (runtime/reader ingest path).
   * @details Assigns without the range check setSkyColor() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setSkyColor() stays the
   *          enforcement point for programmatic callers.
   */
  void setSkyColorUnchecked(const MFColor &value) { _skyColor = value; }

  /**
   * @brief Gets the value of transparency. AccessType: inputOutput
   * @details
   * @return SFFloat The current value of transparency.
   */
  SFFloat getTransparency() const { return _transparency; }

  /**
   * @brief Sets the value of transparency. AccessType: inputOutput
   * @details
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
   * @brief The X3D type name of this node (e.g. "X3DBackgroundNode").
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

      if (v < 0)
        throw std::out_of_range("groundAngle below minimum of 0");
      if (v > 1.5708)
        throw std::out_of_range("groundAngle above maximum of 1.5708");
    }
  }

  static void validateGroundColor(const MFColor &value) {

    for (const auto &v : value) {

      if (v.r < 0)
        throw std::out_of_range("groundColor.r below minimum of 0");
      if (v.r > 1)
        throw std::out_of_range("groundColor.r above maximum of 1");

      if (v.g < 0)
        throw std::out_of_range("groundColor.g below minimum of 0");
      if (v.g > 1)
        throw std::out_of_range("groundColor.g above maximum of 1");

      if (v.b < 0)
        throw std::out_of_range("groundColor.b below minimum of 0");
      if (v.b > 1)
        throw std::out_of_range("groundColor.b above maximum of 1");
    }
  }

  static void validateSkyAngle(const MFFloat &value) {

    for (const auto &v : value) {

      if (v < 0)
        throw std::out_of_range("skyAngle below minimum of 0");
      if (v > 3.1416)
        throw std::out_of_range("skyAngle above maximum of 3.1416");
    }
  }

  static void validateSkyColor(const MFColor &value) {

    for (const auto &v : value) {

      if (v.r < 0)
        throw std::out_of_range("skyColor.r below minimum of 0");
      if (v.r > 1)
        throw std::out_of_range("skyColor.r above maximum of 1");

      if (v.g < 0)
        throw std::out_of_range("skyColor.g below minimum of 0");
      if (v.g > 1)
        throw std::out_of_range("skyColor.g above maximum of 1");

      if (v.b < 0)
        throw std::out_of_range("skyColor.b below minimum of 0");
      if (v.b > 1)
        throw std::out_of_range("skyColor.b above maximum of 1");
    }
  }

  static void validateTransparency(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("transparency below minimum of 0");
    if (value > 1)
      throw std::out_of_range("transparency above maximum of 1");
  }

  /**
   * @brief Member variable for groundAngle.
   */

  MFFloat _groundAngle{};

  /**
   * @brief Member variable for groundColor.
   */

  MFColor _groundColor{};

  /**
   * @brief Member variable for skyAngle.
   */

  MFFloat _skyAngle{};

  /**
   * @brief Member variable for skyColor.
   */

  MFColor _skyColor{std::vector<SFColor>{SFColor{0, 0, 0}}};

  /**
   * @brief Member variable for transparency.
   */

  SFFloat _transparency{0};
};

} // namespace x3d::nodes
