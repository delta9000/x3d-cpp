// VisibilitySensor.hpp
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

#include "x3d/nodes/X3DEnvironmentalSensorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class VisibilitySensor
 * @brief VisibilitySensor detects when user can see a specific object or region
 * as they navigate the world.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalSensor.html#VisibilitySensor
 */
class VisibilitySensor : public virtual X3DEnvironmentalSensorNode {
public:
  /**
   * @brief Default constructor for VisibilitySensor
   */
  VisibilitySensor() = default;

  /**
   * @brief Destructor for VisibilitySensor
   */
  ~VisibilitySensor() = default;

  /**
   * @brief Get the default value for center
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultCenter() { return SFVec3f{0, 0, 0}; }

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
  static std::string componentName() { return "EnvironmentalSensor"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of center. AccessType: inputOutput
   * @details Translation offset from origin of local coordinate system.
   * @return SFVec3f The current value of center.
   */
  SFVec3f getCenter() const { return _center; }

  /**
   * @brief Sets the value of center. AccessType: inputOutput
   * @details Translation offset from origin of local coordinate system.
   * @param value The new value for center.
   */
  void setCenter(const SFVec3f &value) { _center = value; }

  void setCenter(SFVec3f &&value) { _center = std::move(value); }

  /**
   * @brief Gets the value of enterTime. AccessType: outputOnly
   * @details Time event generated when user's camera enters visibility region
   * for sensor.
   * @return SFTime The current value of enterTime.
   */
  SFTime getEnterTime() const { return _enterTime; }

  /**
   * @brief Emit an output value on enterTime. AccessType: outputOnly
   * @details Time event generated when user's camera enters visibility region
   * for sensor. outputOnly fields have no author-facing setter; a node's
   * behavior or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitEnterTime(const SFTime &value) { _enterTime = value; }

  /**
   * @brief Gets the value of exitTime. AccessType: outputOnly
   * @details Time event generated when user's camera exits visibility region
   * for sensor.
   * @return SFTime The current value of exitTime.
   */
  SFTime getExitTime() const { return _exitTime; }

  /**
   * @brief Emit an output value on exitTime. AccessType: outputOnly
   * @details Time event generated when user's camera exits visibility region
   * for sensor. outputOnly fields have no author-facing setter; a node's
   * behavior or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitExitTime(const SFTime &value) { _exitTime = value; }

  /**
   * @brief The X3D type name of this node (e.g. "VisibilitySensor").
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

private:
  /**
   * @brief Member variable for center.
   */

  SFVec3f _center{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for enterTime.
   */

  SFTime _enterTime{};

  /**
   * @brief Member variable for exitTime.
   */

  SFTime _exitTime{};
};

} // namespace x3d::nodes
