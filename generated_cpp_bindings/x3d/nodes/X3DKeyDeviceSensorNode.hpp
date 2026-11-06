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
 * @class X3DKeyDeviceSensorNode
 * @brief Base type for all sensor node types that operate using key devices.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/keyDeviceSensor.html#X3DKeyDeviceSensorNode
 */
class X3DKeyDeviceSensorNode : public virtual X3DSensorNode {
public:
  X3DKeyDeviceSensorNode() = default;

  virtual ~X3DKeyDeviceSensorNode() = default;

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "KeyDeviceSensor"; }

  static int componentLevel() { return 1; }

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
};

} // namespace x3d::nodes
