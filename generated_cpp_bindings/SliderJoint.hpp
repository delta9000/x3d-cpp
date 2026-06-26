// SliderJoint.hpp
#ifndef SLIDERJOINT_HPP
#define SLIDERJOINT_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DRigidJointNode.hpp"

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
  /**
   * @brief Default constructor for SliderJoint
   */
  SliderJoint() = default;

  /**
   * @brief Destructor for SliderJoint
   */
  ~SliderJoint() = default;

  /**
   * @brief Get the default value for axis
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultAxis() { return SFVec3f{0, 1, 0}; }

  /**
   * @brief Get the default value for maxSeparation
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMaxSeparation() { return 1; }

  /**
   * @brief Get the default value for minSeparation
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMinSeparation() { return 0; }

  /**
   * @brief Get the default value for sliderForce
   * @return SFFloat The default value
   */
  static SFFloat getDefaultSliderForce() { return 0; }

  /**
   * @brief Get the default value for stopBounce
   * @return SFFloat The default value
   */
  static SFFloat getDefaultStopBounce() { return 0; }

  /**
   * @brief Get the default value for stopErrorCorrection
   * @return SFFloat The default value
   */
  static SFFloat getDefaultStopErrorCorrection() { return 1; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "joints"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "RigidBodyPhysics"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of axis. AccessType: inputOutput
   * @details axis is normalized vector specifying direction of motion.
   * @return SFVec3f The current value of axis.
   */
  SFVec3f getAxis() const { return _axis; }

  /**
   * @brief Sets the value of axis. AccessType: inputOutput
   * @details axis is normalized vector specifying direction of motion.
   * @param value The new value for axis.
   */
  void setAxis(const SFVec3f &value) { _axis = value; }

  void setAxis(SFVec3f &&value) { _axis = std::move(value); }

  /**
   * @brief Gets the value of maxSeparation. AccessType: inputOutput
   * @details maxSeparation is maximum separation distance between the two
   * bodies.
   * @return SFFloat The current value of maxSeparation.
   */
  SFFloat getMaxSeparation() const { return _maxSeparation; }

  /**
   * @brief Sets the value of maxSeparation. AccessType: inputOutput
   * @details maxSeparation is maximum separation distance between the two
   * bodies.
   * @param value The new value for maxSeparation.
   */
  void setMaxSeparation(const SFFloat &value) { _maxSeparation = value; }

  /**
   * @brief Gets the value of minSeparation. AccessType: inputOutput
   * @details minSeparation is minimum separation distance between the two
   * bodies.
   * @return SFFloat The current value of minSeparation.
   */
  SFFloat getMinSeparation() const { return _minSeparation; }

  /**
   * @brief Sets the value of minSeparation. AccessType: inputOutput
   * @details minSeparation is minimum separation distance between the two
   * bodies.
   * @param value The new value for minSeparation.
   */
  void setMinSeparation(const SFFloat &value) { _minSeparation = value; }

  /**
   * @brief Gets the value of separation. AccessType: outputOnly
   * @details separation indicates final separation distance between the two
   * bodies.
   * @return SFFloat The current value of separation.
   */
  SFFloat getSeparation() const { return _separation; }

  /**
   * @brief Emit an output value on separation. AccessType: outputOnly
   * @details separation indicates final separation distance between the two
   * bodies. outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitSeparation(const SFFloat &value) { _separation = value; }

  /**
   * @brief Gets the value of separationRate. AccessType: outputOnly
   * @details separationRate indicates change in separation distance over time
   * between the two bodies.
   * @return SFFloat The current value of separationRate.
   */
  SFFloat getSeparationRate() const { return _separationRate; }

  /**
   * @brief Emit an output value on separationRate. AccessType: outputOnly
   * @details separationRate indicates change in separation distance over time
   * between the two bodies. outputOnly fields have no author-facing setter; a
   * node's behavior or the runtime calls this to produce an output event. The
   * event cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitSeparationRate(const SFFloat &value) { _separationRate = value; }

  /**
   * @brief Gets the value of sliderForce. AccessType: inputOutput
   * @details sliderForce value is used to apply a force (specified in force
   * base units) along the axis of the slider in equal and opposite directions
   * to the two bodies.
   * @return SFFloat The current value of sliderForce.
   */
  SFFloat getSliderForce() const { return _sliderForce; }

  /**
   * @brief Sets the value of sliderForce. AccessType: inputOutput
   * @details sliderForce value is used to apply a force (specified in force
   * base units) along the axis of the slider in equal and opposite directions
   * to the two bodies.
   * @param value The new value for sliderForce.
   */
  void setSliderForce(const SFFloat &value) { _sliderForce = value; }

  /**
   * @brief Gets the value of stopBounce. AccessType: inputOutput
   * @details stopBounce is velocity factor for bounce back once stop point is
   * reached.
   * @return SFFloat The current value of stopBounce.
   */
  SFFloat getStopBounce() const { return _stopBounce; }

  /**
   * @brief Sets the value of stopBounce. AccessType: inputOutput
   * @details stopBounce is velocity factor for bounce back once stop point is
   * reached.
   * @param value The new value for stopBounce.
   */
  void setStopBounce(const SFFloat &value) { _stopBounce = value; }

  /**
   * @brief Gets the value of stopErrorCorrection. AccessType: inputOutput
   * @details stopErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   * @return SFFloat The current value of stopErrorCorrection.
   */
  SFFloat getStopErrorCorrection() const { return _stopErrorCorrection; }

  /**
   * @brief Sets the value of stopErrorCorrection. AccessType: inputOutput
   * @details stopErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   * @param value The new value for stopErrorCorrection.
   */
  void setStopErrorCorrection(const SFFloat &value) {

    _stopErrorCorrection = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "SliderJoint").
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
   * @brief Member variable for axis.
   */

  SFVec3f _axis{SFVec3f{0, 1, 0}};

  /**
   * @brief Member variable for maxSeparation.
   */

  SFFloat _maxSeparation{1};

  /**
   * @brief Member variable for minSeparation.
   */

  SFFloat _minSeparation{0};

  /**
   * @brief Member variable for separation.
   */

  SFFloat _separation{};

  /**
   * @brief Member variable for separationRate.
   */

  SFFloat _separationRate{};

  /**
   * @brief Member variable for sliderForce.
   */

  SFFloat _sliderForce{0};

  /**
   * @brief Member variable for stopBounce.
   */

  SFFloat _stopBounce{0};

  /**
   * @brief Member variable for stopErrorCorrection.
   */

  SFFloat _stopErrorCorrection{1};
};

#endif // SLIDERJOINT_HPP