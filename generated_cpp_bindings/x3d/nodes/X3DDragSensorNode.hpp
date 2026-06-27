// X3DDragSensorNode.hpp
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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DDragSensorNode
 * @brief Base type for all drag-style pointing device sensors.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/pointingDeviceSensor.html#X3DDragSensorNode
 */
class X3DDragSensorNode : public virtual X3DPointingDeviceSensorNode {
public:
  /**
   * @brief Default constructor for X3DDragSensorNode
   */
  X3DDragSensorNode() = default;

  /**
   * @brief Virtual destructor for X3DDragSensorNode
   */
  virtual ~X3DDragSensorNode() = default;

  /**
   * @brief Get the default value for autoOffset
   * @return SFBool The default value
   */
  static SFBool getDefaultAutoOffset() { return true; }

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
  static std::string componentName() { return "PointingDeviceSensor"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of autoOffset. AccessType: inputOutput
   * @details
   * @return SFBool The current value of autoOffset.
   */
  SFBool getAutoOffset() const { return _autoOffset; }

  /**
   * @brief Sets the value of autoOffset. AccessType: inputOutput
   * @details
   * @param value The new value for autoOffset.
   */
  void setAutoOffset(const SFBool &value) { _autoOffset = value; }

  /**
   * @brief Gets the value of trackPoint_changed. AccessType: outputOnly
   * @details
   * @return SFVec3f The current value of trackPoint_changed.
   */
  SFVec3f getTrackPoint_changed() const { return _trackPoint_changed; }

  /**
   * @brief Emit an output value on trackPoint_changed. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitTrackPoint_changed(const SFVec3f &value) {
    _trackPoint_changed = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "X3DDragSensorNode").
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
   * @brief Member variable for autoOffset.
   */

  SFBool _autoOffset{true};

  /**
   * @brief Member variable for trackPoint_changed.
   */

  SFVec3f _trackPoint_changed{};
};

} // namespace x3d::nodes
