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
 * @class LinePickSensor
 * @brief LinePickSensor uses one or more pickingGeometry line segments to
 * compute intersections with pickTarget shapes.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/picking.html#LinePickSensor
 */
class LinePickSensor : public virtual X3DPickSensorNode {
public:
  LinePickSensor() = default;

  ~LinePickSensor() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Picking"; }

  static int componentLevel() { return 1; }

  /**
   * MFVec3f; access: outputOnly. Borrows the stored value.
   * @details Output event containing surface normal vectors computed by the
   * picking intersection computations.
   */
  const MFVec3f &getPickedNormal() const { return _pickedNormal; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Output event containing surface normal vectors computed by the
   * picking intersection computations.
   */
  void emitPickedNormal(const MFVec3f &value) { _pickedNormal = value; }

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

  /**
   * MFVec3f; access: outputOnly. Borrows the stored value.
   * @details Output event containing 3D texture coordinates of surfaces
   * computed by the picking intersection computations.
   */
  const MFVec3f &getPickedTextureCoordinate() const {
    return _pickedTextureCoordinate;
  }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Output event containing 3D texture coordinates of surfaces
   * computed by the picking intersection computations.
   */
  void emitPickedTextureCoordinate(const MFVec3f &value) {
    _pickedTextureCoordinate = value;
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
  MFVec3f _pickedNormal{};

  MFVec3f _pickedPoint{};

  MFVec3f _pickedTextureCoordinate{};
};

} // namespace x3d::nodes
