// Circle2D.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DGeometryNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class Circle2D
 * @brief Circle2D is a geometry node that defines a linear X-Y circle with
 * center (0,0) in X-Y plane.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/geometry2D.html#Circle2D
 */
class Circle2D : public virtual X3DGeometryNode {
public:
  /**
   * @brief Default constructor for Circle2D
   */
  Circle2D() = default;

  /**
   * @brief Destructor for Circle2D
   */
  ~Circle2D() = default;

  /**
   * @brief Get the default value for radius
   * @return SFFloat The default value
   */
  static SFFloat getDefaultRadius() { return 1; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "geometry"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Geometry2D"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of radius. AccessType: initializeOnly
   * @details circle radius.
   * @return SFFloat The current value of radius.
   */
  SFFloat getRadius() const { return _radius; }
  /**
   * @brief Data-layer write of radius (reader/init ingest path).
   * @details radius is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setRadius().
   */
  void setRadiusUnchecked(const SFFloat &value) { _radius = value; }

  /**
   * @brief The X3D type name of this node (e.g. "Circle2D").
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
   * @brief Member variable for radius.
   */

  SFFloat _radius{1};
};

} // namespace x3d::nodes
