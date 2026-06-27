// Coordinate.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DGeometricPropertyNode.hpp"

#include "x3d/nodes/X3DCoordinateNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Coordinate
 * @brief Coordinate builds geometry by defining a set of 3D coordinate
 * (triplet) point values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rendering.html#Coordinate
 */
class Coordinate : public virtual X3DCoordinateNode {
public:
  /**
   * @brief Default constructor for Coordinate
   */
  Coordinate() = default;

  /**
   * @brief Destructor for Coordinate
   */
  ~Coordinate() = default;

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesX3DCoordinateNode";
  }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "coord"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Rendering"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of point. AccessType: inputOutput
   * @details point contains a set of 3D coordinate (triplet) point values.
   * @return MFVec3f The current value of point.
   */
  MFVec3f getPoint() const { return _point; }

  /**
   * @brief Sets the value of point. AccessType: inputOutput
   * @details point contains a set of 3D coordinate (triplet) point values.
   * @param value The new value for point.
   */
  void setPoint(const MFVec3f &value) { _point = value; }

  void setPoint(MFVec3f &&value) { _point = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "Coordinate").
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
   * @brief Member variable for point.
   */

  MFVec3f _point{};
};

} // namespace x3d::nodes
