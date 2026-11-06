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

#include "x3d/nodes/X3DLightNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class SpotLight
 * @brief Linear attenuation may occur at level 2, full support at level 3.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/lighting.html#SpotLight
 */
class SpotLight : public virtual X3DLightNode {
public:
  SpotLight() = default;

  ~SpotLight() = default;

  static SFVec3f getDefaultAttenuation() { return SFVec3f{1, 0, 0}; }

  static SFFloat getDefaultBeamWidth() { return 0.589049; }

  static SFFloat getDefaultCutOffAngle() { return 1.570796; }

  static SFVec3f getDefaultDirection() { return SFVec3f{0, 0, -1}; }

  static SFBool getDefaultGlobal() { return true; }

  static SFVec3f getDefaultLocation() { return SFVec3f{0, 0, 0}; }

  static SFFloat getDefaultRadius() { return 100; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Lighting"; }

  static int componentLevel() { return 2; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Constant, linear-distance and squared-distance dropoff factors as
   * radial distance increases from the source.
   */
  const SFVec3f &getAttenuation() const { return _attenuation; }

  /**
   * Access: inputOutput.
   * @details Constant, linear-distance and squared-distance dropoff factors as
   * radial distance increases from the source.
   */
  void setAttenuation(const SFVec3f &value) {

    validateAttenuation(value);

    _attenuation = value;
  }

  void setAttenuation(SFVec3f &&value) {

    validateAttenuation(value);

    _attenuation = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setAttenuation() for range validation.
   */
  void setAttenuationUnchecked(const SFVec3f &value) { _attenuation = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Inner conical solid angle (in radians) where light source has
   * uniform full intensity.
   */
  const SFFloat &getBeamWidth() const { return _beamWidth; }

  /**
   * Access: inputOutput.
   * @details Inner conical solid angle (in radians) where light source has
   * uniform full intensity.
   */
  void setBeamWidth(const SFFloat &value) {

    validateBeamWidth(value);

    _beamWidth = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setBeamWidth() for range validation.
   */
  void setBeamWidthUnchecked(const SFFloat &value) { _beamWidth = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Outer conical solid angle (in radians) where light source
   * intensity becomes zero.
   */
  const SFFloat &getCutOffAngle() const { return _cutOffAngle; }

  /**
   * Access: inputOutput.
   * @details Outer conical solid angle (in radians) where light source
   * intensity becomes zero.
   */
  void setCutOffAngle(const SFFloat &value) {

    validateCutOffAngle(value);

    _cutOffAngle = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setCutOffAngle() for range validation.
   */
  void setCutOffAngleUnchecked(const SFFloat &value) { _cutOffAngle = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Orientation vector of light relative to local coordinate system.
   */
  const SFVec3f &getDirection() const { return _direction; }

  /**
   * Access: inputOutput.
   * @details Orientation vector of light relative to local coordinate system.
   */
  void setDirection(const SFVec3f &value) { _direction = value; }

  void setDirection(SFVec3f &&value) { _direction = std::move(value); }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Global lights illuminate all objects within their volume of
   * lighting influence.
   */
  const SFBool &getGlobal() const { return _global; }

  /**
   * Access: inputOutput.
   * @details Global lights illuminate all objects within their volume of
   * lighting influence.
   */
  void setGlobal(const SFBool &value) { _global = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Position of light relative to local coordinate system.
   */
  const SFVec3f &getLocation() const { return _location; }

  /**
   * Access: inputOutput.
   * @details Position of light relative to local coordinate system.
   */
  void setLocation(const SFVec3f &value) { _location = value; }

  void setLocation(SFVec3f &&value) { _location = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Maximum effective distance of light relative to local light
   * position, affected by ancestor scaling.
   */
  const SFFloat &getRadius() const { return _radius; }

  /**
   * Access: inputOutput.
   * @details Maximum effective distance of light relative to local light
   * position, affected by ancestor scaling.
   */
  void setRadius(const SFFloat &value) {

    validateRadius(value);

    _radius = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setRadius() for range validation.
   */
  void setRadiusUnchecked(const SFFloat &value) { _radius = value; }

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
  static void checkRangesAttenuation(const SFVec3f &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesBeamWidth(const SFFloat &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesCutOffAngle(const SFFloat &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesRadius(const SFFloat &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

private:
  static void validateAttenuation(const SFVec3f &value) {

    if (value.x < 0.0f)
      throw std::out_of_range("attenuation.x below minimum of 0");

    if (value.y < 0.0f)
      throw std::out_of_range("attenuation.y below minimum of 0");

    if (value.z < 0.0f)
      throw std::out_of_range("attenuation.z below minimum of 0");
  }

  static void validateBeamWidth(const SFFloat &value) {

    if (value > 1.570796f)
      throw std::out_of_range("beamWidth above maximum of 1.570796");
  }

  static void validateCutOffAngle(const SFFloat &value) {

    if (value > 1.570796f)
      throw std::out_of_range("cutOffAngle above maximum of 1.570796");
  }

  static void validateRadius(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("radius below minimum of 0");
  }

  SFVec3f _attenuation{SFVec3f{1, 0, 0}};

  SFFloat _beamWidth{0.589049};

  SFFloat _cutOffAngle{1.570796};

  SFVec3f _direction{SFVec3f{0, 0, -1}};

  SFBool _global{true};

  SFVec3f _location{SFVec3f{0, 0, 0}};

  SFFloat _radius{100};
};

} // namespace x3d::nodes
