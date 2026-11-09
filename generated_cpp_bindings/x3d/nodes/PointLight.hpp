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
 * @class PointLight
 * @brief Linear attenuation may occur at level 2, full support at level 3.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/lighting.html#PointLight
 */
class PointLight : public virtual X3DLightNode {
public:
  PointLight() = default;

  ~PointLight() = default;

  static SFVec3f getDefaultAttenuation() { return SFVec3f{1, 0, 0}; }

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

  static void validateRadius(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("radius below minimum of 0");
  }

  SFVec3f _attenuation{SFVec3f{1, 0, 0}};

  SFBool _global{true};

  SFVec3f _location{SFVec3f{0, 0, 0}};

  SFFloat _radius{100};
};

} // namespace x3d::nodes
