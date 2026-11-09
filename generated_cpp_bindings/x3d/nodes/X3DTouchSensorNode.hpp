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
 * @class X3DTouchSensorNode
 * @brief Base type for all touch-style pointing device sensors.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/pointingDeviceSensor.html#X3DTouchSensorNode
 */
class X3DTouchSensorNode : public virtual X3DPointingDeviceSensorNode {
public:
  X3DTouchSensorNode() = default;

  virtual ~X3DTouchSensorNode() = default;

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "PointingDeviceSensor"; }

  static int componentLevel() { return 1; }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details
   */
  const SFTime &getTouchTime() const { return _touchTime; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitTouchTime(const SFTime &value) { _touchTime = value; }

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
  SFTime _touchTime{};
};

} // namespace x3d::nodes
