// PlaneSensor.hpp
#ifndef PLANESENSOR_HPP
#define PLANESENSOR_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DSensorNode.hpp"

#include "X3DPointingDeviceSensorNode.hpp"

#include "X3DDragSensorNode.hpp"

/**
 * @class PlaneSensor
 * @brief PlaneSensor converts pointing device motion into 2D translation
 * parallel to the local Z=0 plane.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/pointingDeviceSensor.html#PlaneSensor
 */
class PlaneSensor : public virtual X3DDragSensorNode {
public:
  /**
   * @brief Default constructor for PlaneSensor
   */
  PlaneSensor() = default;

  /**
   * @brief Destructor for PlaneSensor
   */
  ~PlaneSensor() = default;

  /**
   * @brief Get the default value for axisRotation
   * @return SFRotation The default value
   */
  static SFRotation getDefaultAxisRotation() { return SFRotation{0, 0, 1, 0}; }

  /**
   * @brief Get the default value for maxPosition
   * @return SFVec2f The default value
   */
  static SFVec2f getDefaultMaxPosition() { return SFVec2f{-1, -1}; }

  /**
   * @brief Get the default value for minPosition
   * @return SFVec2f The default value
   */
  static SFVec2f getDefaultMinPosition() { return SFVec2f{0, 0}; }

  /**
   * @brief Get the default value for offset
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultOffset() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "PointingDeviceSensor"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of axisRotation. AccessType: inputOutput
   * @details axisRotation determines local sensor coordinate system by rotating
   * the local coordinate system.
   * @return SFRotation The current value of axisRotation.
   */
  SFRotation getAxisRotation() const { return _axisRotation; }

  /**
   * @brief Sets the value of axisRotation. AccessType: inputOutput
   * @details axisRotation determines local sensor coordinate system by rotating
   * the local coordinate system.
   * @param value The new value for axisRotation.
   */
  void setAxisRotation(const SFRotation &value) { _axisRotation = value; }

  void setAxisRotation(SFRotation &&value) { _axisRotation = std::move(value); }

  /**
   * @brief Gets the value of maxPosition. AccessType: inputOutput
   * @details minPosition and maxPosition clamp translations to a range of
   * values measured from origin of Z=0 plane default maxPosition < minPosition
   * means no clamping.
   * @return SFVec2f The current value of maxPosition.
   */
  SFVec2f getMaxPosition() const { return _maxPosition; }

  /**
   * @brief Sets the value of maxPosition. AccessType: inputOutput
   * @details minPosition and maxPosition clamp translations to a range of
   * values measured from origin of Z=0 plane default maxPosition < minPosition
   * means no clamping.
   * @param value The new value for maxPosition.
   */
  void setMaxPosition(const SFVec2f &value) { _maxPosition = value; }

  void setMaxPosition(SFVec2f &&value) { _maxPosition = std::move(value); }

  /**
   * @brief Gets the value of minPosition. AccessType: inputOutput
   * @details minPosition and maxPosition clamp translations to a range of
   * values measured from origin of Z=0 plane default maxPosition < minPosition
   * means no clamping.
   * @return SFVec2f The current value of minPosition.
   */
  SFVec2f getMinPosition() const { return _minPosition; }

  /**
   * @brief Sets the value of minPosition. AccessType: inputOutput
   * @details minPosition and maxPosition clamp translations to a range of
   * values measured from origin of Z=0 plane default maxPosition < minPosition
   * means no clamping.
   * @param value The new value for minPosition.
   */
  void setMinPosition(const SFVec2f &value) { _minPosition = value; }

  void setMinPosition(SFVec2f &&value) { _minPosition = std::move(value); }

  /**
   * @brief Gets the value of offset. AccessType: inputOutput
   * @details Sends event and remembers last value sensed.
   * @return SFVec3f The current value of offset.
   */
  SFVec3f getOffset() const { return _offset; }

  /**
   * @brief Sets the value of offset. AccessType: inputOutput
   * @details Sends event and remembers last value sensed.
   * @param value The new value for offset.
   */
  void setOffset(const SFVec3f &value) { _offset = value; }

  void setOffset(SFVec3f &&value) { _offset = std::move(value); }

  /**
   * @brief Gets the value of translation_changed. AccessType: outputOnly
   * @details translation_changed events equal sum of relative translation
   * change plus offset value.
   * @return SFVec3f The current value of translation_changed.
   */
  SFVec3f getTranslation_changed() const { return _translation_changed; }

  /**
   * @brief Emit an output value on translation_changed. AccessType: outputOnly
   * @details translation_changed events equal sum of relative translation
   * change plus offset value. outputOnly fields have no author-facing setter; a
   * node's behavior or the runtime calls this to produce an output event. The
   * event cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitTranslation_changed(const SFVec3f &value) {
    _translation_changed = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "PlaneSensor").
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

private:
  /**
   * @brief Member variable for axisRotation.
   */

  SFRotation _axisRotation{SFRotation{0, 0, 1, 0}};

  /**
   * @brief Member variable for maxPosition.
   */

  SFVec2f _maxPosition{SFVec2f{-1, -1}};

  /**
   * @brief Member variable for minPosition.
   */

  SFVec2f _minPosition{SFVec2f{0, 0}};

  /**
   * @brief Member variable for offset.
   */

  SFVec3f _offset{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for translation_changed.
   */

  SFVec3f _translation_changed{};
};

#endif // PLANESENSOR_HPP