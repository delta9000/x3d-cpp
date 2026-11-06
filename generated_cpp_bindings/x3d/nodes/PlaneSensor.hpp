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

#include "x3d/nodes/X3DSensorNode.hpp"

#include "x3d/nodes/X3DPointingDeviceSensorNode.hpp"

#include "x3d/nodes/X3DDragSensorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class PlaneSensor
 * @brief PlaneSensor converts pointing device motion into 2D translation
 * parallel to the local Z=0 plane.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/pointingDeviceSensor.html#PlaneSensor
 */
class PlaneSensor : public virtual X3DDragSensorNode {
public:
  PlaneSensor() = default;

  ~PlaneSensor() = default;

  static SFRotation getDefaultAxisRotation() { return SFRotation{0, 0, 1, 0}; }

  static SFVec2f getDefaultMaxPosition() { return SFVec2f{-1, -1}; }

  static SFVec2f getDefaultMinPosition() { return SFVec2f{0, 0}; }

  static SFVec3f getDefaultOffset() { return SFVec3f{0, 0, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "PointingDeviceSensor"; }

  static int componentLevel() { return 1; }

  /**
   * SFRotation; access: inputOutput. Borrows the stored value.
   * @details axisRotation determines local sensor coordinate system by rotating
   * the local coordinate system.
   */
  const SFRotation &getAxisRotation() const { return _axisRotation; }

  /**
   * Access: inputOutput.
   * @details axisRotation determines local sensor coordinate system by rotating
   * the local coordinate system.
   */
  void setAxisRotation(const SFRotation &value) { _axisRotation = value; }

  void setAxisRotation(SFRotation &&value) { _axisRotation = std::move(value); }

  /**
   * SFVec2f; access: inputOutput. Borrows the stored value.
   * @details minPosition and maxPosition clamp translations to a range of
   * values measured from origin of Z=0 plane default maxPosition < minPosition
   * means no clamping.
   */
  const SFVec2f &getMaxPosition() const { return _maxPosition; }

  /**
   * Access: inputOutput.
   * @details minPosition and maxPosition clamp translations to a range of
   * values measured from origin of Z=0 plane default maxPosition < minPosition
   * means no clamping.
   */
  void setMaxPosition(const SFVec2f &value) { _maxPosition = value; }

  void setMaxPosition(SFVec2f &&value) { _maxPosition = std::move(value); }

  /**
   * SFVec2f; access: inputOutput. Borrows the stored value.
   * @details minPosition and maxPosition clamp translations to a range of
   * values measured from origin of Z=0 plane default maxPosition < minPosition
   * means no clamping.
   */
  const SFVec2f &getMinPosition() const { return _minPosition; }

  /**
   * Access: inputOutput.
   * @details minPosition and maxPosition clamp translations to a range of
   * values measured from origin of Z=0 plane default maxPosition < minPosition
   * means no clamping.
   */
  void setMinPosition(const SFVec2f &value) { _minPosition = value; }

  void setMinPosition(SFVec2f &&value) { _minPosition = std::move(value); }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Sends event and remembers last value sensed.
   */
  const SFVec3f &getOffset() const { return _offset; }

  /**
   * Access: inputOutput.
   * @details Sends event and remembers last value sensed.
   */
  void setOffset(const SFVec3f &value) { _offset = value; }

  void setOffset(SFVec3f &&value) { _offset = std::move(value); }

  /**
   * SFVec3f; access: outputOnly. Borrows the stored value.
   * @details translation_changed events equal sum of relative translation
   * change plus offset value.
   */
  const SFVec3f &getTranslation_changed() const { return _translation_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details translation_changed events equal sum of relative translation
   * change plus offset value.
   */
  void emitTranslation_changed(const SFVec3f &value) {
    _translation_changed = value;
  }

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

private:
  SFRotation _axisRotation{SFRotation{0, 0, 1, 0}};

  SFVec2f _maxPosition{SFVec2f{-1, -1}};

  SFVec2f _minPosition{SFVec2f{0, 0}};

  SFVec3f _offset{SFVec3f{0, 0, 0}};

  SFVec3f _translation_changed{};
};

} // namespace x3d::nodes
