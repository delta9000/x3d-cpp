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
 * @class PointPickSensor
 * @brief PointPickSensor tests one or more pickingGeometry points in space as
 * lying inside the provided pickTarget geometry.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/picking.html#PointPickSensor
 */
class PointPickSensor : public virtual X3DPickSensorNode {
public:
  PointPickSensor() = default;

  ~PointPickSensor() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Picking"; }

  static int componentLevel() { return 1; }

  /**
   * MFVec3f; access: outputOnly. Borrows the stored value.
   * @details Output event containing 3D points on surface of underlying
   * pickingGeometry computed by the picking intersection computations, given in
   * the local coordinate system.
   */
  const MFVec3f &getPickedPoint() const { return _pickedPoint; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Output event containing 3D points on surface of underlying
   * pickingGeometry computed by the picking intersection computations, given in
   * the local coordinate system.
   */
  void emitPickedPoint(const MFVec3f &value) { _pickedPoint = value; }

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
  MFVec3f _pickedPoint{};
};

} // namespace x3d::nodes
