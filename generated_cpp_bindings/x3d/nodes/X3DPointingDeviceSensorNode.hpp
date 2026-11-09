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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DPointingDeviceSensorNode
 * @brief Base type for all pointing device sensors.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/pointingDeviceSensor.html#X3DPointingDeviceSensorNode
 */
class X3DPointingDeviceSensorNode : public virtual X3DSensorNode {
public:
  X3DPointingDeviceSensorNode() = default;

  virtual ~X3DPointingDeviceSensorNode() = default;

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "PointingDeviceSensor"; }

  static int componentLevel() { return 1; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFBool &getIsOver() const { return _isOver; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitIsOver(const SFBool &value) { _isOver = value; }

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
  SFBool _isOver{};
};

} // namespace x3d::nodes
