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

#include "x3d/nodes/X3DPickSensorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class VolumePickSensor
 * @brief VolumePickSensor tests picking intersections using the pickingGeometry
 * against the pickTarget geometry volume.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/picking.html#VolumePickSensor
 */
class VolumePickSensor : public virtual X3DPickSensorNode {
public:
  VolumePickSensor() = default;

  ~VolumePickSensor() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Picking"; }

  static int componentLevel() { return 3; }

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
