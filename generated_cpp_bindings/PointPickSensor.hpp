// PointPickSensor.hpp
#ifndef POINTPICKSENSOR_HPP
#define POINTPICKSENSOR_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DSensorNode.hpp"

#include "X3DPickSensorNode.hpp"

/**
 * @class PointPickSensor
 * @brief PointPickSensor tests one or more pickingGeometry points in space as
 * lying inside the provided pickTarget geometry.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/picking.html#PointPickSensor
 */
class PointPickSensor : public virtual X3DPickSensorNode {
public:
  /**
   * @brief Default constructor for PointPickSensor
   */
  PointPickSensor() = default;

  /**
   * @brief Destructor for PointPickSensor
   */
  ~PointPickSensor() = default;

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
  static std::string componentName() { return "Picking"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of pickedPoint. AccessType: outputOnly
   * @details Output event containing 3D points on surface of underlying
   * pickingGeometry computed by the picking intersection computations, given in
   * the local coordinate system.
   * @return MFVec3f The current value of pickedPoint.
   */
  MFVec3f getPickedPoint() const { return _pickedPoint; }

  /**
   * @brief Emit an output value on pickedPoint. AccessType: outputOnly
   * @details Output event containing 3D points on surface of underlying
   * pickingGeometry computed by the picking intersection computations, given in
   * the local coordinate system. outputOnly fields have no author-facing
   * setter; a node's behavior or the runtime calls this to produce an output
   * event. The event cascade reaches it through the reflected field table so
   * producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitPickedPoint(const MFVec3f &value) { _pickedPoint = value; }

  /**
   * @brief The X3D type name of this node (e.g. "PointPickSensor").
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
   * @brief Member variable for pickedPoint.
   */

  MFVec3f _pickedPoint{};
};

#endif // POINTPICKSENSOR_HPP