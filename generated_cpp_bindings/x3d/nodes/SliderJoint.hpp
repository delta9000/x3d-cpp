#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DRigidJointNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class SliderJoint
 * @brief SliderJoint constrains all movement between body1 and body2 along a
 * single axis. Contains two RigidBody nodes (containerField values body1,
 * body2).
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#SliderJoint
 */
class SliderJoint : public virtual X3DRigidJointNode {
public:
  SliderJoint() = default;

  ~SliderJoint() = default;

  static SFVec3f getDefaultAxis() { return SFVec3f{0, 1, 0}; }

  static SFFloat getDefaultMaxSeparation() { return 1; }

  static SFFloat getDefaultMinSeparation() { return 0; }

  static SFFloat getDefaultSliderForce() { return 0; }

  static SFFloat getDefaultStopBounce() { return 0; }

  static SFFloat getDefaultStopErrorCorrection() { return 1; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "joints"; }

  static std::string componentName() { return "RigidBodyPhysics"; }

  static int componentLevel() { return 2; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details axis is normalized vector specifying direction of motion.
   */
  const SFVec3f &getAxis() const { return _axis; }

  /**
   * Access: inputOutput.
   * @details axis is normalized vector specifying direction of motion.
   */
  void setAxis(const SFVec3f &value) { _axis = value; }

  void setAxis(SFVec3f &&value) { _axis = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details maxSeparation is maximum separation distance between the two
   * bodies.
   */
  const SFFloat &getMaxSeparation() const { return _maxSeparation; }

  /**
   * Access: inputOutput.
   * @details maxSeparation is maximum separation distance between the two
   * bodies.
   */
  void setMaxSeparation(const SFFloat &value) { _maxSeparation = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details minSeparation is minimum separation distance between the two
   * bodies.
   */
  const SFFloat &getMinSeparation() const { return _minSeparation; }

  /**
   * Access: inputOutput.
   * @details minSeparation is minimum separation distance between the two
   * bodies.
   */
  void setMinSeparation(const SFFloat &value) { _minSeparation = value; }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details separation indicates final separation distance between the two
   * bodies.
   */
  const SFFloat &getSeparation() const { return _separation; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details separation indicates final separation distance between the two
   * bodies.
   */
  void emitSeparation(const SFFloat &value) { _separation = value; }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details separationRate indicates change in separation distance over time
   * between the two bodies.
   */
  const SFFloat &getSeparationRate() const { return _separationRate; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details separationRate indicates change in separation distance over time
   * between the two bodies.
   */
  void emitSeparationRate(const SFFloat &value) { _separationRate = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details sliderForce value is used to apply a force (specified in force
   * base units) along the axis of the slider in equal and opposite directions
   * to the two bodies.
   */
  const SFFloat &getSliderForce() const { return _sliderForce; }

  /**
   * Access: inputOutput.
   * @details sliderForce value is used to apply a force (specified in force
   * base units) along the axis of the slider in equal and opposite directions
   * to the two bodies.
   */
  void setSliderForce(const SFFloat &value) { _sliderForce = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details stopBounce is velocity factor for bounce back once stop point is
   * reached.
   */
  const SFFloat &getStopBounce() const { return _stopBounce; }

  /**
   * Access: inputOutput.
   * @details stopBounce is velocity factor for bounce back once stop point is
   * reached.
   */
  void setStopBounce(const SFFloat &value) { _stopBounce = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details stopErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   */
  const SFFloat &getStopErrorCorrection() const { return _stopErrorCorrection; }

  /**
   * Access: inputOutput.
   * @details stopErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   */
  void setStopErrorCorrection(const SFFloat &value) {

    _stopErrorCorrection = value;
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
  SFVec3f _axis{SFVec3f{0, 1, 0}};

  SFFloat _maxSeparation{1};

  SFFloat _minSeparation{0};

  SFFloat _separation{};

  SFFloat _separationRate{};

  SFFloat _sliderForce{0};

  SFFloat _stopBounce{0};

  SFFloat _stopErrorCorrection{1};
};

} // namespace x3d::nodes
